
#include "xframe.h"
#include "lib_cairox.h"

static GtkWidget *net_2d_viewer_widget = NULL;
static cairo_surface_t *net_2d_viewer_surface = NULL;

#define DX  7.0
#define DY 10.0

/******************************************************************************/
/* The 2D network state view drawing functions ********************************/

static void draw_2d_vector(cairo_t *cr, double *vector, int width, double x, double y)
{
    // The size of the rectangles for each unit: */
    int i;

    /* x and y are the centre of the vector ... apply offsets: */

    x = x - (width * 0.5 * DX);

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width(cr, 1.0);

    for (i = 0; i < width; i++) {
        double x0 = x + (i - 0.5) * DX;
        double y0 = y - 0.5 * DY;

        if (xg.colour) {
            /* The bar's colour: */
            cairox_select_colour_scale(cr, 1.0 - vector[i]);
        }
        else {
            /* The bar's greyscale: */
            cairo_set_source_rgb(cr, (1.0 - vector[i]), (1.0 - vector[i]), (1.0 - vector[i]));
        }

        /* The unit's rectangle: */
        cairo_rectangle(cr, x0, y0, DX, DY);
        cairo_fill_preserve(cr);
        /* The outline: */
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_stroke(cr);
    }
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
}

/******************************************************************************/

