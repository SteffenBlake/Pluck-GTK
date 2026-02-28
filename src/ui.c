/**
 * ui.c — GTK window construction and signal handlers.
 *
 * Responsibilities:
 *   • Build the layer-shell overlay window (search entry + results list).
 *   • Handle keyboard input (Escape to dismiss, arrow keys via GTK defaults).
 *   • Run fd + fzf on every keystroke and populate the results list.
 *   • Apply minimal CSS (rounded window corners, search entry margins).
 */

#include "ui.h"
#include "config.h"
#include "files.h"
#include "search.h"

#include <gtk/gtk.h>
#include <gtk4-layer-shell.h>
#include <glib.h>
#include <string.h>

/* Maximum number of search results shown at one time. */
#define MAX_RESULTS 10

/* Maximum length of the fd + fzf shell command string. */
#define CMD_BUFFER_SIZE 4096

/* Maximum length of a single result line from fzf. */
#define RESULT_LINE_SIZE 1024

/* Fraction of monitor width used for the overlay window. */
#define WINDOW_WIDTH_FRACTION  0.5

/* Fraction of monitor height used as the top margin (vertical placement). */
#define WINDOW_TOP_FRACTION    0.33

/* Maximum pixel height of the scrollable results list. */
#define RESULTS_MAX_HEIGHT 400

/* CSS applied at APPLICATION priority to style the overlay window. */
static const char *PLUCK_CSS =
    "window {"
    "  border-radius: 4px;"
    "}"
    "entry {"
    "  margin: 8px 12px 4px 12px;"
    "}";

/* -------------------------------------------------------------------------
 * Internal helpers
 * ---------------------------------------------------------------------- */

/**
 * clear_list:
 * @list: The GtkListBox whose rows should be removed.
 *
 * Removes every child row from @list, leaving it empty.
 */
static void clear_list(GtkListBox *list)
{
    GtkWidget *child = gtk_widget_get_first_child(GTK_WIDGET(list));
    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_list_box_remove(list, child);
        child = next;
    }
}

/**
 * apply_css:
 *
 * Loads PLUCK_CSS and registers it at APPLICATION priority so it affects
 * every widget in the default display.
 */
static void apply_css(void)
{
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider, PLUCK_CSS);
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

/* -------------------------------------------------------------------------
 * Internal types
 * ---------------------------------------------------------------------- */

/**
 * PluckUI:
 *
 * Bundles the window and search entry pointers so that signal handlers
 * which need both can receive them through a single user_data pointer.
 */
typedef struct {
    GtkWindow      *win;
    GtkSearchEntry *entry;
} PluckUI;

/* -------------------------------------------------------------------------
 * Signal handlers
 * ---------------------------------------------------------------------- */

/**
 * on_row_activated:
 *
 * Called when the user clicks a result row or presses Enter on it.
 * Extracts the plain-text file path stored in the row's GtkLabel and
 * delegates to open_containing_folder().
 */
static void on_row_activated(GtkListBox    *list,
                              GtkListBoxRow *row,
                              gpointer       user_data)
{
    (void)list;
    GtkWindow *win   = GTK_WINDOW(user_data);
    GtkWidget *label = gtk_list_box_row_get_child(row);

    if (label && GTK_IS_LABEL(label)) {
        const char *filepath = gtk_label_get_text(GTK_LABEL(label));
        if (filepath && *filepath)
            open_containing_folder(filepath, win);
    }
}

/**
 * update_results:
 *
 * Connected to the GtkSearchEntry "search-changed" signal.
 * Clears the current results list, then pipes the current query through
 * `fd | fzf` and appends up to MAX_RESULTS highlighted rows.
 */
static void update_results(GtkSearchEntry *entry, gpointer user_data)
{
    GtkListBox *list  = GTK_LIST_BOX(user_data);
    clear_list(list);

    const char *query = gtk_editable_get_text(GTK_EDITABLE(entry));
    if (!query || !*query)
        return;

    /* Shell-quote both arguments so that paths with spaces, quotes, or other
     * special characters cannot inject arbitrary shell commands. */
    gchar *quoted_root  = g_shell_quote(search_root);
    gchar *quoted_query = g_shell_quote(query);

    char cmd[CMD_BUFFER_SIZE];
    snprintf(cmd, sizeof(cmd),
             "fd --type f --hidden . %s | fzf -f %s | head -n %d",
             quoted_root, quoted_query, MAX_RESULTS);

    g_free(quoted_root);
    g_free(quoted_query);

    FILE *fp = popen(cmd, "r");
    if (!fp)
        return;

    char line[RESULT_LINE_SIZE];
    while (fgets(line, sizeof(line), fp)) {
        /* Strip trailing newline. */
        line[strcspn(line, "\n")] = '\0';

        char      *markup = create_highlighted_markup(line, query);
        GtkWidget *row    = gtk_list_box_row_new();

        /* The label stores the plain path as text and displays markup. */
        GtkWidget *label = gtk_label_new(line);
        gtk_label_set_markup(GTK_LABEL(label), markup);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0f);

        /* Ellipsise in the middle so long paths remain readable. */
        gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_MIDDLE);
        gtk_label_set_max_width_chars(GTK_LABEL(label), 1);
        gtk_widget_set_hexpand(label, TRUE);

        gtk_widget_set_margin_start(label, 12);
        gtk_widget_set_margin_end(label, 12);
        gtk_widget_set_margin_top(label, 4);
        gtk_widget_set_margin_bottom(label, 4);

        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
        gtk_list_box_append(list, row);

        g_free(markup);
    }

    pclose(fp);
}

/**
 * on_key_pressed:
 *
 * Capture-phase key controller attached to the window.
 * • Closes the window when Escape is pressed.
 * • When focus has moved to the results list (via arrow keys) and the user
 *   starts typing again, grabs focus back to the search entry so the
 *   keystroke is delivered there instead of to the list.
 */
