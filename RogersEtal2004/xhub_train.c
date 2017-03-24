/*******************************************************************************

    File:       xhub_train.c
    Contents:   Train the network given the current parameters and pattern
                set and show the error on a graph
    Author:     Rick Cooper
    Copyright (c) 2016 Richard P. Cooper

    Public procedures:
        void hub_train_create_widgets(GtkWidget *page, XGlobals *xg)

*******************************************************************************/

/******** Include files: ******************************************************/

#include "xhub.h"
#include "lib_cairoxg_2_2.h"

/******************************************************************************/

static GtkWidget   *xhub_train_viewer = NULL;
static GraphStruct *xhub_train_graph = NULL;
static Boolean      xhub_train_colour = TRUE;
static Boolean      xhub_train_pause = FALSE;
static double       xhub_train_err_max = 1.0;
static int          xhub_train_graph_epoch_max = 100;
static int          xhub_train_cycles = 0;

/******************************************************************************/

static void xhub_train_viewer_paint_to_cairo(cairo_t *cr, int w, int h)
{
    if (xhub_train_graph != NULL) {
        graph_set_extent(xhub_train_graph, 0, 0, w, h);
        cairox_draw_graph(cr, xhub_train_graph, xhub_train_colour);
    }
}

/*----------------------------------------------------------------------------*/

static void xhub_train_repaint(XGlobals *xg)
{
    cairo_t *cr;
    double w, h;

    if (xhub_train_viewer != NULL) {
        w = xhub_train_viewer->allocation.width;
        h = xhub_train_viewer->allocation.height;
        cr = gdk_cairo_create(xhub_train_viewer->window);
        xhub_train_viewer_paint_to_cairo(cr, w, h);
        cairo_destroy(cr);
    }
}

/******************************************************************************/

static void xhub_train_network(XGlobals *xg, int steps)
{
    char buffer[16];
    xhub_train_pause = FALSE;
    while (!xhub_train_pause && steps-- > 0) {
        network_train(xg->net, xg->pattern_set);
        xhub_train_cycles++;
        xg->training_step++;
        if (xhub_train_graph->dataset[0].points < CXG_POINT_MAX) {
            xhub_train_graph->dataset[0].x[xhub_train_graph->dataset[0].points] = xhub_train_cycles;
            xhub_train_graph->dataset[0].y[xhub_train_graph->dataset[0].points] = network_test(xg->net, xg->pattern_set, EF_SUM_SQUARE);
            xhub_train_graph->dataset[0].se[xhub_train_graph->dataset[0].points] = 0.0;

            fprintf(stdout, "%4d: SSQ Error: %8.6f\n", xhub_train_cycles, xhub_train_graph->dataset[0].y[xhub_train_graph->dataset[0].points]);

            xhub_train_graph->dataset[0].points++;
            if (xhub_train_cycles > xhub_train_graph_epoch_max) {
                xhub_train_graph_epoch_max = xhub_train_graph_epoch_max * 10;
                graph_set_axis_properties(xhub_train_graph, GTK_ORIENTATION_HORIZONTAL, 0, xhub_train_graph_epoch_max, 11, "%4.0f", "Epochs");
            }
            xhub_train_repaint(xg);
        }
        else {
            fprintf(stdout, "%4d: SSQ Error: %8.6f\n", xhub_train_cycles, network_test(xg->net, xg->pattern_set, EF_SUM_SQUARE));
        }

        /* Update the interface counter: */
        g_snprintf(buffer, 16, "%d", xg->training_step);
        gtk_label_set_text(GTK_LABEL(xg->label_training_cycles), buffer);

        gtkx_flush_events();
    }
}

/******************************************************************************/

static void train_ensure_initialised(XGlobals *xg)
{
    if (xhub_train_graph->dataset[0].points == 0) {
        xhub_train_cycles = 0;
        xhub_train_graph->dataset[0].x[0] = xhub_train_cycles;
        xhub_train_graph->dataset[0].y[0] = network_test(xg->net, xg->pattern_set, EF_SUM_SQUARE);
        xhub_train_graph->dataset[0].se[0] = 0.0;
        xhub_train_graph->dataset[0].points++;
    }
}

/*----------------------------------------------------------------------------*/

