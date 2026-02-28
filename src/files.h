/**
 * files.h — File-system interaction helpers.
 *
 * Provides a single entry point for opening a file's containing folder in
 * the system file manager via GtkFileLauncher (GTK 4.10+).
 */

#ifndef PLUCK_FILES_H
#define PLUCK_FILES_H

#include <gtk/gtk.h>

/**
 * open_containing_folder:
 * @filepath: Absolute or relative path to the file whose parent directory
 *            should be revealed.
 * @win:      The application window.  It is closed automatically once the
 *            file-manager request has been dispatched.
 *
 * Asynchronously asks the desktop's default file manager to open the folder
 * that contains @filepath.  After the request is dispatched (success or
 * failure) @win is closed.
 */
void open_containing_folder(const char *filepath, GtkWindow *win);

#endif /* PLUCK_FILES_H */