static gboolean on_key_pressed(GtkEventControllerKey *controller,
                                guint                  keyval,
                                guint                  keycode,
                                GdkModifierType        state,
                                gpointer               user_data)
{
    (void)controller;
    (void)keycode;
    (void)state;

    PluckUI *ui = user_data;

    if (keyval == GDK_KEY_Escape) {
        gtk_window_close(ui->win);
        return TRUE;
    }

    /* If focus has moved away from the search entry (e.g. into the results
     * list) and the user presses a key that isn't a navigation or modifier
     * key, redirect focus back to the entry so typing resumes there. */
    GtkWidget *focused = gtk_root_get_focus(GTK_ROOT(ui->win));
    if (focused != GTK_WIDGET(ui->entry)) {
        switch (keyval) {
        case GDK_KEY_Up:
        case GDK_KEY_Down:
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
        case GDK_KEY_Tab:
        case GDK_KEY_ISO_Left_Tab:
            break;
        default:
            /* Grab focus back to the entry; return FALSE so the event
             * continues to propagate and is handled by the entry. */
            gtk_widget_grab_focus(GTK_WIDGET(ui->entry));
            return FALSE;
        }
    }

    return FALSE;
}

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

void activate(GtkApplication *app, gpointer user_data)
{
    (void)user_data;

    GtkWindow *win = GTK_WINDOW(gtk_application_window_new(app));

    /* ---- Determine window dimensions from the primary monitor ---- */
    int win_width  = 600;
    int margin_top = 200;

    GdkDisplay  *display  = gdk_display_get_default();
    GListModel  *monitors = gdk_display_get_monitors(display);
    if (g_list_model_get_n_items(monitors) > 0) {
        GdkMonitor  *monitor = g_list_model_get_item(monitors, 0);
        GdkRectangle geo;
        gdk_monitor_get_geometry(monitor, &geo);
        win_width  = (int)(geo.width  * WINDOW_WIDTH_FRACTION);
        margin_top = (int)(geo.height * WINDOW_TOP_FRACTION);
        g_object_unref(monitor);
    }

    /* ---- Attach to the Wayland layer shell ---- */
    gtk_layer_init_for_window(win);
    gtk_layer_set_layer(win, GTK_LAYER_SHELL_LAYER_TOP);
    gtk_layer_set_namespace(win, "pluck-gtk");
    gtk_layer_set_exclusive_zone(win, -1);
    gtk_layer_set_keyboard_mode(win, GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE);

    /* Anchor to the top edge only, then offset downward by margin_top. */
    gtk_layer_set_anchor(win, GTK_LAYER_SHELL_EDGE_TOP,    TRUE);
    gtk_layer_set_anchor(win, GTK_LAYER_SHELL_EDGE_LEFT,   FALSE);
    gtk_layer_set_anchor(win, GTK_LAYER_SHELL_EDGE_RIGHT,  FALSE);
    gtk_layer_set_anchor(win, GTK_LAYER_SHELL_EDGE_BOTTOM, FALSE);
    gtk_layer_set_margin(win, GTK_LAYER_SHELL_EDGE_TOP, margin_top);

    gtk_window_set_default_size(win, win_width, -1);
    gtk_window_set_resizable(win, FALSE);

    /* ---- Widget tree ---- */
    GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_widget_set_vexpand(GTK_WIDGET(box), FALSE);
    gtk_widget_set_size_request(GTK_WIDGET(box), win_width, -1);
    gtk_window_set_child(win, GTK_WIDGET(box));

    /* Search entry */
    GtkSearchEntry *entry = GTK_SEARCH_ENTRY(gtk_search_entry_new());
    gtk_widget_set_hexpand(GTK_WIDGET(entry), TRUE);
    gtk_box_append(box, GTK_WIDGET(entry));

    /* Scrollable results list */
    GtkScrolledWindow *scroll = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new());
    gtk_scrolled_window_set_policy(scroll, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_max_content_height(scroll, RESULTS_MAX_HEIGHT);
    gtk_scrolled_window_set_propagate_natural_height(scroll, TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(scroll), FALSE);
    gtk_widget_set_margin_start(GTK_WIDGET(scroll), 12);
    gtk_widget_set_margin_end(GTK_WIDGET(scroll), 12);
    gtk_widget_set_margin_bottom(GTK_WIDGET(scroll), 8);

    GtkListBox *list = GTK_LIST_BOX(gtk_list_box_new());
    gtk_list_box_set_selection_mode(list, GTK_SELECTION_SINGLE);
    gtk_widget_set_vexpand(GTK_WIDGET(list), FALSE);
    gtk_scrolled_window_set_child(scroll, GTK_WIDGET(list));
    gtk_box_append(box, GTK_WIDGET(scroll));

    /* ---- Signal connections ---- */
    g_signal_connect(list,  "row-activated", G_CALLBACK(on_row_activated), win);
    g_signal_connect(entry, "search-changed", G_CALLBACK(update_results), list);

    GtkEventController *key_ctrl = gtk_event_controller_key_new();
    gtk_event_controller_set_propagation_phase(key_ctrl, GTK_PHASE_CAPTURE);

    PluckUI *ui = g_new(PluckUI, 1);
    ui->win   = win;
    ui->entry = entry;
    /* ui is freed automatically when the window (and therefore the
     * controller) is destroyed. */
    g_object_set_data_full(G_OBJECT(win), "pluck-ui", ui, g_free);

    g_signal_connect(key_ctrl, "key-pressed", G_CALLBACK(on_key_pressed), ui);
    gtk_widget_add_controller(GTK_WIDGET(win), key_ctrl);

    apply_css();
    gtk_window_present(win);
}