static void reset_network_callback(GtkWidget *caller, XGlobals *xg)
{
    network_initialise_weights(xg->net);
    hub_explore_initialise_network(xg);
    gtk_label_set_text(GTK_LABEL(xg->label_weight_history), "Randomised");
    gtk_label_set_text(GTK_LABEL(xg->label_training_cycles), "0");
    xhub_train_graph->dataset[0].points = 0;
    xg->training_step = 0;
    train_ensure_initialised(xg);
    xhub_train_graph_epoch_max = 100;
    graph_set_axis_properties(xhub_train_graph, GTK_ORIENTATION_HORIZONTAL, 0, xhub_train_graph_epoch_max, 11, "%4.0f", "Epochs");
    xhub_train_repaint(xg);
}

/*----------------------------------------------------------------------------*/

static void train_callback_0001(GtkWidget *caller, XGlobals *xg)
{
    train_ensure_initialised(xg);
    xhub_train_network(xg, 1);
}

/*----------------------------------------------------------------------------*/

static void train_callback_0100(GtkWidget *caller, XGlobals *xg)
{
    train_ensure_initialised(xg);
    xhub_train_network(xg, 100);
}

/*----------------------------------------------------------------------------*/

static void train_callback_1000(GtkWidget *caller, XGlobals *xg)
{
    train_ensure_initialised(xg);
    xhub_train_network(xg, 1000);
}

/*----------------------------------------------------------------------------*/

static void pause_callback(GtkWidget *caller, XGlobals *xg)
{
    xhub_train_pause = TRUE;
    xhub_train_repaint(xg);
}

/*----------------------------------------------------------------------------*/

static void zoom_in_callback(GtkWidget *caller, XGlobals *xg)
{
    if (xhub_train_err_max > 0.99) {
        xhub_train_err_max = 0.5;
    }
    else if (xhub_train_err_max > 0.49) {
        xhub_train_err_max = 0.2;
    }
    else {
        xhub_train_err_max = 0.1;
    }

    graph_set_axis_properties(xhub_train_graph, GTK_ORIENTATION_VERTICAL, 0, xhub_train_err_max, 11, "%4.2f", "Error");
    xhub_train_repaint(xg);
}

/*----------------------------------------------------------------------------*/

static void zoom_out_callback(GtkWidget *caller, XGlobals *xg)
{
    if (xhub_train_err_max < 0.11) {
        xhub_train_err_max = 0.2;
    }
    else if (xhub_train_err_max < 0.21) {
        xhub_train_err_max = 0.5;
    }
    else {
        xhub_train_err_max = 1.0;
    }

    graph_set_axis_properties(xhub_train_graph, GTK_ORIENTATION_VERTICAL, 0, xhub_train_err_max, 11, "%4.2f", "Error");
    xhub_train_repaint(xg);
}

/*----------------------------------------------------------------------------*/

static void callback_toggle_colour(GtkWidget *caller, XGlobals *xg)
{
    xhub_train_colour = (Boolean) (GTK_TOGGLE_BUTTON(caller)->active);
    xhub_train_repaint(xg);
}

/*----------------------------------------------------------------------------*/

static void xhub_train_print_callback(GtkWidget *caller, XGlobals *xg)
{
    cairo_surface_t *surface = NULL;
    cairo_t *cr = NULL;
    char filename[128], prefix[64];
    int width  = xhub_train_viewer->allocation.width;
    int height = xhub_train_viewer->allocation.height;

    g_snprintf(prefix, 64, "training_%s", xg->pattern_set_name);

    /* PDF version: */
    g_snprintf(filename, 128, "FIGURES/%s.pdf", prefix);
    surface = cairo_pdf_surface_create(filename, width, height);
    cr = cairo_create(surface);
    xhub_train_viewer_paint_to_cairo(cr, width, height);
    cairo_destroy(cr);

    /* Save the PNG while we're at it: */
    g_snprintf(filename, 128, "FIGURES/%s.png", prefix);
    cairo_surface_write_to_png(surface, filename);
    /* And destroy the surface */
    cairo_surface_destroy(surface);
}

/******************************************************************************/

static void xhub_train_repaint_callback(GtkWidget *caller, XGlobals *xg)
{
    xhub_train_repaint(xg);
}

/*----------------------------------------------------------------------------*/