Boolean net_2d_viewer_expose(GtkWidget *widget, GdkEvent *event, void *data)
{
    if (!GTK_WIDGET_MAPPED(net_2d_viewer_widget) || (net_2d_viewer_surface == NULL)) {
        return(TRUE);
    }
    else {
        double in_vector[IN_WIDTH];
        double hidden_vector[HIDDEN_WIDTH];
        double out_vector[OUT_WIDTH];
        int height = net_2d_viewer_widget->allocation.height;
        int width  = net_2d_viewer_widget->allocation.width;
        char buffer[64], tmp[128];
        PangoLayout *layout;
        cairo_t *cr;
        CairoxTextParameters tp;
        CairoxArrowParameters ap;
        CairoxLineParameters lp;
        CairoxPoint coordinates[8]; 
        double x, y, dl;
        int i;

        cr = cairo_create(net_2d_viewer_surface);
        layout = pango_cairo_create_layout(cr);
        pangox_layout_set_font_size(layout, 12);

        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_paint(cr);
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);

        /* Work out the inter-layer spacing: */

        dl = height / 3;

        /* The various vectors: */

        network_ask_input(xg.net, in_vector);
        x = width/2.0;
        y = height/2.0 - dl;
        draw_2d_vector(cr, in_vector, IN_WIDTH, x, y);

        network_ask_hidden(xg.net, hidden_vector);
        x = width/2.0;
        y = height/2.0;
        draw_2d_vector(cr, hidden_vector, HIDDEN_WIDTH, x, y);

        network_ask_output(xg.net, out_vector);
        x = width/2.0;
        y = height/2.0 + dl;
        draw_2d_vector(cr, out_vector, OUT_WIDTH, x, y);

        /* Arrows to indicate network connectivity - Input to hidden: */

        coordinates[0].x = width/2.0;
        coordinates[0].y = height/2.0 - dl + DY;
        coordinates[1].x = width/2.0;
        coordinates[1].y = height/2.0 - DY;

        cairox_arrow_parameters_set(&ap, 3.0, LS_SOLID, VS_STRAIGHT, AH_SHARP, 1.0, FALSE);
        cairox_paint_arrow(cr, coordinates, 2, &ap);

        /* Arrows to indicate network connectivity - Hidden to output: */

        coordinates[0].x = width/2.0;
        coordinates[0].y = height/2.0 + DY;
        coordinates[1].x = width/2.0;
        coordinates[1].y = height/2.0 + dl - DY;

        cairox_arrow_parameters_set(&ap, 3.0, LS_SOLID, VS_STRAIGHT, AH_SHARP, 1.0, FALSE);
        cairox_paint_arrow(cr, coordinates, 2, &ap);

        /* Arrows to indicate network connectivity - Hidden to Hiden: */

        coordinates[0].x = width/2.0;
        coordinates[0].y = height/2.0 + DY;
        coordinates[1].x = width/2.0;
        coordinates[1].y = height/2.0 + DY + 60;
        coordinates[2].x = width/2.0 + 200;
        coordinates[2].y = height/2.0 + DY + 60;
        coordinates[3].x = width/2.0 + 200;
        coordinates[3].y = height/2.0 + DY;
        coordinates[4].x = width/2.0 + 200;
        coordinates[4].y = height/2.0 - DY;
        coordinates[5].x = width/2.0 + 200;
        coordinates[5].y = height/2.0 - DY - 60;
        coordinates[6].x = width/2.0;
        coordinates[6].y = height/2.0 - DY - 60;
        coordinates[7].x = width/2.0;
        coordinates[7].y = height/2.0 - DY;

        cairox_arrow_parameters_set(&ap, 3.0, LS_SOLID, VS_CURVED, AH_SHARP, 1.0, FALSE);
        cairox_paint_arrow(cr, coordinates, 8, &ap);

        /* Write the fixated object at the top left: */

        if (world_decode_viewed(buffer, 64, in_vector)) {
            x = width/2.0 - (IN_WIDTH / 2.0 * DX) + 8.5 * DX;
            y = height/2.0 - dl - 0.5 * DY;

            cairox_text_parameters_set(&tp, x, y-17, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, "Fixated Object:");
            cairox_text_parameters_set(&tp, x, y-5, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, buffer);

            cairox_line_parameters_set(&lp, 2.0, LS_SOLID, FALSE);
            cairox_paint_line(cr, &lp, x-8.5*DX, y-2.0, x+8.5*DX, y-2.0);
        }

        if (in_vector[18] > 0.99) { /* The coffee instruction */
            x = width/2.0 - (IN_WIDTH / 2.0 * DX) + 18 * DX - 1.0;
            y = height/2.0 - dl - 0.5 * DY;

            coordinates[0].x = x;
            coordinates[0].y = y-30;
            coordinates[1].x = x;
            coordinates[1].y = y-1;

            cairox_arrow_parameters_set(&ap, 1.0, LS_SOLID, VS_STRAIGHT, AH_SHARP, 1.0, FALSE);
            cairox_paint_arrow(cr, coordinates, 2, &ap);
            cairox_text_parameters_set(&tp, x, y-30, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, "Do Coffee");
        }

        if (in_vector[19] > 0.99) { /* The tea instruction */
            x = width/2.0 - (IN_WIDTH / 2.0 * DX) + 19 * DX - 1.0;
            y = height/2.0 - dl - 0.5 * DY;

            coordinates[0].x = x;
            coordinates[0].y = y-30;
            coordinates[1].x = x;
            coordinates[1].y = y-1;

            cairox_arrow_parameters_set(&ap, 1.0, LS_SOLID, VS_STRAIGHT, AH_SHARP, 1.0, FALSE);
            cairox_paint_arrow(cr, coordinates, 2, &ap);
            cairox_text_parameters_set(&tp, x, y-30, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, "Do Tea");
	}

        /* Write the held object at the middle left: */

        if (world_decode_held(buffer, 64, in_vector)) {
            x = width/2.0 - (IN_WIDTH / 2.0 * DX) + 29.0 * DX;
            y = height/2.0 - dl - 0.5 * DY;

            cairox_text_parameters_set(&tp, x, y-17, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, "Held:");
            cairox_text_parameters_set(&tp, x, y-5, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, buffer);

            cairox_line_parameters_set(&lp, 2.0, LS_SOLID, FALSE);
            cairox_paint_line(cr, &lp, x-9.0*DX, y-2.0, x+9.0*DX, y-2.0);
        }

        /* Write the action beneath the network: */

        if (world_decode_output_vector(buffer, 64, out_vector)) {
            i = (int) world_get_network_output_action(NULL, out_vector);
            x = width/2.0 - (OUT_WIDTH / 2.0 * DX) + i * DX - 1.0;
            y = height/2.0 + dl + 0.5 * DY;

            cairox_text_parameters_set(&tp, x, y+36, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, buffer);

            coordinates[0].x = x;
            coordinates[0].y = y+25;
            coordinates[1].x = x;
            coordinates[1].y = y+1;

            cairox_arrow_parameters_set(&ap, 1.0, LS_SOLID, VS_STRAIGHT, AH_SHARP, 1.0, FALSE);
            cairox_paint_arrow(cr, coordinates, 2, &ap);
        }

        /* Write the current cycle count and the output energy: */

        g_snprintf(buffer, 64, "Step: %3d", xg.step);
        cairox_text_parameters_set(&tp, 5, height-5, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);

        world_decode_output_vector(buffer, 64, out_vector);
        i = (int) world_get_network_output_action(NULL, out_vector);
        g_snprintf(tmp, 128, "Output winner: %s (%6.4f)", buffer, out_vector[i]);
        cairox_text_parameters_set(&tp, width-250, height-35, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, tmp);

        out_vector[i] = 0.0;
        world_decode_output_vector(buffer, 64, out_vector);
        i = (int) world_get_network_output_action(NULL, out_vector);
        g_snprintf(tmp, 128, "Output second: %s (%6.4f)", buffer, out_vector[i]);
        cairox_text_parameters_set(&tp, width-250, height-20, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, tmp);

        g_snprintf(buffer, 64, "Output conflict: %7.4f", vector_net_conflict(out_vector, OUT_WIDTH));
        cairox_text_parameters_set(&tp, width-250, height-5, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);

        g_object_unref(layout);
        cairo_destroy(cr);

        /* Now copy the surface to the window: */
        if ((net_2d_viewer_widget->window != NULL) && ((cr = gdk_cairo_create(net_2d_viewer_widget->window)) != NULL)) {
            cairo_set_source_surface(cr, net_2d_viewer_surface, 0, 0);
            cairo_paint(cr);
            cairo_destroy(cr);
        }
    }
    return(FALSE);
}

/******************************************************************************/

void net_2d_viewer_create(GtkWidget *vbox)
{
    net_2d_viewer_widget = gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(net_2d_viewer_widget), "expose_event", G_CALLBACK(net_2d_viewer_expose), NULL);
    g_signal_connect(G_OBJECT(net_2d_viewer_widget), "configure_event", G_CALLBACK(gtkx_configure_surface_callback), &net_2d_viewer_surface);
    g_signal_connect(G_OBJECT(net_2d_viewer_widget), "destroy", G_CALLBACK(gtkx_destroy_surface_callback), &net_2d_viewer_surface);
    gtk_widget_set_events(net_2d_viewer_widget, GDK_EXPOSURE_MASK);
    gtk_box_pack_start(GTK_BOX(vbox), net_2d_viewer_widget, TRUE, TRUE, 0);
    gtk_widget_show(net_2d_viewer_widget);
}

/******************************************************************************/
