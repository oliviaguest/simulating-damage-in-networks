#include "xframe.h"

#define X_MIN   25
#define X_MAX  -12
#define Y_MIN    5
#define Y_MAX  -30

#define GRAPH_STEPS     20000
#define GRAPH_STEP_SIZE 1

/******************************************************************************/
/* The error graph drawing functions ******************************************/

static GtkWidget *graph_widget = NULL;
static cairo_surface_t *graph_surface = NULL;

static double scale = 1.0;

static double            data1[GRAPH_STEPS];
static double            data2[GRAPH_STEPS];
static double            data3[GRAPH_STEPS];

static void draw_axes(GtkWidget *graph, cairo_t *cr, double x_scale, int x_range, double y_scale, double y_range)
{
    PangoLayout *layout = pango_cairo_create_layout(cr);
    CairoxTextParameters tp;
    CairoxLineParameters lp;
    int height = graph->allocation.height;
    int width = graph->allocation.width;
    char buffer[8];
    int dx, j;

    cairox_line_parameters_set(&lp, 1.0, LS_SOLID, FALSE);

    /* Draw x-axis labels and vertical grid lines */

    dx = (int) (10 * ((double) x_range / 100));
    pangox_layout_set_font_size(layout, 12);
    cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
    for (j = 0; j <= x_range; j += dx) {
        short x;

        g_snprintf(buffer, 8, "%d", j);
        x = ((short) (j * x_scale) + X_MIN);
        cairox_text_parameters_set(&tp, x, height + Y_MAX + 12, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_CENTER, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);
        cairox_paint_line(cr, &lp, x, Y_MIN - 2, x, height + Y_MAX + 1);
        cairox_paint_line(cr, &lp, x, Y_MIN - 2, x, height + Y_MAX + 1);
    }

    /* Draw y-axis labels and horizontal grid lines */

    for (j = 0; j <= 5; j++) {
        short y;

        y = (height - (short) (j * 0.2 * y_scale) + Y_MAX);
        sprintf(buffer, "%4.2f", j * y_range * 0.2);
        cairox_text_parameters_set(&tp, X_MIN - 5, y, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_CENTER, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);
        cairox_paint_line(cr, &lp, X_MIN - 2, y, width + X_MAX + 1, y);
    }

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairox_paint_line(cr, &lp, X_MIN, height + Y_MAX + 1, X_MIN, Y_MIN - 1);
    cairox_paint_line(cr, &lp, X_MIN - 1, height + Y_MAX, width + X_MAX + 1, height + Y_MAX);

    cairox_paint_line(cr, &lp, width + X_MAX, height + Y_MAX + 1, width + X_MAX, Y_MIN - 1);
    cairox_paint_line(cr, &lp, X_MIN - 1, Y_MIN, width + X_MAX + 1, Y_MIN);

    g_object_unref(layout);
}

static void draw_plot_line(cairo_t *cr, double *values, int colour, int time0, int count, double x_scale, double y_scale, int height)
{
    short x0, y0, x1, y1;

    if (time0 < count) {
        x0 = (short) (time0 * x_scale) + X_MIN;
        y0 = (short) (height - (values[time0] * y_scale)) + Y_MAX;

        if (colour == 2) { /* Red */
            cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
        }
        else if (colour == 4) { /* Blue */
            cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
        }
        else if (colour == 6) { /* Green */
            cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
	}        

        //      cairox_set_colour(cr, colour);
        while (time0++ < count) {
            x1 = (short) (time0 * x_scale) + X_MIN;
            y1 = (short) (height - values[time0] * y_scale) + Y_MAX;

            cairox_paint_line(cr, NULL, x0, y0, x1, y1);
            x0 = x1;
            y0 = y1;
        }
    }
}

