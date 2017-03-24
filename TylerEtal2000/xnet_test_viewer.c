#include "xframe.h"

#include "lib_cairox.h"

static GtkWidget *net_viewer_widget = NULL;
static cairo_surface_t *net_viewer_surface = NULL;

/******************************************************************************/

static Boolean is_zero_vector(double *in_vector, int width)
{
    int i;

    for (i = 0; i < width; i++) {
        if (in_vector[i] != 0.0) {
            return(FALSE);
        }
    }
    return(TRUE);
}

static void draw_2d_vector(cairo_t *cr, double *vector, int width, XGlobals *xg, int x, int y)
{
    int unit_size = 12;
    int i;

    /* x and y are the centre of the vector: */

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width(cr, 1.0);

    for (i = 0; i < width; i++) {
        double x0 = x - (width * unit_size / 2) + (i - 0.5) * unit_size;
        double y0 = y - unit_size/2.0; 

        if (xg->colour) {
            /* The bar's colour: */
            cairox_select_colour_scale(cr, 1.0 - vector[i]);
        }
        else {
            /* The bar's greyscale: */
            cairo_set_source_rgb(cr, (1.0 - vector[i]), (1.0 - vector[i]), (1.0 - vector[i]));
        }

        /* The colour patch: */
        cairo_rectangle(cr, x0, y0, unit_size, unit_size);
        cairo_fill_preserve(cr);
        /* The outline: */
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_stroke(cr);
    }
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
}

/******************************************************************************/
/* The 3D network state view drawing functions ********************************/

void draw_3d_vector(cairo_t *cr, double *vector, int width, XGlobals *xg, int x, int y)
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

        if (xg->colour) {
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

static void repaint_2d_view(cairo_t *cr, PangoLayout *layout, double width, double height, XGlobals *xg)
{
    double in_vector[IO_WIDTH];
    double hidden_vector[HIDDEN_WIDTH];
    double out_vector[IO_WIDTH];
    CairoxArrowParameters params;
    CairoxPoint coordinates[6];
    CairoxTextParameters tp;
    PatternList *p = NULL;
    char buffer[128];
    int dl, x, y;

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);

    /* Work out the inter-layer spacing: */

    dl = sqrt(height * height + width * width) / 6;

    // ARROWS:
    coordinates[0].x = width/2.0;
    coordinates[0].y = height/2.0 - dl + 15;
    coordinates[1].x = width/2.0;
    coordinates[1].y = height/2.0 - 15;
    cairox_arrow_parameters_set(&params, 1.0, LS_SOLID, VS_STRAIGHT, AH_SHARP, 1.0, FALSE);
    cairox_paint_arrow(cr, coordinates, 2, &params);

    coordinates[0].x = width/2.0;
    coordinates[0].y = height/2.0 + 15;
    coordinates[1].x = width/2.0;
    coordinates[1].y = height/2.0 + dl - 15;
    cairox_arrow_parameters_set(&params, 1.0, LS_SOLID, VS_STRAIGHT, AH_SHARP, 1.0, FALSE);
    cairox_paint_arrow(cr, coordinates, 2, &params);

    if (xg->net->nt == NT_RECURRENT) {
        coordinates[0].x = width/2.0;
        coordinates[0].y = height/2.0 + 15;
        coordinates[1].x = width/2.0;
        coordinates[1].y = height/2.0 + 55;

        coordinates[2].x = width/2.0 + 200;
        coordinates[2].y = height/2.0 + 55;
        coordinates[3].x = width/2.0 + 200;
        coordinates[3].y = height/2.0 - 55;

        coordinates[4].x = width/2.0;
        coordinates[4].y = height/2.0 - 55;
        coordinates[5].x = width/2.0;
        coordinates[5].y = height/2.0 - 15;

        cairox_arrow_parameters_set(&params, 1.0, LS_SOLID, VS_STRAIGHT, AH_SHARP, 1.0, FALSE);
        cairox_paint_arrow(cr, coordinates, 6, &params);
    }

    network_ask_input(xg->net, in_vector);
    draw_2d_vector(cr, in_vector, IO_WIDTH, xg, width/2.0, height/2.0 - dl);
    network_ask_hidden(xg->net, hidden_vector);
    draw_2d_vector(cr, hidden_vector, HIDDEN_WIDTH, xg, width/2.0, height/2.0);
    network_ask_output(xg->net, out_vector);
    draw_2d_vector(cr, out_vector, IO_WIDTH, xg, width/2.0, height/2.0 + dl);

    /* Write the input at the top: */
    if (is_zero_vector(in_vector, IO_WIDTH)) {
        x = width/2.0;
        y = height/2.0 - dl - 10;

        g_snprintf(buffer, 128, "INPUT: ZERO");
        cairox_text_parameters_set(&tp, x, y, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);
    }
    else if ((p = world_decode_vector(xg->training_set, in_vector)) != NULL) {
        x = width/2.0;
        y = height/2.0 - dl - 10;

        g_snprintf(buffer, 128, "INPUT: %s (d = %7.3f)", p->name, euclidean_distance(IO_WIDTH, p->vector_in, in_vector));
        cairox_text_parameters_set(&tp, x, y, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);
    }

    /* Write the output in the bottom: */
    if ((p = world_decode_vector(xg->training_set, out_vector)) != NULL) {
        x = width/2.0;
        y = height/2.0 + dl + 10;

        cairox_text_parameters_set(&tp, x, y, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_TOP, 0.0);
        g_snprintf(buffer, 128, "OUTPUT: %s (d = %7.3f)", p->name, euclidean_distance(IO_WIDTH, p->vector_out, out_vector));
        cairox_paint_pango_text(cr, &tp, layout, buffer);
    }
}