static void xhub_train_viewer_destroy(GtkWidget *caller, XGlobals *xg)
{
    graph_destroy(xhub_train_graph);
    xhub_train_graph = NULL;
    // Destroying viewer ...
    xhub_train_viewer = NULL;
}

/******************************************************************************/

void hub_train_create_widgets(GtkWidget *page, XGlobals *xg)
{
    GtkWidget *hbox, *tmp;

    graph_destroy(xhub_train_graph);
    if ((xhub_train_graph = graph_create(1)) != NULL) {
        CairoxFontProperties *fp;

        graph_set_margins(xhub_train_graph, 62, 26, 30, 45);
        graph_set_axis_properties(xhub_train_graph, GTK_ORIENTATION_VERTICAL, 0, xhub_train_err_max, 11, "%4.2f", "Error");
        graph_set_axis_properties(xhub_train_graph, GTK_ORIENTATION_HORIZONTAL, 0, xhub_train_graph_epoch_max, 11, "%4.0f", "Epochs");
        xhub_train_graph->dataset[0].points = 0;
        fp = font_properties_create("Sans", 16, PANGO_STYLE_NORMAL, PANGO_WEIGHT_NORMAL, "black");
        graph_set_title(xhub_train_graph, "Network Error as a Function of Training");
        graph_set_title_font_properties(xhub_train_graph, fp);
        font_properties_set_size(fp, 12);
        graph_set_axis_font_properties(xhub_train_graph, GTK_ORIENTATION_HORIZONTAL, fp);
        graph_set_axis_font_properties(xhub_train_graph, GTK_ORIENTATION_VERTICAL, fp);
        graph_set_legend_properties(xhub_train_graph, FALSE, 0.8, 0.1, NULL);
        graph_set_legend_font_properties(xhub_train_graph, fp);
        font_properties_destroy(fp);
    }

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    // Toolbar-like buttons
    gtkx_stock_image_button_create(hbox, GTK_STOCK_GOTO_FIRST, G_CALLBACK(reset_network_callback), xg);
    gtkx_stock_image_button_create(hbox, GTK_STOCK_GO_FORWARD, G_CALLBACK(train_callback_0001), xg);
    gtkx_stock_image_button_create(hbox, GTK_STOCK_JUMP_TO, G_CALLBACK(train_callback_0100), xg);
    gtkx_stock_image_button_create(hbox, GTK_STOCK_GOTO_LAST, G_CALLBACK(train_callback_1000), xg);
    gtkx_stock_image_button_create(hbox, GTK_STOCK_MEDIA_PAUSE, G_CALLBACK(pause_callback), xg);

    /*--- Filler: ---*/
    tmp = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 0);
    gtk_widget_show(tmp);

    /*--- Zoom in / out: ---*/
    gtkx_stock_image_button_create(hbox, GTK_STOCK_ZOOM_IN, G_CALLBACK(zoom_in_callback), xg);
    gtkx_stock_image_button_create(hbox, GTK_STOCK_ZOOM_OUT, G_CALLBACK(zoom_out_callback), xg);

    /*--- Filler: ---*/
    tmp = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 0);
    gtk_widget_show(tmp);

    /* Widgets for setting colour/greyscale: */
    /*--- The label: ---*/
    tmp = gtk_label_new("  Colour: ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
    /*--- The checkbox: ---*/
    tmp = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), xhub_train_colour);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(tmp), "toggled", G_CALLBACK(callback_toggle_colour), xg);
    gtk_widget_show(tmp);

    /*--- Spacer: ---*/
    tmp = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    /*--- And a print button: ---*/
    gtkx_stock_image_button_create(hbox, GTK_STOCK_PRINT, G_CALLBACK(xhub_train_print_callback), xg);

    /* Now the drawing area for the graph: */
    xhub_train_viewer = gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(xhub_train_viewer), "expose_event", G_CALLBACK(xhub_train_repaint_callback), xg);
    g_signal_connect(G_OBJECT(xhub_train_viewer), "destroy", G_CALLBACK(xhub_train_viewer_destroy), xg);
    gtk_box_pack_start(GTK_BOX(page), xhub_train_viewer, TRUE, TRUE, 0);
    gtk_widget_show(xhub_train_viewer);
}

/******************************************************************************/