static Boolean repaint_graph(GtkWidget *widget, GdkEventConfigure *event, XGlobals *xg)
{
    PangoLayout *layout;
    cairo_t *cr;
    CairoxTextParameters tp;
    double x_scale, y_scale;
    char buffer[30];
    int range, width, height, data_index;

    if (!GTK_WIDGET_MAPPED(graph_widget) || (graph_surface == NULL)) {
        return(TRUE);
    }
    else {
        width = graph_widget->allocation.width;
        height = graph_widget->allocation.height;

        cr = cairo_create(graph_surface);

        data_index = xg->training_step / GRAPH_STEP_SIZE;
        range = GRAPH_STEPS * GRAPH_STEP_SIZE;  // calculate_range(xg->training_step);
        x_scale = (width - (X_MIN - X_MAX)) / (double) range;
        y_scale = (height - (Y_MIN - Y_MAX));

        /* White background: */
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_paint(cr);

        draw_axes(graph_widget, cr, x_scale, range, y_scale, scale);
        draw_plot_line(cr, data1, 2, 0, data_index, x_scale * GRAPH_STEP_SIZE, y_scale / scale, height);
        draw_plot_line(cr, data2, 4, 0, data_index, x_scale * GRAPH_STEP_SIZE, y_scale / scale, height);
        draw_plot_line(cr, data3, 6, 0, data_index, x_scale * GRAPH_STEP_SIZE, y_scale / scale, height);

        /* Report current epoch / error: */    

        layout = pango_cairo_create_layout(cr);
        pangox_layout_set_font_size(layout, 12);

        g_snprintf(buffer, 30, "Cycle: %3d", xg->training_step);
        cairox_text_parameters_set(&tp, 5, height - 2, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_text_parameters_set_foreground(&tp, 0.0, 0.0, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);

        g_snprintf(buffer, 30, "SSQ Error: %7.5f", data1[data_index]);
        cairox_text_parameters_set(&tp, width*0.375, height - 2, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_text_parameters_set_foreground(&tp, 1.0, 0.0, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);

        g_snprintf(buffer, 30, "Cross Entropy: %7.5f", data2[data_index]);
        cairox_text_parameters_set(&tp, width*0.625, height - 2, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_text_parameters_set_foreground(&tp, 0.0, 0.0, 1.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);

        g_snprintf(buffer, 30, "RMS Error: %7.5f", data3[data_index]);
        cairox_text_parameters_set(&tp, width*0.875, height - 2, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_text_parameters_set_foreground(&tp, 0.0, 1.0, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);

        g_object_unref(layout);
        cairo_destroy(cr);

        /* Now copy the surface to the window: */
        if ((graph_widget->window != NULL) && ((cr = gdk_cairo_create(graph_widget->window)) != NULL)) {
            cairo_set_source_surface(cr, graph_surface, 0, 0);
            cairo_paint(cr);
            cairo_destroy(cr);
        }
        return (FALSE);
    }
}

/******************************************************************************/
/* Callbacks associated with the training notebook ****************************/

static void randomise_weights_callback(GtkWidget *button, XGlobals *xg)
{
    xg->pause = TRUE;
    network_initialise_weights(xg->net, xg->pars.wn);
    xgraph_initialise(xg);
}

static void reset_time_callback(GtkWidget *button, XGlobals *xg)
{
    xgraph_initialise(xg);
}

static void train_and_plot(XGlobals *xg, int cycles)
{
    xg->pause = FALSE;
    while ((cycles > 0) && (!xg->pause)) {
        if (xg->training_step >= GRAPH_STEPS * GRAPH_STEP_SIZE) {
            xg->pause = TRUE;
        }
        else {
            network_train(xg->net, &(xg->pars), xg->training_set);
            xg->training_step++;
            cycles--;
	}
        xgraph_set_error_scores(xg);
        gtkx_flush_events();
    }
}

static void train_callback_0001(GtkWidget *button, XGlobals *xg)
{
    train_and_plot(xg, 1);
}

static void train_callback_0100(GtkWidget *button, XGlobals *xg)
{
    train_and_plot(xg, 100);
}

static void train_callback_1000(GtkWidget *button, XGlobals *xg)
{
    train_and_plot(xg, 1000);
}

static void train_callback_5000(GtkWidget *button, XGlobals *xg)
{
    train_and_plot(xg, 5000);
}

static void train_callback_20000(GtkWidget *button, XGlobals *xg)
{
    train_and_plot(xg, 20000);
}

static void scale_up_callback(GtkWidget *button, XGlobals *xg)
{
    scale = scale / 2.0;
    repaint_graph(graph_widget, NULL, xg);
}

static void scale_down_callback(GtkWidget *button, XGlobals *xg)
{
    scale = scale * 2.0;
    repaint_graph(graph_widget, NULL, xg);
}

static void pause_callback(GtkWidget *button, XGlobals *xg)
{
    xg->pause = TRUE;
}

/******************************************************************************/

void xgraph_create(GtkWidget *page, XGlobals *xg)
{
    GtkWidget *toolbar;

    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_TEXT);
    gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(page), toolbar, FALSE, FALSE, 0);
    gtk_widget_show(toolbar);

    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Randomise", NULL, NULL, NULL, G_CALLBACK(randomise_weights_callback), xg);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Reset Time", NULL, NULL, NULL, G_CALLBACK(reset_time_callback), xg);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "1 Epoch", NULL, NULL, NULL, G_CALLBACK(train_callback_0001), xg);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "100 Epochs", NULL, NULL, NULL, G_CALLBACK(train_callback_0100), xg);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "1000 Epochs", NULL, NULL, NULL, G_CALLBACK(train_callback_1000), xg);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "5000 Epochs", NULL, NULL, NULL, G_CALLBACK(train_callback_5000), xg);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "20000 Epochs", NULL, NULL, NULL, G_CALLBACK(train_callback_20000), xg);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Pause", NULL, NULL, NULL, G_CALLBACK(pause_callback), xg);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), " + ", NULL, NULL, NULL, G_CALLBACK(scale_up_callback), xg);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), " - ", NULL, NULL, NULL, G_CALLBACK(scale_down_callback), xg);

    graph_widget = gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(graph_widget), "expose_event", G_CALLBACK(repaint_graph), xg);
    g_signal_connect(G_OBJECT(graph_widget), "configure_event", G_CALLBACK(gtkx_configure_surface_callback), &graph_surface);
    g_signal_connect(G_OBJECT(graph_widget), "destroy", G_CALLBACK(gtkx_destroy_surface_callback), &graph_surface);
