/******************************************************************************/

#include <gtk/gtk.h>

extern void gtkx_server_release(GtkWidget *win);
extern void gtkx_server_grab(GtkWidget *win, GtkWidget *parent);
extern void gtkx_flush_events();
extern void gtkx_position_popup(GtkWidget *caller, GtkWidget *child);
extern void gtkx_warn(GtkWidget *parent, char *message);

extern GtkWidget *gtkx_button_create(GtkWidget *parent, const char *label, void callback(), void *arg);

extern void gtkx_configure_surface_callback(GtkWidget *widget, GdkEvent *event, cairo_surface_t **surface);
extern void gtkx_destroy_surface_callback(GtkWidget *widget, cairo_surface_t **surface);


/******************************************************************************/
