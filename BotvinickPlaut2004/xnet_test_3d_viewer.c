#include "xframe.h"

#include "lib_cairox.h"

static GtkWidget *net_3d_viewer_widget = NULL;
static cairo_surface_t *net_3d_viewer_surface = NULL;

/******************************************************************************/

static void draw_arrow(cairo_t *cr, int x0, int y0, int x1, int y1)
{
    CairoxArrowParameters params;
    CairoxPoint coordinates[2];

    coordinates[0].x = x0;
    coordinates[0].y = y0;
    coordinates[1].x = x1;
    coordinates[1].y = y1;

    cairox_arrow_parameters_set(&params, 1.0, LS_SOLID, VS_STRAIGHT, AH_SHARP, 1.0, FALSE);
    cairox_paint_arrow(cr, coordinates, 2, &params);
}

/******************************************************************************/
/* The 3D network state view drawing functions ********************************/

void draw_3d_vector(cairo_t *cr, double *vector, int width, int x, int y)
{
    double dx = 10.0 * cos(M_PI / 6.0);
    double dy = 10.0 * sin(M_PI / 6.0);
    int i;

    /* x and y are the centre of the vector ... apply offsets: */

    x = x - (width * 6.0 * cos(M_PI/6.0));
    y = y + (width * 6.0 * sin(M_PI/6.0));

    for (i = width; i > 0; i--) {
        double x0 = x + i * dx * 1.3;
        double y0 = y - i * dy * 1.3;
        double h = vector[i-1] * 50.0;

        if (xg.colour) {
            /* The bar's colour: */

            cairox_select_colour_scale(cr, 1.0 - vector[i-1]);
        }
        else {
            /* The bar's greyscale: */
            cairo_set_source_rgb(cr, (1.0 - vector[i-1]), (1.0 - vector[i-1]), (1.0 - vector[i-1]));
        }

        /* The shaded the bar: */
        cairo_move_to(cr, x0, y0);
        cairo_line_to(cr, x0-dx, y0-dy);
        cairo_line_to(cr, x0-dx, y0-dy-h);
        cairo_line_to(cr, x0, y0-h-2*dy);
        cairo_line_to(cr, x0+dx, y0-dy-h);
        cairo_line_to(cr, x0+dx, y0-dy);
        cairo_line_to(cr, x0, y0);
        cairo_fill(cr);

        /* The outline: */
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        /* The base: */
        cairox_paint_line(cr, NULL, x0, y0, x0-dx, y0-dy);
        cairox_paint_line(cr, NULL, x0, y0, x0+dx, y0-dy);

        /* The top: */
        cairox_paint_line(cr, NULL, x0, y0-h, x0-dx, y0-dy-h);
        cairox_paint_line(cr, NULL, x0, y0-h, x0+dx, y0-dy-h);
        cairox_paint_line(cr, NULL, x0-dx, y0-dy-h, x0, y0-2*dy-h);
        cairox_paint_line(cr, NULL, x0+dx, y0-dy-h, x0, y0-2*dy-h);

        /* The sides: */
        cairox_paint_line(cr, NULL, x0, y0, x0, y0-h);
        cairox_paint_line(cr, NULL, x0-dx, y0-dy, x0-dx, y0-dy-h);
        cairox_paint_line(cr, NULL, x0+dx, y0-dy, x0+dx, y0-dy-h);
    }
}

/******************************************************************************/

