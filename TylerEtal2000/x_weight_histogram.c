
#include "xframe.h"

#include "lib_cairox.h"
#include "lib_cairoxg_2_2.h"

/* Actually its a polygon, not a histogram! */

static GtkWidget *histogram_viewer_widget = NULL;
static cairo_surface_t *histogram_viewer_surface = NULL;


static double lesion_severity = 0.0;
static double noise_sd = 0.00;

static GraphStruct *gd_ih = NULL;
static GraphStruct *gd_hh = NULL;
static GraphStruct *gd_ho = NULL;

Boolean histogram_viewer_repaint(GtkWidget *widget, GdkEvent *event, XGlobals *xg);

/******************************************************************************/
/* Callbacks associated with the testing notebook *****************************/

static void weight_histogram_print_callback(GtkWidget *button, XGlobals *xg)
{
    // FIXME: Save the histograms to a file
}

static void lesion_ih_callback(GtkWidget *mi, XGlobals *xg)
{
    network_perturb_weights_ih(xg->net, noise_sd);
    network_lesion_weights_ih(xg->net, lesion_severity);
}

static void lesion_hh_callback(GtkWidget *mi, XGlobals *xg)
{
    if (xg->net->nt != NT_RECURRENT) {
        gtkx_warn(xg->frame, "Cannot lesion HH connections in a non-recurrent network\nAction ignored");
    }
    else {
        network_perturb_weights_hh(xg->net, noise_sd);
        network_lesion_weights_hh(xg->net, lesion_severity);
    }
}

static void lesion_ho_callback(GtkWidget *mi, XGlobals *xg)
{
    network_perturb_weights_ho(xg->net, noise_sd);
    network_lesion_weights_ho(xg->net, lesion_severity);
}

static void randomise_net_callback(GtkWidget *mi, XGlobals *xg)
{
    network_initialise_weights(xg->net, xg->pars.wn);
    histogram_viewer_repaint(NULL, NULL, xg);
}

static void train_net_callback(GtkWidget *mi, XGlobals *xg)
{
    network_initialise_weights(xg->net, xg->pars.wn);
    network_train_to_epochs(xg->net, &(xg->pars), xg->training_set);
    histogram_viewer_repaint(NULL, NULL, xg);
}

static void lesion_net_callback(GtkWidget *mi, XGlobals *xg)
{
    lesion_ih_callback(mi, xg);
    if (xg->net->nt == NT_RECURRENT) {
        lesion_hh_callback(mi, xg);
    }
    lesion_ho_callback(mi, xg);
    histogram_viewer_repaint(NULL, NULL, xg);
}

static void callback_spin_lesion_level(GtkWidget *widget, GtkSpinButton *tmp)
{
    lesion_severity = gtk_spin_button_get_value_as_int(tmp) / 100.0;
}

static void callback_spin_noise_level(GtkWidget *widget, GtkSpinButton *tmp)
{
    noise_sd  = gtk_spin_button_get_value(tmp);
}

static void callback_toggle_colour(GtkWidget *button, XGlobals *xg)
{
    xg->colour = (Boolean) (GTK_TOGGLE_BUTTON(button)->active);
    histogram_viewer_repaint(NULL, NULL, xg);
}

#define BINS 11
#define BIN_MAX 0.1
#define BIN_MIN -0.1

static int get_bin_for_weight(double weight)
{
    if (weight < BIN_MIN) {
        return(0);
    }
    else if (weight > BIN_MAX) {
        return(BINS-1);
    }
    else {
        return(BINS * (weight - BIN_MIN) / (BIN_MAX - BIN_MIN));
    }
}

static void populate_histogram(GraphStruct *gd, double *weights, int w1, int w2)
{
    int count[BINS];
    double b_c[BINS];
    int i;

    for (i = 0; i < BINS; i++) {
        count[i] = 0;
        b_c[i] = BIN_MIN + (i * (BIN_MAX - BIN_MIN) / (double) (BINS-1));
    }

    for (i = 0; i < (w1 * w2); i++) {
        // Allocate each weight to a bin
        count[get_bin_for_weight(weights[i])]++;
    }

    for (i = 0; i < BINS; i++) {
        gd->dataset[0].x[i] = b_c[i];
        gd->dataset[0].y[i] = count[i] / (double) (w1 * w2);
    }
    gd->dataset[0].points = BINS;
}

