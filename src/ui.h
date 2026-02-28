/**
 * ui.h — GTK window construction and signal handlers.
 *
 * Exposes the single GtkApplication "activate" callback that builds and
 * presents the Pluck search overlay window.
 */

#ifndef PLUCK_UI_H
#define PLUCK_UI_H

#include <gtk/gtk.h>

/**
 * activate:
 * @app:       The running GtkApplication instance.
 * @user_data: Unused; present to satisfy the GCallback signature.
 *
 * Creates the Pluck overlay window, attaches it to the Wayland layer shell,
 * wires up all signal handlers, and calls gtk_window_present().
 *
 * Connect this to the GtkApplication "activate" signal before calling
 * g_application_run().
 */
void activate(GtkApplication *app, gpointer user_data);

#endif /* PLUCK_UI_H */