static void repaint_3d_view(cairo_t *cr, PangoLayout *layout, double width, double height, XGlobals *xg)
{
    double in_vector[IO_WIDTH];
    double hidden_vector[HIDDEN_WIDTH];
    double out_vector[IO_WIDTH];
    CairoxArrowParameters params;
    CairoxPoint coordinates[6];
    PatternList *p = NULL;
    CairoxTextParameters tp;
    char buffer[128];
    int x, y, dl;

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);

    /* Work out the inter-layer spacing: */

    dl = sqrt(height * height + width * width) / 6;

    // ARROWS:
    coordinates[0].x = width/2.0 - (dl-20) * cos(M_PI/6.0);
    coordinates[0].y = height/2.0 + 25 - (dl-20) * sin(M_PI/6.0);
    coordinates[1].x = width/2.0 - 30 * cos(M_PI/6.0);
    coordinates[1].y = height/2.0 + 25 - 30 * sin(M_PI/6.0);
    cairox_arrow_parameters_set(&params, 1.0, LS_SOLID, VS_STRAIGHT, AH_SHARP, 1.0, FALSE);
    cairox_paint_arrow(cr, coordinates, 2, &params);

    coordinates[0].x = width/2.0 + 30 * cos(M_PI/6.0);
    coordinates[0].y = height/2.0 + 25 + 30 * sin(M_PI/6.0);
    coordinates[1].x = width/2.0 + (dl-30) * cos(M_PI/6.0);
    coordinates[1].y = height/2.0 + 25 + (dl-30) * sin(M_PI/6.0);
    cairox_arrow_parameters_set(&params, 1.0, LS_SOLID, VS_STRAIGHT, AH_SHARP, 1.0, FALSE);
    cairox_paint_arrow(cr, coordinates, 2, &params);

    if (xg->net->nt == NT_RECURRENT) {
        coordinates[0].x = width/2.0 + 30 * cos(M_PI/6.0);
        coordinates[0].y = height/2.0 + 25 + 30 * sin(M_PI/6.0);
        coordinates[1].x = width/2.0 + 60 * cos(M_PI/6.0);
        coordinates[1].y = height/2.0 + 25 + 60 * sin(M_PI/6.0);

        coordinates[2].x = width/2.0 + 60 * cos(M_PI/6.0) + 200 * cos(M_PI/6.0);
        coordinates[2].y = height/2.0 + 25 + 60 * sin(M_PI/6.0) - 200 * sin(M_PI/6.0);
        coordinates[3].x = width/2.0 - 70 * cos(M_PI/6.0) + 200 * cos(M_PI/6.0);
        coordinates[3].y = height/2.0 + 25 - 70 * sin(M_PI/6.0) - 200 * sin(M_PI/6.0);

        coordinates[4].x = width/2.0 - 70 * cos(M_PI/6.0);
        coordinates[4].y = height/2.0 + 25 - 70 * sin(M_PI/6.0);
        coordinates[5].x = width/2.0 - 30 * cos(M_PI/6.0);
        coordinates[5].y = height/2.0 + 25 - 30 * sin(M_PI/6.0);

        cairox_arrow_parameters_set(&params, 1.0, LS_SOLID, VS_STRAIGHT, AH_SHARP, 1.0, FALSE);
        cairox_paint_arrow(cr, coordinates, 6, &params);
    }

    /* The various vectors: */
    network_ask_input(xg->net, in_vector);
    x = width/2.0 - dl * cos(M_PI/6.0);
    y = height/2.0 + 25 - dl * sin(M_PI/6.0);
    draw_3d_vector(cr, in_vector, IO_WIDTH, xg, x, y);

    network_ask_hidden(xg->net, hidden_vector);
    x = width/2.0;
    y = height/2.0 + 25;
    draw_3d_vector(cr, hidden_vector, HIDDEN_WIDTH, xg, x, y);

    network_ask_output(xg->net, out_vector);
    x = width/2.0 + dl * cos(M_PI/6.0);
    y = height/2.0 + 25 + dl * sin(M_PI/6.0);
    draw_3d_vector(cr, out_vector, IO_WIDTH, xg, x, y);

    /* Write the input at the top left: */
    if (is_zero_vector(in_vector, IO_WIDTH)) {
        x = width/2.0 - 300 * cos(PI/6.0) - 80;
        y = height/2.0 - 300 * sin(PI/6.0);

        g_snprintf(buffer, 128, "INPUT: ZERO");
        cairox_text_parameters_set(&tp, x, y, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);
    }
    else if ((p = world_decode_vector(xg->training_set, in_vector)) != NULL) {
        x = width/2.0 - 300 * cos(PI/6.0) - 80;
        y = height/2.0 - 300 * sin(PI/6.0);

        g_snprintf(buffer, 128, "INPUT: %s (d = %7.3f)", p->name, euclidean_distance(IO_WIDTH, p->vector_in, in_vector));
        cairox_text_parameters_set(&tp, x, y, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);
    }

    /* Write the output in the bottom right: */
    if ((p = world_decode_vector(xg->training_set, out_vector)) != NULL) {
        x = width/2.0 + 300 * cos(PI/6.0) - 80;
        y = height/2.0 + 300 * sin(PI/6.0);

        cairox_text_parameters_set(&tp, x, y, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        g_snprintf(buffer, 128, "OUTPUT: %s (d = %7.3f)", p->name, euclidean_distance(IO_WIDTH, p->vector_out, out_vector));
        cairox_paint_pango_text(cr, &tp, layout, buffer);
    }
}

