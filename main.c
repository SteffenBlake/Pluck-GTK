#include <gtk/gtk.h>
#include <gtk4-layer-shell.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>
#include <stdlib.h>

static char search_root[1024] = ".";
static int cached_width = 0;
static int cached_margin_top = 0;

static void clear_list(GtkListBox *list) {
    GtkWidget *child = gtk_widget_get_first_child(GTK_WIDGET(list));
    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_list_box_remove(list, child);
        child = next;
    }
}

/* Check if character is in query string */
static gboolean char_in_query(char c, const char *query) {
    char lower_c = tolower(c);
    for (const char *q = query; *q; q++) {
        if (tolower(*q) == lower_c) {
            return TRUE;
        }
    }
    return FALSE;
}

/* Find all exact match positions (case-insensitive) */
static gboolean find_exact_matches(const char *text, const char *query, gboolean *matches, int text_len) {
    int query_len = strlen(query);
    if (query_len == 0) return FALSE;
    
    gboolean found_any = FALSE;
    
    for (int i = 0; i <= text_len - query_len; i++) {
        if (strncasecmp(&text[i], query, query_len) == 0) {
            for (int j = 0; j < query_len; j++) {
                matches[i + j] = TRUE;
            }
            found_any = TRUE;
        }
    }
    
    return found_any;
}

/* Create markup string with highlighted matches */
static char* create_highlighted_markup(const char *text, const char *query) {
    GString *markup = g_string_new("");
    int text_len = strlen(text);
    
    /* Try to find exact matches first */
    gboolean *exact_matches = g_new0(gboolean, text_len);
    gboolean has_exact = find_exact_matches(text, query, exact_matches, text_len);
    
    for (int i = 0; i < text_len; i++) {
        char ch = text[i];
        char *escaped = g_markup_escape_text(&ch, 1);
        
        gboolean should_highlight = FALSE;
        
        if (has_exact) {
            /* Use exact match positions */
            should_highlight = exact_matches[i];
        } else {
            /* Fallback to character matching */
            should_highlight = char_in_query(ch, query);
        }
        
        if (should_highlight) {
            g_string_append_printf(markup, "<span weight='bold' foreground='#FFD700'>%s</span>", escaped);
        } else {
            g_string_append(markup, escaped);
        }
        
        g_free(escaped);
    }
    
    g_free(exact_matches);
    return g_string_free(markup, FALSE);
}

/* Callback for file launcher */
static void on_file_launcher_finish(GObject *source, GAsyncResult *result, gpointer user_data) {
    GtkWindow *win = GTK_WINDOW(user_data);
    GtkFileLauncher *launcher = GTK_FILE_LAUNCHER(source);
    GError *error = NULL;
    
    if (!gtk_file_launcher_open_containing_folder_finish(launcher, result, &error)) {
        if (error) {
            g_warning("Failed to open containing folder: %s", error->message);
            g_error_free(error);
        }
    }
    
    /* Close the window after opening */
    gtk_window_close(win);
}

/* Open file's containing folder in file manager */
static void open_file(const char *filepath, GtkWindow *win) {
    GFile *file = g_file_new_for_path(filepath);
    
    GtkFileLauncher *launcher = gtk_file_launcher_new(file);
    gtk_file_launcher_open_containing_folder(launcher, win, NULL, on_file_launcher_finish, win);
    
    g_object_unref(file);
}

/* Handle row activation (click or Enter) */
static void on_row_activated(GtkListBox *list, GtkListBoxRow *row, gpointer user_data) {
    GtkWindow *win = GTK_WINDOW(user_data);
    
    /* Get the label from the row */
    GtkWidget *label = gtk_list_box_row_get_child(row);
    if (label && GTK_IS_LABEL(label)) {
        /* Get the plain text (without markup) */
        const char *filepath = gtk_label_get_text(GTK_LABEL(label));
        
        if (filepath && *filepath) {
            open_file(filepath, win);
        }
    }
}