Boolean histogram_viewer_repaint(GtkWidget *widget, GdkEvent *event, XGlobals *xg)
{
    if (!GTK_WIDGET_MAPPED(histogram_viewer_widget) || (histogram_viewer_surface == NULL)) {
        return(TRUE);
    }
    else {
        int height = histogram_viewer_widget->allocation.height;
        int width  = histogram_viewer_widget->allocation.width;
        PangoLayout *layout;
        cairo_t *cr;

        graph_set_extent(gd_ih, 0, 0, width/2, height/2);
        graph_set_extent(gd_ho, 0, height/2, width/2, height/2);
        graph_set_extent(gd_hh, width/2, height*0.25, width/2, height*0.5);

        cr = cairo_create(histogram_viewer_surface);
        layout = pango_cairo_create_layout(cr);
        pangox_layout_set_font_size(layout, 12);

        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_paint(cr);

        // The data
        populate_histogram(gd_ih, xg->net->weights_ih, xg->net->in_width+1, xg->net->hidden_width);
        cairox_draw_graph(cr, gd_ih, xg->colour);
        if (xg->net->nt == NT_RECURRENT) {
            populate_histogram(gd_hh, xg->net->weights_hh, xg->net->hidden_width, xg->net->hidden_width);
            cairox_draw_graph(cr, gd_hh, xg->colour);
        }
        populate_histogram(gd_ho, xg->net->weights_ho, xg->net->hidden_width+1, xg->net->out_width);
        cairox_draw_graph(cr, gd_ho, xg->colour);

        g_object_unref(layout);
        cairo_destroy(cr);

        /* Now copy the surface to the window: */
        if ((histogram_viewer_widget->window != NULL) && ((cr = gdk_cairo_create(histogram_viewer_widget->window)) != NULL)) {
            cairo_set_source_surface(cr, histogram_viewer_surface, 0, 0);
            cairo_paint(cr);
            cairo_destroy(cr);
        }
    }
    return(FALSE);
}


static void histogram_viewer_create(GtkWidget *vbox, XGlobals *xg)
{
    histogram_viewer_widget = gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(histogram_viewer_widget), "expose_event", G_CALLBACK(histogram_viewer_repaint), xg);
    g_signal_connect(G_OBJECT(histogram_viewer_widget), "configure_event", G_CALLBACK(gtkx_configure_surface_callback), &histogram_viewer_surface);
    g_signal_connect(G_OBJECT(histogram_viewer_widget), "destroy", G_CALLBACK(gtkx_destroy_surface_callback), &histogram_viewer_surface);
    gtk_widget_set_events(histogram_viewer_widget, GDK_EXPOSURE_MASK);
    gtk_box_pack_start(GTK_BOX(vbox), histogram_viewer_widget, TRUE, TRUE, 0);
    gtk_widget_show(histogram_viewer_widget);
}

/******************************************************************************/

