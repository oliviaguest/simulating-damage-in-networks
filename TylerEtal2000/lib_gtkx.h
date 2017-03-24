/******************************************************************************/

#include <gtk/gtk.h>

extern void gtkx_server_release(GtkWidget *win);
extern void gtkx_server_grab(GtkWidget *win, GtkWidget *parent);
extern void gtkx_flush_events();
extern void gtkx_position_popup(GtkWidget *caller, GtkWidget *child);
extern void gtkx_warn(GtkWidget *parent, char *message);

extern GdkPixmap *gtkx_canvas_get_blank(GtkWidget *widget);
extern int gtkx_viewer_expose(GtkWidget *widget, GdkEventExpose *event, void *data);
extern GtkWidget *gtkx_stock_image_button_create(GtkWidget *box, const char *stock_id, void callback(), void *arg);
extern GtkWidget *gtkx_button_create(GtkWidget *box, const char *label, void callback(), void *arg);

extern void gtkx_configure_surface_callback(GtkWidget *widget, GdkEvent *event, cairo_surface_t **surface);
extern void gtkx_destroy_surface_callback(GtkWidget *widget, cairo_surface_t **surface);

extern GtkWidget *gtkx_toolbar_insert_space(GtkToolbar *toolbar, int expand, int position);
extern GtkWidget *gtkx_toolbar_insert_stock(GtkToolbar *toolbar, const char *stock_id, GtkTooltips *tips, const char *text, GCallback callback, void *data, int position);

/******************************************************************************/
