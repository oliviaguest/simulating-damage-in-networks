#include "xframe.h"
#include "lib_cairox_2_0.h"

static GtkWidget *world_state_viewer_widget = NULL;
static cairo_surface_t *world_state_viewer_surface = NULL;

/******************************************************************************/
/* The text view of the state of the world drawing functions ******************/

Boolean world_state_viewer_expose(GtkWidget *widget, GdkEventConfigure *event, void *data)
{
    char template[10] = "xbp_XXXXX";
    FILE *fp;

    if (!GTK_WIDGET_MAPPED(world_state_viewer_widget)) {
        return(TRUE);
    }
    else if (world_state_viewer_surface == NULL) {
        return(TRUE);
    }
    else if ((mkstemp(template) >= 0) || ((fp = fopen(template, "w")) == NULL)) {
        return(TRUE);
    }
    else {
        PangoLayout *layout;
        cairo_t *cr;
        CairoxTextParameters p;
        char line[80];
        int y = 20;

        world_print_state(fp);
        fclose(fp);

        cr = cairo_create(world_state_viewer_surface);
        layout = pango_cairo_create_layout(cr);
        pangox_layout_set_font_size(layout, 12);

        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_paint(cr);
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);

        fp = fopen(template, "r");
        while (fgets(line, 80, fp) != NULL) {
            y += 14;
            cairox_text_parameters_set(&p,   5, y, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
            cairox_paint_pango_text(cr, &p, layout, line);
        }
        fclose(fp);
        remove(template);

        g_object_unref(layout);
        cairo_destroy(cr);

        /* Now copy the surface to the window: */
        if ((world_state_viewer_widget->window != NULL) && ((cr = gdk_cairo_create(world_state_viewer_widget->window)) != NULL)) {
            cairo_set_source_surface(cr, world_state_viewer_surface, 0, 0);
            cairo_paint(cr);
            cairo_destroy(cr);
        }
        return(FALSE);
    }
}

static void world_state_viewer_configure(GtkWidget *widget, GdkEventConfigure *event, void *data)
{
    int width = widget->allocation.width;
    int height = widget->allocation.height;

    if (world_state_viewer_surface != NULL) {
        cairo_surface_destroy(world_state_viewer_surface);
    }
    world_state_viewer_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
}

static void world_state_viewer_destroy(GtkWidget *widget, void *dummy)
{
    // Destroy the surface when we destroy the viewer:

    if (world_state_viewer_surface != NULL) {
        cairo_surface_destroy(world_state_viewer_surface);
        world_state_viewer_surface = NULL;
    }
}

/******************************************************************************/

void world_state_viewer_create(GtkWidget *vbox)
{
    world_state_viewer_widget = gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(world_state_viewer_widget), "expose_event", G_CALLBACK(world_state_viewer_expose), NULL);
    g_signal_connect(G_OBJECT(world_state_viewer_widget), "configure_event", G_CALLBACK(world_state_viewer_configure), NULL);
    g_signal_connect(G_OBJECT(world_state_viewer_widget), "destroy", G_CALLBACK(world_state_viewer_destroy), NULL);
    gtk_widget_set_events(world_state_viewer_widget, GDK_EXPOSURE_MASK);
    gtk_box_pack_start(GTK_BOX(vbox), world_state_viewer_widget, TRUE, TRUE, 0);
    gtk_widget_show(world_state_viewer_widget);
}

/******************************************************************************/