void weight_histogram_create_widgets(GtkWidget *vbox, XGlobals *xg)
{
    /* The buttons for testing mode: */

    GtkWidget *tmp, *page, *hbox, *toolbar;
    int pos = 0;

    graph_destroy(gd_ih);
    gd_ih = graph_create(1);
    graph_destroy(gd_ho);
    gd_ho = graph_create(1);
    graph_destroy(gd_hh);
    gd_hh = graph_create(1);

    graph_set_margins(gd_ih, 52, 16, 30, 45);
    graph_set_margins(gd_ho, 52, 16, 30, 45);
    graph_set_margins(gd_hh, 52, 16, 30, 45);

    graph_set_title(gd_ih, "Input -> Hidden Weights");
    graph_set_axis_properties(gd_ih, GTK_ORIENTATION_HORIZONTAL, BIN_MIN, BIN_MAX, BINS, "%4.2f", "Weights");
    graph_set_axis_properties(gd_ih, GTK_ORIENTATION_VERTICAL, 0.0, 1.0, 11, "%3.1f", "Frequency");
    graph_set_dataset_properties(gd_ih, 0, "Weights", 0.5, 0.5, 0.5, 10, LS_SOLID, MARK_NONE);

    graph_set_title(gd_ho, "Hidden -> Output Weights");
    graph_set_axis_properties(gd_ho, GTK_ORIENTATION_HORIZONTAL, BIN_MIN, BIN_MAX, BINS, "%4.2f", "Weights");
    graph_set_axis_properties(gd_ho, GTK_ORIENTATION_VERTICAL, 0.0, 1.0, 11, "%3.1f", "Frequency");
    graph_set_dataset_properties(gd_ho, 0, "Weights", 0.5, 0.5, 0.5, 10, LS_SOLID, MARK_NONE);

    graph_set_title(gd_hh, "Hidden -> Hidden Weights");
    graph_set_axis_properties(gd_hh, GTK_ORIENTATION_HORIZONTAL, BIN_MIN, BIN_MAX, BINS, "%4.2f", "Weights");
    graph_set_axis_properties(gd_hh, GTK_ORIENTATION_VERTICAL, 0.0, 1.0, 11, "%3.1f", "Frequency");
    graph_set_dataset_properties(gd_hh, 0, "Weights", 0.5, 0.5, 0.5, 10, LS_SOLID, MARK_NONE);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    /*--- Train the network: ---*/
    gtkx_button_create(hbox, "Randomise", G_CALLBACK(randomise_net_callback), xg);
    tmp = gtk_label_new("  ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);

    /*--- Train the network: ---*/
    gtkx_button_create(hbox, "Train", G_CALLBACK(train_net_callback), xg);
    tmp = gtk_label_new("  ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);

    /* Widgets for adjusting the lesion severity: */
    /*--- The label: ---*/
    tmp = gtk_label_new("  Severity (%): ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
    /*--- The spin button: ---*/
    tmp = gtk_spin_button_new_with_range(0, 100, 1);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(tmp), 0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tmp), lesion_severity * 100);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(tmp), "value-changed", GTK_SIGNAL_FUNC(callback_spin_lesion_level), tmp);
    gtk_widget_show(tmp);

    /* Widgets for adjusting the lesion noise: */
    /*--- The label: ---*/
    tmp = gtk_label_new("  Noise (SD): ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
    /*--- The spin button: ---*/
    tmp = gtk_spin_button_new_with_range(0, 5, 0.1);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(tmp), 2);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tmp), noise_sd);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(tmp), "value-changed", GTK_SIGNAL_FUNC(callback_spin_noise_level), tmp);
    gtk_widget_show(tmp);

    /* Lesion the network: */
    tmp = gtk_label_new("  ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
    gtkx_button_create(hbox, "Lesion", G_CALLBACK(lesion_net_callback), xg);

    /*--- Filler: ---*/
    tmp = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 5);
    gtk_widget_show(tmp);

    /* Widgets for setting colour/greyscale: */
    /*--- The label: ---*/
    tmp = gtk_label_new("  Colour: ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
    /*--- The checkbox: ---*/
    tmp = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), xg->colour);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(tmp), "toggled", GTK_SIGNAL_FUNC(callback_toggle_colour), xg);
    gtk_widget_show(tmp);

    /*--- Filler: ---*/
    tmp = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 5);
    gtk_widget_show(tmp);

    /*--- Toolbar with print buttons: ---*/
    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
    gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(hbox), toolbar, TRUE, TRUE, 0);
    gtk_widget_show(toolbar);

    gtkx_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_PRINT, NULL, NULL, G_CALLBACK(weight_histogram_print_callback), xg, pos++);

    tmp = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);

    page = gtk_vbox_new(FALSE, 0);
    histogram_viewer_create(page, xg);
    gtk_box_pack_start(GTK_BOX(vbox), page, TRUE, TRUE, 0);
    gtk_widget_show(page);
}

/******************************************************************************/