static void update_results(GtkSearchEntry *entry, gpointer user_data) {
    GtkListBox *list = GTK_LIST_BOX(user_data);
    
    clear_list(list);

    const char *query = gtk_editable_get_text(GTK_EDITABLE(entry));

    if (query && *query) {
        char cmd[4096];
        /* Use fd to generate file list, pipe to fzf for fuzzy matching */
        snprintf(cmd, sizeof(cmd),
                 "fd --type f --hidden . \"%s\" | fzf -f \"%s\" | head -n 10",
                 search_root, query);

        FILE *fp = popen(cmd, "r");
        if (fp) {
            char line[1024];
            while (fgets(line, sizeof(line), fp)) {
                line[strcspn(line, "\n")] = 0;

                /* Create label with highlighted text */
                char *markup = create_highlighted_markup(line, query);
                
                GtkWidget *row = gtk_list_box_row_new();
                GtkWidget *label = gtk_label_new(line);  /* Store plain text */
                gtk_label_set_markup(GTK_LABEL(label), markup);  /* Display with markup */
                gtk_label_set_xalign(GTK_LABEL(label), 0.0);
                
                /* Enable ellipsization in the middle (creates /foo/../bar/file.ext effect) */
                gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_MIDDLE);
                
                /* Prevent label from expanding horizontally */
                gtk_label_set_max_width_chars(GTK_LABEL(label), 1);
                gtk_widget_set_hexpand(label, TRUE);
                
                /* Add padding to the label */
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
    }
}

static gboolean on_key(GtkEventControllerKey *controller,
                       guint keyval,
                       guint keycode,
                       GdkModifierType state,
                       gpointer user_data)
{
    if (keyval == GDK_KEY_Escape) {
        gtk_window_close(GTK_WINDOW(user_data));
        return TRUE;
    }
    return FALSE;
}

static void apply_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    
    const char *css = 
        "window {"
        "  border-radius: 4px;"
        "}"
        "entry {"
        "  margin: 8px 12px 4px 12px;"
        "}";
    
    gtk_css_provider_load_from_string(provider, css);
    
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    
    g_object_unref(provider);
}

static void activate(GtkApplication *app, gpointer user_data)
{
    GtkWindow *win = GTK_WINDOW(gtk_application_window_new(app));

    /* Calculate 50% width and 33% top margin */
    GdkDisplay *display = gdk_display_get_default();
    GListModel *monitors = gdk_display_get_monitors(display);
    if (g_list_model_get_n_items(monitors) > 0) {
        GdkMonitor *monitor = g_list_model_get_item(monitors, 0);
        GdkRectangle geo;
        gdk_monitor_get_geometry(monitor, &geo);
        cached_width = (int)(geo.width * 0.5);
        cached_margin_top = (int)(geo.height * 0.33);
    } else {
        cached_width = 600;
        cached_margin_top = 200;
    }

    gtk_layer_init_for_window(win);
    gtk_layer_set_layer(win, GTK_LAYER_SHELL_LAYER_TOP);
    gtk_layer_set_namespace(win, "file-search");
    gtk_layer_set_exclusive_zone(win, -1);
    gtk_layer_set_keyboard_mode(win, GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);

    /* Anchor to top but with margin to push it down 33% */
    gtk_layer_set_anchor(win, GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_layer_set_anchor(win, GTK_LAYER_SHELL_EDGE_LEFT, FALSE);
    gtk_layer_set_anchor(win, GTK_LAYER_SHELL_EDGE_RIGHT, FALSE);
    gtk_layer_set_anchor(win, GTK_LAYER_SHELL_EDGE_BOTTOM, FALSE);
    
    gtk_layer_set_margin(win, GTK_LAYER_SHELL_EDGE_TOP, cached_margin_top);

    /* Set the width immediately and prevent resizing */
    gtk_window_set_default_size(win, cached_width, -1);
    gtk_window_set_resizable(win, FALSE);

    GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_widget_set_vexpand(GTK_WIDGET(box), FALSE);
    gtk_widget_set_size_request(GTK_WIDGET(box), cached_width, -1);
    gtk_window_set_child(win, GTK_WIDGET(box));

    GtkSearchEntry *entry = GTK_SEARCH_ENTRY(gtk_search_entry_new());
    gtk_widget_set_hexpand(GTK_WIDGET(entry), TRUE);
    gtk_box_append(box, GTK_WIDGET(entry));

    /* Wrap listbox in a scrolled window for better sizing */
    GtkScrolledWindow *scroll = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new());
    gtk_scrolled_window_set_policy(scroll, 
                                   GTK_POLICY_NEVER, 
                                   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_max_content_height(scroll, 400);
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

    /* Connect row activation signal */
    g_signal_connect(list, "row-activated", G_CALLBACK(on_row_activated), win);

    g_signal_connect(entry, "search-changed",
                     G_CALLBACK(update_results), list);

    GtkEventController *controller = gtk_event_controller_key_new();
    gtk_event_controller_set_propagation_phase(controller, GTK_PHASE_CAPTURE);
    g_signal_connect(controller, "key-pressed", G_CALLBACK(on_key), win);
    gtk_widget_add_controller(GTK_WIDGET(win), controller);

    /* Apply CSS for rounded corners */
    apply_css();

    gtk_window_present(win);
}

int main(int argc, char **argv)
{
    if (argc > 1) {
        strncpy(search_root, argv[1], sizeof(search_root) - 1);
        search_root[sizeof(search_root) - 1] = '\0';
    }

    GtkApplication *app = gtk_application_new("file.search", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    int status = g_application_run(G_APPLICATION(app), 0, NULL);
    g_object_unref(app);

    return status;
}