//    g_signal_connect(G_OBJECT(graph), "expose_event", GTK_SIGNAL_FUNC(gtkx_viewer_expose), NULL);
//    g_signal_connect(G_OBJECT(graph), "configure_event", GTK_SIGNAL_FUNC(repaint_graph), xg);
    gtk_widget_set_events(graph_widget, GDK_EXPOSURE_MASK);
    gtk_box_pack_start(GTK_BOX(page), graph_widget, TRUE, TRUE, 0);
    gtk_widget_show(graph_widget);
}

void xgraph_set_error_scores(XGlobals *xg)
{
    char buffer[16];

    int i = (int) xg->training_step / GRAPH_STEP_SIZE;

    if (i * GRAPH_STEP_SIZE == xg->training_step) {
        data1[i] = sqrt(network_test(xg->net, xg->training_set, SUM_SQUARE_ERROR));
        data2[i] = network_test(xg->net, xg->training_set, CROSS_ENTROPY);
        data3[i] = 0; // network_test(xg->net, xg->training_set, SOFT_MAX_ERROR);
    }
    repaint_graph(graph_widget, NULL, xg);

    g_snprintf(buffer, 16, "%d", xg->training_step);
    gtk_label_set_text(GTK_LABEL(xg->weight_history_cycles), buffer);
}

void xgraph_initialise(XGlobals *xg)
{
    xgraph_set_error_scores(xg);
}

/******************************************************************************/
