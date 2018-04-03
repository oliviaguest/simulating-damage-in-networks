
#include "xframe.h"

/******************************************************************************/
/* Callback to exit/kill the graphics interface *******************************/

static void exit_bp(GtkWidget *button, XGlobals *xg)
{
    training_set_free(xg->training_set);
    network_destroy(xg->net);
    xg->net = NULL;
    gtk_main_quit();
}

/******************************************************************************/

void initialise_widgets(XGlobals *xg)
{
    /* Initialise the error measures: */
    xg->training_step = 0;
    xgraph_initialise(xg);
    /* Clear/redraw the state viewers: */
    net_viewer_repaint(NULL, NULL, xg);
}

/******************************************************************************/
/* Callback to load a set of weights ******************************************/

static void destroy_chooser(GtkWidget *button, GtkWidget *chooser)
{
    gtk_widget_destroy(chooser);
}

static void throw_warning(const char *file, int line, const char *func, const char *reason, int w)
{
    fprintf(stdout, "Warning: %s: %d %s: %s %d\n", file, line, func, reason, w);
}

static void select_og_weight_file(GtkWidget *w, GtkWidget *chooser)
{
    char *filename = (char *)gtk_file_selection_get_filename(GTK_FILE_SELECTION(chooser));
    XGlobals *xg = g_object_get_data(G_OBJECT(chooser), "globals");
    int tyler_iterations_so_far, p_min, p_max;
    char buffer[128];
    double weight, bias, error;
    int i, j;
    FILE *fp;

    if ((fp = fopen(filename, "r")) == NULL) {
        throw_warning(__FILE__,  __LINE__, __FUNCTION__, "Could not open tyler weight file", 0);
    }
    else if (!fscanf(fp, " %i", &tyler_iterations_so_far)) {
        throw_warning(__FILE__,  __LINE__, __FUNCTION__, "Could not read variable: tyler_iterations_so_far", 0);
    }
    else if (!fscanf(fp, " %i %i", &p_min, &p_max)) {
        throw_warning(__FILE__,  __LINE__, __FUNCTION__, "Could not read variables: p_min and p_max", 0);
    }
    else {
        // Weights from hidden units to output units:
        for (i = 0; i < xg->net->hidden_width; i++) {
            for (j = 0; j < xg->net->out_width; j++) {
                if (!fscanf(fp, " %lf", &weight)) {
                    throw_warning(__FILE__,  __LINE__, __FUNCTION__, "Could not read variable: weight_hidden_out[][]", 0);
                }
                else {
                    xg->net->weights_ho[i * xg->net->out_width + j] = weight;
                }
            }
        }

        // Weights from input units to hidden units:
        for (i = 0; i < xg->net->in_width; i++) {
            for (j = 0; j < xg->net->hidden_width; j++) {
                if (!fscanf(fp, " %lf", &weight)) {
                    throw_warning(__FILE__,  __LINE__, __FUNCTION__, "Could not read variable: weight_in_hidden[][]", 0);
                }
                else {
                    xg->net->weights_ih[i * xg->net->hidden_width + j] = weight;
                }
            }
        }

        // Bias on hidden units
        for (i = 0; i < HIDDEN_WIDTH; i++) {
            if (!fscanf(fp, " %lf", &bias)) {
                throw_warning(__FILE__,  __LINE__, __FUNCTION__, "Could not read variable: bias_hidden[]", 0);
            }
            else {
                xg->net->weights_ih[xg->net->in_width * xg->net->hidden_width + i] = bias; // was -bias
            }
        }

        // Bias on output units
        for (j = 0; j < xg->net->out_width; j++) {
            if (!fscanf(fp, " %lf", &bias)) {
                throw_warning(__FILE__,  __LINE__, __FUNCTION__, "Could not read variable: bias_out[]", 0);
            }
            else {
                xg->net->weights_ho[xg->net->hidden_width * xg->net->out_width + j] = bias; // was -bias
            }
        }

        for (i = 0; i < tyler_iterations_so_far; i++) {
            if (!fscanf(fp, " %lf", &error)) {
                throw_warning(__FILE__,  __LINE__, __FUNCTION__, "Could not read variable: Total_Error[]", 0);
            }
        }

        char *buf = malloc(sizeof(char *) * 4);;
        char *compare = "EOF\0";

        if (!fscanf(fp, " %s", buf)) {
            throw_warning(__FILE__,  __LINE__, __FUNCTION__, "Could not reach end of file", 0);
        }

        if(strncmp(buf, compare, 3) != 0) {
            throw_warning(__FILE__,  __LINE__, __FUNCTION__, "Could not reach end of file", 0);
        }

        gtk_label_set_text(GTK_LABEL(xg->weight_history_label), filename);
        g_snprintf(buffer, 128, "Weights successfully restored from\n%s", filename);

        fclose(fp);
    }

    gtk_widget_destroy(chooser);
}

