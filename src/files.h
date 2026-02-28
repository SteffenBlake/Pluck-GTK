/**
 * files.h — File-system interaction helpers.
 *
 * Provides entry points for opening a file with its default application or,
 * as a fallback, revealing its containing folder in the system file manager.
 * Both operations use GtkFileLauncher (GTK 4.10+).
 */

#ifndef PLUCK_FILES_H
#define PLUCK_FILES_H

#include <gtk/gtk.h>

/**
 * open_file:
 * @filepath: Absolute or relative path to the file to open.
 * @win:      The application window.  It is closed automatically once the
 *            operation has been dispatched.
 *
 * Asynchronously tries to open @filepath with the desktop's default
 * application for that file type.  If no default application is registered,
 * the call falls back to revealing the file's parent directory in the file
 * manager (identical to open_containing_folder()).
 */
void open_file(const char *filepath, GtkWindow *win);

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
