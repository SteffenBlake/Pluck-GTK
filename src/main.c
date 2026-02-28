/**
 * main.c — Application entry point for Pluck-GTK.
 *
 * Usage:
 *   pluck-gtk [search-root]
 *
 *   search-root  Optional path to the directory that `fd` will scan.
 *                Defaults to "." (current working directory).
 *
 * Pluck-GTK is a Wayland overlay file-search launcher built with GTK 4 and
 * gtk4-layer-shell.  It presents a floating search bar that pipes queries
 * through `fd` and `fzf`, then opens the selected file's containing folder
 * in the system file manager.
 */

#include "config.h"
#include "ui.h"

#include <gtk/gtk.h>
#include <string.h>

/* Define the global search-root buffer declared in config.h. */
char search_root[SEARCH_ROOT_MAX] = ".";

int main(int argc, char **argv)
{
    /* Override the default search root if the user supplied a path. */
    if (argc > 1) {
        strncpy(search_root, argv[1], SEARCH_ROOT_MAX - 1);
        search_root[SEARCH_ROOT_MAX - 1] = '\0';
    }

    GtkApplication *app = gtk_application_new("io.github.steffenblake.PluckGTK",
                                              G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    int status = g_application_run(G_APPLICATION(app), 0, NULL);
    g_object_unref(app);

    return status;
}
