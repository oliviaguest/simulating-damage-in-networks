#include "xframe.h"
#include "lib_cairox.h"

static GtkWidget *output_viewer_widget = NULL;
static cairo_surface_t *output_viewer_surface = NULL;

/******************************************************************************/
/* The 2D network state view drawing functions ********************************/

static void draw_2d_vector(cairo_t *cr, GtkWidget *widget, double *vector, int width, int x, int y)
{
    int i;

    /* x and y are the top left of the vector: */

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width(cr, 1);
    for (i = width; i > 0; i--) {
        double x0 = x + i * 12;
        double y0 = y; 

        if (xg.colour) {
            /* The bar's colour: */
            cairox_select_colour_scale(cr, 1.0 - vector[i-1]);
        }
        else {
            /* The bar's greyscale: */
            cairo_set_source_rgb(cr, (1.0 - vector[i-1]), (1.0 - vector[i-1]), (1.0 - vector[i-1]));
        }

        /* The colour patch: */
        cairo_rectangle(cr, x0, y0, 12, 12);
        cairo_fill_preserve(cr);
        /* The outline: */
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_stroke(cr);
    }
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
}

/******************************************************************************/

static void draw_vector_stats(cairo_t *cr, PangoLayout *layout, int step, int x)
{
    CairoxTextParameters p;
    char buffer[64], tmp[128];
    double v;
    int i, j;

    /* Write step counter: */
    g_snprintf(buffer, 64, "%3d", step+1);
    cairox_text_parameters_set(&p,  8, x+10, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
    cairox_paint_pango_text(cr, &p, layout, buffer);

    /* Write the winner: */
    world_decode_output_vector(buffer, 64, xg.output[step]);
    i = (int) world_get_network_output_action(NULL, xg.output[step]);
    g_snprintf(tmp, 128, "%s (%7.5f)", buffer, xg.output[step][i]);
    cairox_text_parameters_set(&p,  280, x+10, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
    cairox_paint_pango_text(cr, &p, layout, tmp);

    /* Write second choice: */
    v = xg.output[step][i];
    xg.output[step][i] = 0.0;
    world_decode_output_vector(buffer, 64, xg.output[step]);
    j = (int) world_get_network_output_action(NULL, xg.output[step]);
    g_snprintf(tmp, 128, "%s (%7.5f)", buffer, xg.output[step][j]);
    cairox_text_parameters_set(&p,  480, x+10, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
    cairox_paint_pango_text(cr, &p, layout, tmp);
    xg.output[step][i] = v;

    /* Write conflict measure: */
    g_snprintf(buffer, 64, "%7.5f", xg.conflict[step]);
    cairox_text_parameters_set(&p,  700, x+10, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
    cairox_paint_pango_text(cr, &p, layout, buffer);
}

/******************************************************************************/

Boolean net_output_viewer_expose(GtkWidget *widget, GdkEventExpose *event, void *data)
{
    if (!GTK_WIDGET_MAPPED(output_viewer_widget)) {
        return(TRUE);
    }
    else if (output_viewer_surface == NULL) {
        return(TRUE);
    }
    else {
        PangoLayout *layout;
        cairo_t *cr;
        CairoxTextParameters p;
        int i;

        cr = cairo_create(output_viewer_surface);
        layout = pango_cairo_create_layout(cr);
        pangox_layout_set_font_size(layout, 12);

        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_paint(cr);
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);

        /* Headings: */

        cairox_text_parameters_set(&p,   5, 20, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &p, layout, "Step");
        cairox_text_parameters_set(&p,  50, 20, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &p, layout, "Output Vector");
        cairox_text_parameters_set(&p,  300, 20, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &p, layout, "Winner");
        cairox_text_parameters_set(&p,  500, 20, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &p, layout, "Second");
        cairox_text_parameters_set(&p,  700, 20, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &p, layout, "Conflict");

        for (i = 0; (i < 100) && (i < xg.step); i++) {
            draw_2d_vector(cr, output_viewer_widget, xg.output[i], OUT_WIDTH, 20, 30+i*14);
            draw_vector_stats(cr, layout, i, 30+i*14);
        }

        g_object_unref(layout);
        cairo_destroy(cr);

        /* Now copy the surface to the window: */
        if ((output_viewer_widget->window != NULL) && ((cr = gdk_cairo_create(output_viewer_widget->window)) != NULL)) {
            cairo_set_source_surface(cr, output_viewer_surface, 0, 0);
            cairo_paint(cr);
            cairo_destroy(cr);
        }
        return(FALSE);
    }
}

static void net_output_viewer_configure(GtkWidget *widget, GdkEventConfigure *event, void *data)
{
    int width = widget->allocation.width;
    int height = widget->allocation.height;

    if (output_viewer_surface != NULL) {
        cairo_surface_destroy(output_viewer_surface);
    }
    output_viewer_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
}

static void net_output_viewer_destroy(GtkWidget *widget, void *dummy)
{
    // Destroy the surface when we destroy the viewer:

    if (output_viewer_surface != NULL) {
        cairo_surface_destroy(output_viewer_surface);
        output_viewer_surface = NULL;
    }
}

/******************************************************************************/

void net_output_viewer_create(GtkWidget *vbox)
{
    output_viewer_widget = gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(output_viewer_widget), "expose_event", G_CALLBACK(net_output_viewer_expose), NULL);
    g_signal_connect(G_OBJECT(output_viewer_widget), "configure_event", G_CALLBACK(net_output_viewer_configure), NULL);
    g_signal_connect(G_OBJECT(output_viewer_widget), "destroy", G_CALLBACK(net_output_viewer_destroy), NULL);
    gtk_widget_set_events(output_viewer_widget, GDK_EXPOSURE_MASK);
    gtk_box_pack_start(GTK_BOX(vbox), output_viewer_widget, TRUE, TRUE, 0);
    gtk_widget_show(output_viewer_widget);
}

/******************************************************************************/
