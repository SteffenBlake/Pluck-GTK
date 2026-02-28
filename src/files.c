/**
 * files.c — File-system interaction implementation.
 *
 * Uses GtkFileLauncher (available since GTK 4.10) to open a file with its
 * default application.  If no default application is registered for the file
 * type the desktop's file manager is asked to reveal the containing folder
 * instead.  The Pluck window is closed once either action has been dispatched.
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

/**
 * on_launch_finish:
 *
 * GAsyncReadyCallback invoked by GTK when the attempt to open the file with
 * its default application completes.  On success the window is closed.  On
 * failure (e.g. no application registered for this file type) the launcher is
 * reused to fall back to revealing the containing folder instead.
 */
static void on_launch_finish(GObject      *source,
                              GAsyncResult *result,
                              gpointer      user_data)
{
    GtkWindow      *win      = GTK_WINDOW(user_data);
    GtkFileLauncher *launcher = GTK_FILE_LAUNCHER(source);
    GError         *error    = NULL;

    if (!gtk_file_launcher_launch_finish(launcher, result, &error)) {
        if (error) {
            g_warning("Failed to launch file with default application: %s", error->message);
            g_error_free(error);
        }
        /* Fall back to revealing the file in the system file manager. */
        gtk_file_launcher_open_containing_folder(launcher, win, NULL,
                                                 on_open_folder_finish, win);
        return;
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

void open_file(const char *filepath, GtkWindow *win)
{
    GFile           *file     = g_file_new_for_path(filepath);
    GtkFileLauncher *launcher = gtk_file_launcher_new(file);
    g_object_unref(file);

    gtk_file_launcher_launch(launcher, win, NULL, on_launch_finish, win);
    /* Release our ref; the async machinery holds its own for the duration. */
    g_object_unref(launcher);
}
