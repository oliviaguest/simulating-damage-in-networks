
#include "xframe.h"

static double lesion_severity = 0.0;
static double noise_sd = 0.00;

/******************************************************************************/
/* Functions for creating/maintaining the results of BP Simulation 1 **********/

static void record_output_vector(XGlobals *xg)
{
    if (xg->pattern_num < 100) {
        double *vector_out = (double *)malloc(IO_WIDTH * sizeof(double));
        int j;

        network_ask_output(xg->net, vector_out);
        for (j = 0; j < IO_WIDTH; j++) {
            xg->output[xg->pattern_num-1][j] = vector_out[j];
        }
        xg->conflict[xg->pattern_num-1] = net_conflict(xg->net);
        free(vector_out);
    }
}

static void show_net_output(XGlobals *xg)
{
    double *vector_in = (double *)malloc(IO_WIDTH * sizeof(double));

    world_set_network_input_vector(vector_in, xg->training_set, xg->pattern_num);
    network_tell_input(xg->net, vector_in);
    network_tell_randomise_hidden(xg->net);

    if (xg->net->nt == NT_RECURRENT) {
        do {
            network_tell_propagate(xg->net);
            net_viewer_repaint(NULL, NULL, xg);
            gtkx_flush_events();
            g_usleep(500000);
        } while ((!xg->net->settled) && (xg->net->cycles < 100));
    }
    else {
        network_tell_propagate(xg->net);
        net_viewer_repaint(NULL, NULL, xg);
    }
    record_output_vector(xg);
    free(vector_in);
}

/******************************************************************************/
/* Callbacks associated with the testing notebook *****************************/

static void test_first_callback(GtkWidget *button, XGlobals *xg)
{
    /* Select the first pattern and run the display function: */
    xg->pattern_num = 1;

    show_net_output(xg);
}

static void test_next_callback(GtkWidget *button, XGlobals *xg)
{
    /* Select the next pattern and run the display function: */

    if (++(xg->pattern_num) > pattern_list_length(xg->training_set)) {
        xg->pattern_num = 1;
    }

    show_net_output(xg);
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

static void train_net_callback(GtkWidget *mi, XGlobals *xg)
{
    network_initialise_weights(xg->net, xg->pars.wn);
    network_train_to_epochs(xg->net, &(xg->pars), xg->training_set);
}

static void lesion_net_callback(GtkWidget *mi, XGlobals *xg)
{
    lesion_ih_callback(mi, xg);
    if (xg->net->nt == NT_RECURRENT) {
        lesion_hh_callback(mi, xg);
    }
    lesion_ho_callback(mi, xg);
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
    net_viewer_repaint(NULL, NULL, xg);
}

static void callback_toggle_ddd(GtkWidget *button, XGlobals *xg)
{
    xg->ddd = (Boolean) (GTK_TOGGLE_BUTTON(button)->active);
    net_viewer_repaint(NULL, NULL, xg);
}

/******************************************************************************/

void test_net_create_widgets(GtkWidget *vbox, XGlobals *xg)
{
    /* The buttons for testing mode: */

    GtkWidget *tmp, *page, *hbox, *toolbar;
    int pos = 0;

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    /*--- Toolbar with run buttons: ---*/
    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
    gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(hbox), toolbar, TRUE, TRUE, 0);
    gtk_widget_show(toolbar);

    gtkx_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_GOTO_FIRST, NULL, NULL, G_CALLBACK(test_first_callback), xg, pos++);
    gtkx_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_GO_FORWARD, NULL, NULL, G_CALLBACK(test_next_callback), xg, pos++);

    /*--- Filler: ---*/
    tmp = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 5);
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

    /* Widgets for setting 2d/3d view: */
    /*--- The label: ---*/
    tmp = gtk_label_new("  3D: ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
    /*--- The checkbox: ---*/
    tmp = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), xg->ddd);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(tmp), "toggled", GTK_SIGNAL_FUNC(callback_toggle_ddd), xg);
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

    tmp = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);

    page = gtk_vbox_new(FALSE, 0);
    net_viewer_create(page, xg);
    gtk_box_pack_start(GTK_BOX(vbox), page, TRUE, TRUE, 0);
    gtk_widget_show(page);
}

/******************************************************************************/