static void select_rc_weight_file(GtkWidget *w, GtkWidget *chooser)
{
    char *filename = (char *)gtk_file_selection_get_filename(GTK_FILE_SELECTION(chooser));
    XGlobals *xg = g_object_get_data(G_OBJECT(chooser), "globals");
    char buffer[128];
    FILE *fp;
    int j;

    network_destroy(xg->net);
    xg->net = NULL;

    if ((fp = fopen(filename, "r")) == NULL) {
        g_snprintf(buffer, 128, "ERROR: Cannot read file ... weights not restored");
    }
    else if ((xg->net = network_read_weights_from_file(fp, &j)) == NULL) {
        g_snprintf(buffer, 128, "ERROR: Weight file format error %d ... weights not restored", j);
        fclose(fp);
    }
    else {
        gtk_label_set_text(GTK_LABEL(xg->weight_history_label), filename);
        g_snprintf(buffer, 128, "Weights successfully restored from\n%s", filename);

        /* Initialise the graph and various viewers: */
        xgraph_set_error_scores(xg);
        initialise_widgets(xg);

        fclose(fp);
    }
    gtkx_warn(chooser, buffer);
    gtk_widget_destroy(chooser);
}

static void load_weights_rc_callback(GtkWidget *mi, XGlobals *xg)
{
    GtkWidget *chooser = gtk_file_selection_new("Select weight file");
    g_object_set_data(G_OBJECT(chooser), "globals", xg);
    gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(chooser));
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(chooser)->ok_button), "clicked", GTK_SIGNAL_FUNC(select_rc_weight_file), chooser);
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(chooser)->cancel_button), "clicked", GTK_SIGNAL_FUNC(destroy_chooser), chooser);
    gtkx_position_popup(xg->frame, chooser);
    gtk_widget_show(chooser);
}

static void load_weights_og_callback(GtkWidget *mi, XGlobals *xg)
{
    GtkWidget *chooser = gtk_file_selection_new("Select weight file");
    g_object_set_data(G_OBJECT(chooser), "globals", xg);
    gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(chooser));
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(chooser)->ok_button), "clicked", GTK_SIGNAL_FUNC(select_og_weight_file), chooser);
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(chooser)->cancel_button), "clicked", GTK_SIGNAL_FUNC(destroy_chooser), chooser);
    gtkx_position_popup(xg->frame, chooser);
    gtk_widget_show(chooser);
}

/******************************************************************************/
/* Callback to save a set of weights ******************************************/

static Boolean weight_file_dump_header_info(FILE *fp, char *training_data, XGlobals *xg)
{
    fprintf(fp, "%% Training set: %s\n", training_data);
    fprintf(fp, "%% Learning rate: %7.5f\n", xg->pars.lr);
    fprintf(fp, "%% Weight decay: %7.5f\n", xg->pars.wd);
    fprintf(fp, "%% Momentum: %7.5f\n", xg->pars.momentum);
    fprintf(fp, "%% Update time: %s\n", (xg->pars.wut == UPDATE_BY_ITEM) ? "By item" : "By epoch");
    fprintf(fp, "%% Epochs: %d\n", xg->training_step);
    return(TRUE);
}

static void save_weights_to_file(GtkWidget *w, GtkWidget *chooser)
{
    char *file = (char *)gtk_file_selection_get_filename(GTK_FILE_SELECTION(chooser));
    XGlobals *xg = g_object_get_data(G_OBJECT(chooser), "globals");
    char buffer[128];
    FILE *fp;

    if ((fp = fopen(file, "w")) == NULL) {
        g_snprintf(buffer, 128, "ERROR: Cannot write to %s", file);
    }
    else if (!weight_file_dump_header_info(fp, TRAINING_PATTERNS, xg)) {
        g_snprintf(buffer, 128, "ERROR: Cannot dump weight header information ... weights not saved");
        fclose(fp);
        remove(file);
    }
    else if (!network_dump_weights(xg->net, fp)) {
        g_snprintf(buffer, 128, "ERROR: Problem dumping weights ... weights not saved");
        fclose(fp);
        remove(file);
    }
    else {
        g_snprintf(buffer, 128, "Weights successfully saved to\n%s", file);
        fclose(fp);
    }
    gtkx_warn(chooser, buffer);
    gtk_widget_destroy(chooser);
}

