/**
 * files.c — File-system interaction implementation.
 *
 * Uses GtkFileLauncher (available since GTK 4.10) to ask the desktop
 * environment's file manager to reveal the folder that contains the chosen
 * file, then closes the Pluck window.
 */

#include "files.h"

#include <gtk/gtk.h>

/* -------------------------------------------------------------------------
 * Internal helpers
 * ---------------------------------------------------------------------- */

/**
 * on_open_folder_finish:
 *
 * GAsyncReadyCallback invoked by GTK when the file-manager request
 * completes (or fails).  Logs a warning on failure and then closes the
 * application window in both cases.
 */
static void on_open_folder_finish(GObject      *source,
                                  GAsyncResult *result,
                                  gpointer      user_data)
{
    GtkWindow      *win      = GTK_WINDOW(user_data);
    GtkFileLauncher *launcher = GTK_FILE_LAUNCHER(source);
    GError         *error    = NULL;

    if (!gtk_file_launcher_open_containing_folder_finish(launcher, result, &error)) {
        if (error) {
            g_warning("Failed to open containing folder: %s", error->message);
            g_error_free(error);
        }
    }

    gtk_window_close(win);
}

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

void open_containing_folder(const char *filepath, GtkWindow *win)
{
    GFile           *file     = g_file_new_for_path(filepath);
    GtkFileLauncher *launcher = gtk_file_launcher_new(file);
    g_object_unref(file);

    gtk_file_launcher_open_containing_folder(launcher, win, NULL,
                                             on_open_folder_finish, win);
    /* Release our ref; the async machinery holds its own for the duration. */
    g_object_unref(launcher);
}