Boolean net_3d_viewer_expose(GtkWidget *widget, GdkEvent *event, void *data)
{
    if (!GTK_WIDGET_MAPPED(net_3d_viewer_widget) || (net_3d_viewer_surface == NULL)) {
        return(TRUE);
    }
    else {
        double in_vector[IN_WIDTH];
        double hidden_vector[HIDDEN_WIDTH];
        double out_vector[OUT_WIDTH];
        int height = net_3d_viewer_widget->allocation.height;
        int width  = net_3d_viewer_widget->allocation.width;
        char buffer[64], tmp[128];
        PangoLayout *layout;
        cairo_t *cr;
        CairoxTextParameters p;
        int x, y, dl, i;

        cr = cairo_create(net_3d_viewer_surface);
        layout = pango_cairo_create_layout(cr);
        pangox_layout_set_font_size(layout, 14);

        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_paint(cr);
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);

        /* Work out the inter-layer spacing: */

        dl = sqrt(height * height + width * width) / 6;

        /* The various vectors: */

        network_ask_input(xg.net, in_vector);
        x = width/2.0 - dl * cos(M_PI/6.0);
        y = height/2.0 + 25 - dl * sin(M_PI/6.0);
        draw_3d_vector(cr, in_vector, IN_WIDTH, x, y);

        network_ask_hidden(xg.net, hidden_vector);
        x = width/2.0;
        y = height/2.0 + 25;
        draw_3d_vector(cr, hidden_vector, HIDDEN_WIDTH, x, y);

        network_ask_output(xg.net, out_vector);
        x = width/2.0 + dl * cos(M_PI/6.0);
        y = height/2.0 + 25 + dl * sin(M_PI/6.0);
        draw_3d_vector(cr, out_vector, OUT_WIDTH, x, y);

        // Now arrows between layers:
        CairoxArrowParameters ap;
        CairoxPoint coordinates[6];

        cairox_arrow_parameters_set(&ap, 6.0, LS_SOLID, VS_CURVED, AH_SHARP, 0.5, FALSE);

        coordinates[0].x = width/2.0 - (dl-10) * cos(M_PI/6.0);
        coordinates[0].y = height/2.0 + 25 - (dl-10) * sin(M_PI/6.0);
        coordinates[1].x = width/2.0  - 20 * cos(M_PI/6.0);
        coordinates[1].y = height/2.0 + 25 - 20 * sin(M_PI/6.0);
        cairox_paint_arrow(cr, coordinates, 2, &ap);

        coordinates[0].x = width/2.0  + 10 * cos(M_PI/6.0);
        coordinates[0].y = height/2.0 + 25 + 10 * sin(M_PI/6.0);
        coordinates[1].x = width/2.0 + (dl-20) * cos(M_PI/6.0);
        coordinates[1].y = height/2.0 + 25 + (dl-20) * sin(M_PI/6.0);
        cairox_paint_arrow(cr, coordinates, 2, &ap);

        coordinates[0].x = width/2.0  + 10 * cos(M_PI/6.0);
        coordinates[0].y = height/2.0 + 25 + 10 * sin(M_PI/6.0);
        coordinates[1].x = width/2.0  + 40 * cos(M_PI/6.0);
        coordinates[1].y = height/2.0 + 25 + 40 * sin(M_PI/6.0);
        coordinates[2].x = width/2.0  + 40 * cos(M_PI/6.0) + 400 * cos(M_PI/6.0);
        coordinates[2].y = height/2.0 + 25 + 40 * sin(M_PI/6.0) - 400 * sin(M_PI/6.0);
        coordinates[3].x = width/2.0  - 80 * cos(M_PI/6.0) + 400 * cos(M_PI/6.0);
        coordinates[3].y = height/2.0 + 25 - 80 * sin(M_PI/6.0) - 400 * sin(M_PI/6.0);
        coordinates[4].x = width/2.0  - 80 * cos(M_PI/6.0);
        coordinates[4].y = height/2.0 + 25 - 80 * sin(M_PI/6.0);
        coordinates[5].x = width/2.0  - 20 * cos(M_PI/6.0);
        coordinates[5].y = height/2.0 + 25  - 20 * sin(M_PI/6.0);

        cairox_paint_arrow(cr, coordinates, 6, &ap);



        /* Write the fixated object at the top left: */

        if (world_decode_viewed(buffer, 64, in_vector)) {
            x = width/2.0 - (dl + 120 + 50) * cos(M_PI/6.0);
            y = height/2.0 - 25 - (dl - 120 + 50) * sin(M_PI/6.0);

            cairox_text_parameters_set(&p, x, y, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 30.0);
            cairox_paint_pango_text(cr, &p, layout, "Fixated:");
            cairox_text_parameters_set(&p, x, y+15, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 30.0);
            cairox_paint_pango_text(cr, &p, layout, buffer);

            // Draw a square bracket to show fixated units:
            double x0, y0, x1, y1, dx, dy;
            CairoxLineParameters lp;

            cairox_line_parameters_set(&lp, 2, LS_SOLID, TRUE);

            x = width/2.0 - (dl + 12.5*13 - 35) * cos(M_PI/6.0);
            y = height/2.0 - 25 - (dl - 12.5*13 - 35) * sin(M_PI/6.0) - 55;
            // 18 fixated units (plus width of 18th unit):
            x0 = x - (19 * 6.0 * cos(M_PI/6.0));
            y0 = y + (19 * 6.0 * sin(M_PI/6.0));
            x1 = x + (19 * 6.0 * cos(M_PI/6.0));
            y1 = y - (19 * 6.0 * sin(M_PI/6.0));

            dx = 6 * cos(M_PI/6.0);
            dy = 6 * sin(M_PI/6.0);

            cairox_paint_line(cr, &lp, x0, y0, x1, y1);
            cairox_paint_line(cr, &lp, x0+dx, y0+dy, x0, y0);
            cairox_paint_line(cr, &lp, x1+dx, y1+dy, x1, y1);
        }

        if (in_vector[18] > 0.99) { /* The coffee instruction */
            x = width/2.0 - (dl - 1.5*13 + 50) * cos(M_PI/6.0);
            y = height/2.0 - 25 - (dl + 1.5*13 + 50) * sin(M_PI/6.0);

            cairox_text_parameters_set(&p, x, y, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_CENTER, 30.0);
            cairox_paint_pango_text(cr, &p, layout, "Do Coffee");
            draw_arrow(cr, x+5*cos(M_PI/6.0), y+5*sin(M_PI/6.0), x+35*cos(M_PI/6.0), y+35*sin(M_PI/6.0));
        }

        if (in_vector[19] > 0.99) { /* The tea instruction */
            x = width/2.0 - (dl - 2.5*13 + 50) * cos(M_PI/6.0);
            y = height/2.0 - 25 - (dl + 2.5*13 + 50) * sin(M_PI/6.0);

            cairox_text_parameters_set(&p, x, y, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_CENTER, 30.0);
            cairox_paint_pango_text(cr, &p, layout, "Do Tea");
            draw_arrow(cr, x+5*cos(M_PI/6.0), y+5*sin(M_PI/6.0), x+35*cos(M_PI/6.0), y+35*sin(M_PI/6.0));
	}

        /* Write the held object at the middle left: */

        if (world_decode_held(buffer, 64, in_vector)) {
            x = width/2.0 - (dl - 120 + 50) * cos(M_PI/6.0);
            y = height/2.0 - 25 - (dl + 120 + 50) * sin(M_PI/6.0);

            cairox_text_parameters_set(&p, x, y, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 30.0);
            cairox_paint_pango_text(cr, &p, layout, "Held:");
            cairox_text_parameters_set(&p, x, y+15, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 30.0);
            cairox_paint_pango_text(cr, &p, layout, buffer);

            // Draw a square bracket to show fixated units:
            double x0, y0, x1, y1, dx, dy;
            CairoxLineParameters lp;

            cairox_line_parameters_set(&lp, 2, LS_SOLID, TRUE);

            x = width/2.0 - (dl - 8.0*13 - 35) * cos(M_PI/6.0);
            y = height/2.0 - 25 - (dl + 8.0*13 - 35) * sin(M_PI/6.0) - 55;
            // 19 held units (plus width of 19th unit):
            x0 = x - (20 * 6.0 * cos(M_PI/6.0));
            y0 = y + (20 * 6.0 * sin(M_PI/6.0));
            x1 = x + (20 * 6.0 * cos(M_PI/6.0));
            y1 = y - (20 * 6.0 * sin(M_PI/6.0));

            dx = 6 * cos(M_PI/6.0);
            dy = 6 * sin(M_PI/6.0);

            cairox_paint_line(cr, &lp, x0, y0, x1, y1);
            cairox_paint_line(cr, &lp, x0+dx, y0+dy, x0, y0);
            cairox_paint_line(cr, &lp, x1+dx, y1+dy, x1, y1);
        }

        /* Write the action in the bottom right: */

        if (world_decode_output_vector(buffer, 64, out_vector)) {
            i = (int) world_get_network_output_action(NULL, out_vector);

            x = width/2.0 + (dl + 30 + (i * 13 - 96)) * cos(M_PI/6.0);
            y = height/2.0 + 25 + (dl + 30 - (i * 13 - 96)) * sin(M_PI/6.0);

            cairox_text_parameters_set(&p, x+1, y+7, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
            cairox_paint_pango_text(cr, &p, layout, buffer);

            draw_arrow(cr, x, y, x-25*cos(M_PI/6.0), y-25*sin(M_PI/6.0));
        }

        /* Write the current cycle count and the output energy: */

        g_snprintf(buffer, 64, "Step: %3d", xg.step);
        cairox_text_parameters_set(&p, 5, height-5, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &p, layout, buffer);

        world_decode_output_vector(buffer, 64, out_vector);
        i = (int) world_get_network_output_action(NULL, out_vector);
        g_snprintf(tmp, 128, "Output winner: %s (%6.4f)", buffer, out_vector[i]);
        cairox_text_parameters_set(&p, width-250, height-35, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &p, layout, tmp);

        out_vector[i] = 0.0;
        world_decode_output_vector(buffer, 64, out_vector);
        i = (int) world_get_network_output_action(NULL, out_vector);
        g_snprintf(tmp, 128, "Output second: %s (%6.4f)", buffer, out_vector[i]);
        cairox_text_parameters_set(&p, width-250, height-20, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &p, layout, tmp);

        g_snprintf(buffer, 64, "Output conflict: %7.4f", vector_net_conflict(out_vector, OUT_WIDTH));
        cairox_text_parameters_set(&p, width-250, height-5, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &p, layout, buffer);

        g_object_unref(layout);
        cairo_destroy(cr);

        /* Now copy the surface to the window: */
        if ((net_3d_viewer_widget->window != NULL) && ((cr = gdk_cairo_create(net_3d_viewer_widget->window)) != NULL)) {
            cairo_set_source_surface(cr, net_3d_viewer_surface, 0, 0);
            cairo_paint(cr);
            cairo_destroy(cr);
        }
    }
    return(FALSE);
}

/******************************************************************************/

void net_3d_viewer_create(GtkWidget *vbox)
{
    net_3d_viewer_widget = gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(net_3d_viewer_widget), "expose_event", G_CALLBACK(net_3d_viewer_expose), NULL);
    g_signal_connect(G_OBJECT(net_3d_viewer_widget), "configure_event", G_CALLBACK(gtkx_configure_surface_callback), &net_3d_viewer_surface);
    g_signal_connect(G_OBJECT(net_3d_viewer_widget), "destroy", G_CALLBACK(gtkx_destroy_surface_callback), &net_3d_viewer_surface);
    gtk_widget_set_events(net_3d_viewer_widget, GDK_EXPOSURE_MASK);
    gtk_box_pack_start(GTK_BOX(vbox), net_3d_viewer_widget, TRUE, TRUE, 0);
    gtk_widget_show(net_3d_viewer_widget);
}

/******************************************************************************/