static void save_weights_callback(GtkWidget *mi, XGlobals *xg)
{
    GtkWidget *chooser = gtk_file_selection_new("Save weight file");
    g_object_set_data(G_OBJECT(chooser), "globals", xg);
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(chooser)->ok_button), "clicked", GTK_SIGNAL_FUNC(save_weights_to_file), chooser);
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(chooser)->cancel_button), "clicked", GTK_SIGNAL_FUNC(destroy_chooser), chooser);
    gtkx_position_popup(xg->frame, chooser);
    gtk_widget_show(chooser);
}

/******************************************************************************/

static void initialise_weights_callback(GtkWidget *dummy, XGlobals *xg)
{
    /* Initialise the network: */
    xg->training_step = 0;
    gtk_label_set_text(GTK_LABEL(xg->weight_history_label), "Randomised");
    network_destroy(xg->net);
    xg->net = network_initialise(xg->pars.nt, IO_WIDTH, HIDDEN_WIDTH, IO_WIDTH);
    network_initialise_weights(xg->net, xg->pars.wn);
    /* Initialise the graph and various viewers: */
    initialise_widgets(xg);
}

/******************************************************************************/
/* Callback to load a set of patterns/sequences *******************************/

static PatternList *xtraining_set_read(XGlobals *xg, char *file, int in_width, int out_width)
{
    PatternList *tmp;

    if ((tmp = training_set_read(file, in_width, out_width)) != NULL) {
        gtk_label_set_text(GTK_LABEL(xg->training_set_label), file);
    }
    else {
        gtk_label_set_text(GTK_LABEL(xg->training_set_label), "NONE");
    }
    return(tmp);
}

int reload_training_data(XGlobals *xg, char *file)
{
    training_set_free(xg->training_set);
    xg->training_set = xtraining_set_read(xg, file, IO_WIDTH, IO_WIDTH);
    return(pattern_list_length(xg->training_set));
}

static void select_training_file(GtkWidget *w, GtkWidget *chooser)
{
    char *file = (char *)gtk_file_selection_get_filename(GTK_FILE_SELECTION(chooser));
    XGlobals *xg = g_object_get_data(G_OBJECT(chooser), "globals");
    char buffer[128];

    g_snprintf(buffer, 128, "%d item(s) read from %s", reload_training_data(xg, file), file);
    gtkx_warn(chooser, buffer);
    gtk_widget_destroy(chooser);
}

static void load_patterns_callback(GtkWidget *mi, XGlobals *xg)
{
    GtkWidget *chooser = gtk_file_selection_new("Select training file");
    g_object_set_data(G_OBJECT(chooser), "globals", xg);
    gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(chooser));
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(chooser)->ok_button), "clicked", GTK_SIGNAL_FUNC(select_training_file), chooser);
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(chooser)->cancel_button), "clicked", GTK_SIGNAL_FUNC(destroy_chooser), chooser);
    gtkx_position_popup(xg->frame, chooser);
    gtk_widget_show(chooser);
}

/******************************************************************************/

static void set_net_type_callback(GtkWidget *combo_box, XGlobals *xg)
{
    NetworkType nt = (NetworkType) gtk_combo_box_get_active(GTK_COMBO_BOX(combo_box));

    if (xg->pars.nt != nt) {
        xg->pars.nt = nt;
        initialise_weights_callback(NULL, xg);
    }
}

static void callback_spin_wn(GtkWidget *button, XGlobals *xg)
{
    xg->pars.wn = gtk_spin_button_get_value(GTK_SPIN_BUTTON(button));
}

static void callback_spin_epochs(GtkWidget *button, XGlobals *xg)
{
    xg->pars.epochs = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(button));
}

/******************************************************************************/