/*----------------------------------------------------------------------------*/

Boolean net_viewer_repaint(GtkWidget *widget, GdkEvent *event, XGlobals *xg)
{
    if (!GTK_WIDGET_MAPPED(net_viewer_widget) || (net_viewer_surface == NULL)) {
        return(TRUE);
    }
    else {
        int height = net_viewer_widget->allocation.height;
        int width  = net_viewer_widget->allocation.width;
        PangoLayout *layout;
        cairo_t *cr;
        CairoxTextParameters tp;
        char buffer[128];

        cr = cairo_create(net_viewer_surface);
        layout = pango_cairo_create_layout(cr);
        pangox_layout_set_font_size(layout, 12);

        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_paint(cr);

        if (xg->ddd) {
            /* 3D view: */
            repaint_3d_view(cr, layout, width, height, xg);
	}
        else {
            /* 2D view */
            repaint_2d_view(cr, layout, width, height, xg);
        }

        /* Write the current cycle count: */
        if (xg->net->nt == NT_RECURRENT) {
            if (xg->net->settled) {
                g_snprintf(buffer, 128, "Pattern: %3d (settled after %d cycles)", xg->pattern_num, xg->net->cycles);
	    }
            else {
                g_snprintf(buffer, 128, "Pattern: %3d (cycle %d)", xg->pattern_num, xg->net->cycles);
            }
        }
        else if (xg->net->nt == NT_FEEDFORWARD) {
            g_snprintf(buffer, 128, "Pattern: %3d", xg->pattern_num);
        }
        cairox_text_parameters_set(&tp, 5, height-5, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);

        g_object_unref(layout);
        cairo_destroy(cr);

        /* Now copy the surface to the window: */
        if ((net_viewer_widget->window != NULL) && ((cr = gdk_cairo_create(net_viewer_widget->window)) != NULL)) {
            cairo_set_source_surface(cr, net_viewer_surface, 0, 0);
            cairo_paint(cr);
            cairo_destroy(cr);
        }
    }
    return(FALSE);
}

/******************************************************************************/

void net_viewer_create(GtkWidget *vbox, XGlobals *xg)
{
    net_viewer_widget = gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(net_viewer_widget), "expose_event", G_CALLBACK(net_viewer_repaint), xg);
    g_signal_connect(G_OBJECT(net_viewer_widget), "configure_event", G_CALLBACK(gtkx_configure_surface_callback), &net_viewer_surface);
    g_signal_connect(G_OBJECT(net_viewer_widget), "destroy", G_CALLBACK(gtkx_destroy_surface_callback), &net_viewer_surface);
    gtk_widget_set_events(net_viewer_widget, GDK_EXPOSURE_MASK);
    gtk_box_pack_start(GTK_BOX(vbox), net_viewer_widget, TRUE, TRUE, 0);
    gtk_widget_show(net_viewer_widget);
}

/******************************************************************************/