static void populate_weight_history_page(GtkWidget *page, XGlobals *xg)
{
    GtkWidget *tmp, *hbox;
    char buffer[16];
    NetworkType nt;

    /*--- The Network Type widgets: ---*/

    hbox = gtk_hbox_new(FALSE, 0);

    tmp = gtk_label_new("Network Type:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    tmp = gtk_combo_box_new_text();
    for (nt = 0; nt < NT_MAX; nt++) {
        gtk_combo_box_insert_text(GTK_COMBO_BOX(tmp), nt, nt_name[nt]);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), xg->pars.nt);
    g_signal_connect(G_OBJECT(tmp), "changed", G_CALLBACK(set_net_type_callback), xg);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /*--- Separator : ---*/

    tmp = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(page), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);

    /*--- The Patterns widgets: ---*/

    hbox = gtk_hbox_new(FALSE, 0);

    tmp = gtk_label_new("Training Patterns:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    tmp = gtk_button_new_with_label("  Load  ");
    g_signal_connect(G_OBJECT(tmp), "clicked", GTK_SIGNAL_FUNC(load_patterns_callback), xg);
    gtk_box_pack_end(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /**** Widgets for displaying the training set filename: ****/

    hbox = gtk_hbox_new(FALSE, 0);
    /*--- The label: ---*/
    tmp = gtk_label_new("    File:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    xg->training_set_label = gtk_label_new("NONE");
    gtk_box_pack_start(GTK_BOX(hbox), xg->training_set_label, FALSE, FALSE, 5);
    gtk_widget_show(xg->training_set_label);
    /*---Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /*--- The Weight History widgets: ---*/

    hbox = gtk_hbox_new(FALSE, 0);

    tmp = gtk_label_new("Weight history:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    tmp = gtk_button_new_with_label("  Save  ");
    g_signal_connect(G_OBJECT(tmp), "clicked", GTK_SIGNAL_FUNC(save_weights_callback), xg);
    gtk_box_pack_end(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    tmp = gtk_button_new_with_label("  Load RC ");
    g_signal_connect(G_OBJECT(tmp), "clicked", GTK_SIGNAL_FUNC(load_weights_rc_callback), xg);
    gtk_box_pack_end(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    tmp = gtk_button_new_with_label("  Load OG ");
    g_signal_connect(G_OBJECT(tmp), "clicked", GTK_SIGNAL_FUNC(load_weights_og_callback), xg);
    gtk_box_pack_end(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    tmp = gtk_button_new_with_label("  Randomise  ");
    g_signal_connect(G_OBJECT(tmp), "clicked", GTK_SIGNAL_FUNC(initialise_weights_callback), xg);
    gtk_box_pack_end(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /**** Widgets for displaying the training history: ****/

    hbox = gtk_hbox_new(FALSE, 0);
    /*--- The label: ---*/
    tmp = gtk_label_new("    Weight initialisation:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    xg->weight_history_label = gtk_label_new("Randomised");
    gtk_box_pack_start(GTK_BOX(hbox), xg->weight_history_label, FALSE, FALSE, 5);
    gtk_widget_show(xg->weight_history_label);
    /*---Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /**** Widgets for adjusting initial weight noise: ****/

    hbox = gtk_hbox_new(FALSE, 0);
    /*--- The label: ---*/
    tmp = gtk_label_new("    Initial weight noise (max):");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    /*--- The spin button: ---*/
    tmp = gtk_spin_button_new_with_range(0.0, 1.0, 0.001);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tmp), TRUE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tmp), xg->pars.wn);
    gtk_widget_set_size_request(GTK_WIDGET(tmp), 75, -1);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(tmp), "value-changed", GTK_SIGNAL_FUNC(callback_spin_wn), xg);
    gtk_widget_show(tmp);
    /*--- Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    hbox = gtk_hbox_new(FALSE, 0);
    /*--- The label: ---*/
    tmp = gtk_label_new("    Cycles since initialisation:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    g_snprintf(buffer, 16, "%d", xg->training_step);
    xg->weight_history_cycles = gtk_label_new(buffer);
    gtk_box_pack_start(GTK_BOX(hbox), xg->weight_history_cycles, FALSE, FALSE, 5);
    gtk_widget_show(xg->weight_history_cycles);
    /*---Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /*--- Separator : ---*/

    tmp = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(page), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
}

/******************************************************************************/

static void callback_spin_lr(GtkWidget *button, XGlobals *xg)
{
    xg->pars.lr = gtk_spin_button_get_value(GTK_SPIN_BUTTON(button));
}

static void callback_spin_momentum(GtkWidget *button, XGlobals *xg)
{
    xg->pars.momentum = gtk_spin_button_get_value(GTK_SPIN_BUTTON(button));
}

static void callback_spin_wd(GtkWidget *button, XGlobals *xg)
{
    xg->pars.wd = gtk_spin_button_get_value(GTK_SPIN_BUTTON(button));
}

static void callback_toggle_ef(GtkWidget *button, void *data)
{
    XGlobals *xg = (XGlobals *)g_object_get_data(G_OBJECT(button), "globals");

    if (GTK_TOGGLE_BUTTON(button)->active) {
        xg->pars.ef = (ErrorFunction) data;
    }
}

static void callback_toggle_wut(GtkWidget *button, void *data)
{
    XGlobals *xg = (XGlobals *)g_object_get_data(G_OBJECT(button), "globals");

    if (GTK_TOGGLE_BUTTON(button)->active) {
        xg->pars.wut = (WeightUpdateTime) data;
    }
}

static void populate_parameters_page(GtkWidget *page, XGlobals *xg)
{
    GtkWidget *hbox, *tmp, *button;

    hbox = gtk_hbox_new(FALSE, 0);
    /*--- The label: ---*/
    tmp = gtk_label_new("Training parameters:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /**** Widgets for adjusting the learning rate: ****/

    hbox = gtk_hbox_new(FALSE, 0);
    /*--- The label: ---*/
    tmp = gtk_label_new("    Learning rate:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    /*--- The spin button: ---*/
    tmp = gtk_spin_button_new_with_range(0.0, 1.0, 0.001);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tmp), TRUE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tmp), xg->pars.lr);
    gtk_widget_set_size_request(GTK_WIDGET(tmp), 75, -1);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(tmp), "value-changed", GTK_SIGNAL_FUNC(callback_spin_lr), xg);
    gtk_widget_show(tmp);
    /*--- Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /**** Widgets for adjusting the momentum: ****/

    hbox = gtk_hbox_new(FALSE, 0);
    /*--- The label: ---*/
    tmp = gtk_label_new("    Momentum:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    /*--- The spin button: ---*/
    tmp = gtk_spin_button_new_with_range(0.00, 0.99, 0.01);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tmp), TRUE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tmp), xg->pars.momentum);
    gtk_widget_set_size_request(GTK_WIDGET(tmp), 75, -1);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(tmp), "value-changed", GTK_SIGNAL_FUNC(callback_spin_momentum), xg);
    gtk_widget_show(tmp);
    /*--- Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /**** Widgets for adjusting the weight decay: ****/

    hbox = gtk_hbox_new(FALSE, 0);
    /*--- The label: ---*/
    tmp = gtk_label_new("    Weight decay:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    /*--- The spin button: ---*/
    tmp = gtk_spin_button_new_with_range(0.0, 1.0, 0.001);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tmp), TRUE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tmp), xg->pars.wd);
    gtk_widget_set_size_request(GTK_WIDGET(tmp), 75, -1);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(tmp), "value-changed", GTK_SIGNAL_FUNC(callback_spin_wd), xg);
    gtk_widget_show(tmp);
    /*--- Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /**** Widgets for adjusting the training epochs: ***/

    hbox = gtk_hbox_new(FALSE, 0);
    /*--- The label: ---*/
    tmp = gtk_label_new("    Training epochs:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    /*--- The spin button: ---*/
    tmp = gtk_spin_button_new_with_range(0.0, 100000.0, 1);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tmp), TRUE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tmp), xg->pars.epochs);
    gtk_widget_set_size_request(GTK_WIDGET(tmp), 75, -1);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(tmp), "value-changed", GTK_SIGNAL_FUNC(callback_spin_epochs), xg);
    gtk_widget_show(tmp);
    /*--- Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /**** Widgets for adjusting the error function: ****/

    hbox = gtk_hbox_new(FALSE, 0);
     /*--- The label: ---*/
    tmp = gtk_label_new("    Error function:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    /*--- The first error function radio button: ---*/
    button = gtk_radio_button_new_with_label(NULL, "SSE");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), (xg->pars.ef == SUM_SQUARE_ERROR));
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    g_object_set_data(G_OBJECT(button), "globals", xg);
    g_signal_connect(G_OBJECT(button), "toggled", GTK_SIGNAL_FUNC(callback_toggle_ef), (void *)SUM_SQUARE_ERROR);
    gtk_widget_show(button);
    /*--- The second error function radio button: ---*/
    button = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(button)), "Cross Entropy");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), (xg->pars.ef == CROSS_ENTROPY));
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    g_object_set_data(G_OBJECT(button), "globals", xg);
    g_signal_connect(G_OBJECT(button), "toggled", GTK_SIGNAL_FUNC(callback_toggle_ef), (void *)CROSS_ENTROPY);
    gtk_widget_show(button);
    /*--- The third error function radio button: ---*/
    button = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(button)), "SoftMax");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), (xg->pars.ef == SOFT_MAX_ERROR));
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    g_object_set_data(G_OBJECT(button), "globals", xg);
    g_signal_connect(G_OBJECT(button), "toggled", GTK_SIGNAL_FUNC(callback_toggle_ef), (void *)SOFT_MAX_ERROR);
    gtk_widget_set_sensitive(button, FALSE);
    gtk_widget_show(button);
    /*--- Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /**** Widgets for adjusting the weight update time: ****/

    hbox = gtk_hbox_new(FALSE, 0);
    /*--- The label: ---*/
    tmp = gtk_label_new("    Update weights:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    /*--- The first update time radio button: ---*/
    button = gtk_radio_button_new_with_label(NULL, "by item");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), (xg->pars.wut == UPDATE_BY_ITEM));
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    g_object_set_data(G_OBJECT(button), "globals", xg);
    g_signal_connect(G_OBJECT(button), "toggled", GTK_SIGNAL_FUNC(callback_toggle_wut), (void *)UPDATE_BY_ITEM);
    gtk_widget_show(button);
    /*--- The second update time radio button: ---*/
    button = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(button)), "by epoch");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), (xg->pars.wut == UPDATE_BY_EPOCH));
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    g_object_set_data(G_OBJECT(button), "globals", xg);
    g_signal_connect(G_OBJECT(button), "toggled", GTK_SIGNAL_FUNC(callback_toggle_wut), (void *)UPDATE_BY_EPOCH);
    gtk_widget_show(button);
    /*--- Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /*--- Separator : ---*/

    tmp = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(page), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
}

/* The top-level widget creation routine **************************************/

GtkWidget *make_widgets(XGlobals *xg)
{
    GtkWidget *page, *notes;

    xg->net = network_initialise(xg->pars.nt, IO_WIDTH, HIDDEN_WIDTH, IO_WIDTH);
    network_initialise_weights(xg->net, xg->pars.wn);

    xg->frame = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_container_set_border_width(GTK_CONTAINER(xg->frame), 0);
    gtk_widget_set_size_request(xg->frame, 900, 600);
    gtk_window_set_title(GTK_WINDOW(xg->frame), "Tyler et al. Model");
    g_signal_connect(G_OBJECT(xg->frame), "destroy", GTK_SIGNAL_FUNC(exit_bp), xg);

    /* The notebook: */

    notes = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(xg->frame), notes);

    /* Page A1: Weight history: */

    page = gtk_vbox_new(FALSE, 0);
    populate_weight_history_page(page, xg);
    populate_parameters_page(page, xg);
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("Configuration"));
    gtk_widget_show(page);

    /* Page A1: Pattern set details: */

    page = gtk_vbox_new(FALSE, 0);
    tyler_patterns_create_widgets(page, xg);
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("Patterns"));
    gtk_widget_show(page);

    /* Page A2: The error graph and training interface: */

    page = gtk_vbox_new(FALSE, 0);
    xgraph_create(page, xg);
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("Train"));
    gtk_widget_show(page);

    /* Page A3: Network testing view: */

    page = gtk_vbox_new(FALSE, 0);
    test_net_create_widgets(page, xg);
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("Test"));
    gtk_widget_show(page);

    /* Page A4: Notebook of various network testing views: */

    page = gtk_vbox_new(FALSE, 0);
    test_attractor_create_widgets(page, xg);
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("Attractors"));
    gtk_widget_show(page);

    /* Page A5: Histograph of weights: */

    page = gtk_vbox_new(FALSE, 0);
    weight_histogram_create_widgets(page, xg);
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("Weights"));
    gtk_widget_show(page);

    /* Page A6: Graphs based on Tyler et al., 2000: */

    page = gtk_vbox_new(FALSE, 0);
    lesion_viewer_create_widgets(page, xg);
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("Tyler Graphs"));
    gtk_widget_show(page);

    /* End of notebook pages */
    gtk_notebook_set_page(GTK_NOTEBOOK(notes), 0);
    gtk_widget_show(notes);
 
    xg->training_set = xtraining_set_read(xg, TRAINING_PATTERNS, IO_WIDTH, IO_WIDTH);
    gtk_window_set_position(GTK_WINDOW(xg->frame), GTK_WIN_POS_CENTER);
    return(xg->frame);
}

/******************************************************************************/
