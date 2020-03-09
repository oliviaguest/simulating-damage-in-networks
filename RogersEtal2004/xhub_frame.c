/*******************************************************************************

    File:       xhub_frame.c
    Contents:   Create the main frame and its notebook with all the pages.
    Author:     Rick Cooper
    Copyright (c) 2016 Richard P. Cooper

    Public procedures:
    Boolean weight_file_read(XGlobals *xg, char *filename);
    void populate_weight_history_page(GtkWidget *page, XGlobals *xg)
    void populate_parameters_page(GtkWidget *page, XGlobals *xg)
    Boolean xg_make_widgets(XGlobals *xg)
    void xg_initialise(XGlobals *xg)

TO DO: Move the bulk of this to xhub.c and put the rest in xhub_parameters.c

*******************************************************************************/

/******** Include files: ******************************************************/

#include "xhub.h"
#include <ctype.h>
#include <string.h>
#include "lib_string.h"

#define PATTERN_SET "DataFiles/p1_1000/Patterns P1.pat"
#define WEIGHT_FILE "DataFiles/p1_1000/01.wgt"

// File for saving patterns in a format for the Tyler et al. model
#define PATTERN_SAVE_FILE "../hub_patterns.pat"

/******************************************************************************/
/* Callback to load a set of patterns/sequences *******************************/

static void ps_extract_ps_name_from_filename(char *buffer, int l, const char *file)
{
    char *fl;
    int k = 0;

    fl = strrchr(file, '/');

    k = 0;
    while ((fl[k+1] != '.') && (k < l)) {
        buffer[k] = fl[k+1]; k++;
    }
    buffer[k] = '\0';

    //    k = 0;
    //    while (buffer[k] != '\0') {
    //        buffer[k] = toupper(buffer[k]);
    //        k++;
    //    }
}

Boolean xpattern_set_read(XGlobals *xg, char *file)
{
    PatternList *tmp;
    char buffer[16];

    if ((tmp = hub_pattern_set_read(file)) != NULL) {
        hub_pattern_set_free(xg->pattern_set);
        xg->pattern_set = tmp;
        gtk_label_set_text(GTK_LABEL(xg->label_pattern_set), file);
        xg->current_pattern = xg->pattern_set;

        if (xg->pattern_set_name != NULL) {
            free(xg->pattern_set_name);
        }
        ps_extract_ps_name_from_filename(buffer, 16, file);
        xg->pattern_set_name = string_copy(buffer);

        fprintf(stdout, "Pattern set successfully loaded from %s\n", file);
        return(TRUE);
    }
    else {
        fprintf(stdout, "WARNING: Pattern set not loaded\n");
        return(FALSE);
    }
}

/*----------------------------------------------------------------------------*/

static int reload_training_data(XGlobals *xg, char *file)
{
    xpattern_set_read(xg, file);
    return(pattern_list_length(xg->pattern_set));
}

/*----------------------------------------------------------------------------*/

static void select_training_file(GtkWidget *w, GtkWidget *chooser)
{
    char *file = (char *)gtk_file_selection_get_filename(GTK_FILE_SELECTION(chooser));
    XGlobals *xg = g_object_get_data(G_OBJECT(chooser), "globals");
    char buffer[128];

    g_snprintf(buffer, 128, "%d item(s) read from %s", reload_training_data(xg, file), file);
    gtkx_warn(chooser, buffer);
    gtk_widget_destroy(chooser);
}

/*----------------------------------------------------------------------------*/

static void write_pattern_to_fp(FILE *fp, PatternList *tmp)
{
    int i;

    for (i = 0; i < NUM_NAME; i++) {
        fprintf(fp, "%3.1f ", tmp->name_features[i]);
    }
    for (i = 0; i < NUM_VERBAL; i++) {
        fprintf(fp, "%3.1f ", tmp->verbal_features[i]);
    }
    for (i = 0; i < NUM_VISUAL; i++) {
        fprintf(fp, "%3.1f ", tmp->visual_features[i]);
    }
    fprintf(fp, ">");
    for (i = 0; i < NUM_NAME; i++) {
        fprintf(fp, " %3.1f", tmp->name_features[i]);
    }
    for (i = 0; i < NUM_VERBAL; i++) {
        fprintf(fp, " %3.1f", tmp->verbal_features[i]);
    }
    for (i = 0; i < NUM_VISUAL; i++) {
        fprintf(fp, " %3.1f", tmp->visual_features[i]);
    }
    fprintf(fp, "\n");
}

static void write_patterns_callback(GtkWidget *mi, XGlobals *xg)
{
#if FALSE
    GtkWidget *chooser = gtk_file_selection_new("Select training file");
    g_object_set_data(G_OBJECT(chooser), "globals", xg);
    gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(chooser));
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(chooser)->ok_button), "clicked", G_CALLBACK(select_training_file), chooser);
    g_signal_connect_swapped(G_OBJECT(GTK_FILE_SELECTION(chooser)->cancel_button), "clicked", G_CALLBACK(gtk_widget_destroy), chooser);
    gtkx_position_popup(xg->frame, chooser);
    gtk_widget_show(chooser);
#endif

    FILE *fp;
    PatternList *tmp;

    if ((fp = fopen(PATTERN_SAVE_FILE, "w")) != NULL) {
        for (tmp = xg->pattern_set; tmp != NULL; tmp = tmp->next) {
            write_pattern_to_fp(fp, tmp);
        }
        fclose(fp);
        fprintf(stdout, "Patterns written to %s\n", PATTERN_SAVE_FILE);
    }    
}

/*----------------------------------------------------------------------------*/

static void load_patterns_callback(GtkWidget *mi, XGlobals *xg)
{
    GtkWidget *chooser = gtk_file_selection_new("Select training file");
    g_object_set_data(G_OBJECT(chooser), "globals", xg);
    gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(chooser));
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(chooser)->ok_button), "clicked", G_CALLBACK(select_training_file), chooser);
    g_signal_connect_swapped(G_OBJECT(GTK_FILE_SELECTION(chooser)->cancel_button), "clicked", G_CALLBACK(gtk_widget_destroy), chooser);
    gtkx_position_popup(xg->frame, chooser);
    gtk_widget_show(chooser);
}

/******************************************************************************/

static Boolean is_pattern_set_file(struct dirent *de)
{
    int l;

    l = strlen(de->d_name);
    return((de->d_type == 8) && (l > 4) && (strncmp(&(de->d_name[l-4]), ".pat", 4) == 0));
}

Boolean pattern_file_load_from_folder(XGlobals *xg, char *sub_folder)
{
    DIR *dir;
    struct dirent *de;
    Boolean found = FALSE;
    char filename[128];

    g_snprintf(filename, 128, "DataFiles/%s", sub_folder);

    if ((dir = opendir(filename)) != NULL) {
        while ((!found) && ((de = readdir(dir)) != NULL)) {
            if (is_pattern_set_file(de)) {
                g_snprintf(filename, 128, "DataFiles/%s/%s", sub_folder, de->d_name);
                found = TRUE;
            }
        }
        closedir(dir);
    }

    if (found) {
        return(xpattern_set_read(xg, filename));
    }
    else {
        return(FALSE);
    }
}

/******************************************************************************/
/* Callback to save a set of weights ******************************************/

static Boolean weight_file_dump_header_info(FILE *fp, XGlobals *xg)
{
    char *training_data = (char *)gtk_label_get_text(GTK_LABEL(xg->label_pattern_set));

    fprintf(fp, "%% Training set: %s\n", training_data);
    fprintf(fp, "%% Epochs: %d\n", xg->training_step);

    return(TRUE);
}

/*----------------------------------------------------------------------------*/

static void save_weights_to_file(GtkWidget *w, GtkWidget *chooser)
{
    char *file = (char *)gtk_file_selection_get_filename(GTK_FILE_SELECTION(chooser));
    XGlobals *xg = g_object_get_data(G_OBJECT(chooser), "globals");
    char buffer[128];
    FILE *fp;

    if ((fp = fopen(file, "w")) == NULL) {
        g_snprintf(buffer, 128, "ERROR: Cannot write to %s", file);
    }
    else if (!weight_file_dump_header_info(fp, xg)) {
        g_snprintf(buffer, 128, "ERROR: Cannot dump weight header information ... weights not saved");
        fclose(fp);
        remove(file);
    }
    else if (!network_write_to_file(fp, xg->net)) {
        g_snprintf(buffer, 128, "ERROR: Problem dumping weights ... network not saved");
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

/*----------------------------------------------------------------------------*/

static void save_weights_callback(GtkWidget *button, XGlobals *xg)
{
    GtkWidget *chooser = gtk_file_selection_new("Save weight file");
    g_object_set_data(G_OBJECT(chooser), "globals", xg);
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(chooser)->ok_button), "clicked", G_CALLBACK(save_weights_to_file), chooser);
    g_signal_connect_swapped(G_OBJECT(GTK_FILE_SELECTION(chooser)->cancel_button), "clicked", G_CALLBACK(gtk_widget_destroy), chooser);
    gtkx_position_popup(xg->frame, chooser);
    gtk_widget_show(chooser);
}

/******************************************************************************/
/* Callback to load a set of weights ******************************************/

Boolean weight_file_read(XGlobals *xg, char *filename)
{
    char buffer[128];
    char *err = NULL;
    Network *new_net = NULL;
    FILE *fp;

    if ((fp = fopen(filename, "r")) == NULL) {
        g_snprintf(buffer, 128, "ERROR: Failed to open weight file (%s)", filename);
        gtkx_warn(xg->frame, buffer);
        return(FALSE);
    }
    else if ((new_net = network_read_from_file(fp, &err)) == NULL) {
        fclose(fp);
        g_snprintf(buffer, 128, "ERROR: Weight file format error (%s) ... weights not restored", err);
        gtkx_warn(xg->frame, buffer);
        return(FALSE);
    }
    else {
        fclose(fp);
        network_parameters_set(new_net, &(xg->net->params));
        network_destroy(xg->net);
        xg->net = new_net;
        gtk_label_set_text(GTK_LABEL(xg->label_weight_history), filename);
        hub_explore_initialise_network(xg);
        fprintf(stdout, "Weights successfully restored from %s\n", filename);
	fprintf(stdout, "  Weight range: [%7.5f - %7.5f];", network_weight_minimum(new_net), network_weight_maximum(new_net));
	fprintf(stdout, " Error: %7.5f (Max. Bit); %7.5f (RMS)\n", network_test_max_bit(new_net, xg->pattern_set), sqrt(network_test(new_net, xg->pattern_set, EF_SUM_SQUARE)));
        fflush(stdout);
        return(TRUE);
    }
}

/*----------------------------------------------------------------------------*/

static void select_and_load_weight_file(GtkWidget *w, GtkWidget *chooser)
{
    char *filename = (char *)gtk_file_selection_get_filename(GTK_FILE_SELECTION(chooser));
    XGlobals *xg = g_object_get_data(G_OBJECT(chooser), "globals");
    char buffer[128];

    if (weight_file_read(xg, filename)) {
        g_snprintf(buffer, 128, "Weights successfully restored from\n%s", filename);
    }
    else {
        g_snprintf(buffer, 128, "WARNING: Weights not restored\n");
    }
    gtkx_warn(chooser, buffer);
    gtk_widget_destroy(chooser);
}

/*----------------------------------------------------------------------------*/

static void load_weights_callback(GtkWidget *button, XGlobals *xg)
{
    GtkWidget *chooser = gtk_file_selection_new("Select weight file");
    g_object_set_data(G_OBJECT(chooser), "globals", xg);
    gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(chooser));
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(chooser)->ok_button), "clicked", G_CALLBACK(select_and_load_weight_file), chooser);
    g_signal_connect_swapped(G_OBJECT(GTK_FILE_SELECTION(chooser)->cancel_button), "clicked", G_CALLBACK(gtk_widget_destroy), chooser);
    gtkx_position_popup(xg->frame, chooser);
    gtk_widget_show(chooser);
}

/******************************************************************************/
/* Other callbacks available from the configuration page: *********************/

static void initialise_weights_callback(GtkWidget *button, XGlobals *xg)
{
    network_initialise_weights(xg->net);
    hub_explore_initialise_network(xg);
    gtk_label_set_text(GTK_LABEL(xg->label_weight_history), "Randomised");
    gtk_label_set_text(GTK_LABEL(xg->label_training_cycles), "0");
}

/*============================================================================*/

static void callback_spin_wn(GtkWidget *button, XGlobals *xg)
{
    xg->net->params.wn = gtk_spin_button_get_value(GTK_SPIN_BUTTON(button));
}

/******************************************************************************/
/* Weight history widgets: ****************************************************/

void populate_weight_history_page(GtkWidget *page, XGlobals *xg)
{
    GtkWidget *hbox, *tmp;
    char buffer[16];

    /*--- The Patterns widgets: ---*/

    hbox = gtk_hbox_new(FALSE, 0);

    tmp = gtk_label_new("Training Patterns:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    tmp = gtk_button_new_with_label("  Load...  ");
    g_signal_connect(G_OBJECT(tmp), "clicked", G_CALLBACK(load_patterns_callback), xg);
    gtk_box_pack_end(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    tmp = gtk_button_new_with_label(" Save as... ");
    g_signal_connect(G_OBJECT(tmp), "clicked", G_CALLBACK(write_patterns_callback), xg);
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
    xg->label_pattern_set = gtk_label_new("NONE");
    gtk_box_pack_start(GTK_BOX(hbox), xg->label_pattern_set, FALSE, FALSE, 5);
    gtk_widget_show(xg->label_pattern_set);
    /*---Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /*--- The Weight History widgets: ---*/

    hbox = gtk_hbox_new(FALSE, 0);

    tmp = gtk_label_new("Weight history:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    tmp = gtk_button_new_with_label(" Save as... ");
    g_signal_connect(G_OBJECT(tmp), "clicked", G_CALLBACK(save_weights_callback), xg);
    gtk_box_pack_end(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    tmp = gtk_button_new_with_label("  Load...  ");
    g_signal_connect(G_OBJECT(tmp), "clicked", G_CALLBACK(load_weights_callback), xg);
    gtk_box_pack_end(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    tmp = gtk_button_new_with_label("  Randomise  ");
    g_signal_connect(G_OBJECT(tmp), "clicked", G_CALLBACK(initialise_weights_callback), xg);
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
    xg->label_weight_history = gtk_label_new("Randomised");
    gtk_box_pack_start(GTK_BOX(hbox), xg->label_weight_history, FALSE, FALSE, 5);
    gtk_widget_show(xg->label_weight_history);
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
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tmp), xg->net->params.wn);
    gtk_widget_set_size_request(GTK_WIDGET(tmp), 75, -1);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(tmp), "value-changed", G_CALLBACK(callback_spin_wn), xg);
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
    xg->label_training_cycles = gtk_label_new(buffer);
    gtk_box_pack_start(GTK_BOX(hbox), xg->label_training_cycles, FALSE, FALSE, 5);
    gtk_widget_show(xg->label_training_cycles);
    /*---Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /*--- Separator : ---*/

    tmp = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(page), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
}

/******************************************************************************/
/* Callbacks for setting network parameters: **********************************/

static void callback_spin_lr(GtkWidget *button, XGlobals *xg)
{
    xg->net->params.lr = gtk_spin_button_get_value(GTK_SPIN_BUTTON(button));
}

/*----------------------------------------------------------------------------*/

static void callback_spin_momentum(GtkWidget *button, XGlobals *xg)
{
    xg->net->params.momentum = gtk_spin_button_get_value(GTK_SPIN_BUTTON(button));
}

/*----------------------------------------------------------------------------*/

static void callback_spin_wd(GtkWidget *button, XGlobals *xg)
{
    xg->net->params.wd = gtk_spin_button_get_value(GTK_SPIN_BUTTON(button));
}

/*----------------------------------------------------------------------------*/

static void callback_toggle_ef(GtkWidget *button, void *data)
{
    XGlobals *xg = (XGlobals *)g_object_get_data(G_OBJECT(button), "globals");

    if (GTK_TOGGLE_BUTTON(button)->active) {
        xg->net->params.ef = (ErrorFunction) data;
    }
}

/*----------------------------------------------------------------------------*/

static void callback_toggle_wut(GtkWidget *button, void *data)
{
    XGlobals *xg = (XGlobals *)g_object_get_data(G_OBJECT(button), "globals");

    if (GTK_TOGGLE_BUTTON(button)->active) {
        xg->net->params.wut = (WeightUpdateTime) data;
    }
}

/*----------------------------------------------------------------------------*/

static void callback_spin_criterion(GtkWidget *button, XGlobals *xg)
{
    xg->net->params.criterion = gtk_spin_button_get_value(GTK_SPIN_BUTTON(button));
}

/*----------------------------------------------------------------------------*/

static void callback_spin_epochs(GtkWidget *button, XGlobals *xg)
{
    xg->net->params.epochs = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(button));
}

/*----------------------------------------------------------------------------*/

static void callback_spin_sc(GtkWidget *button, XGlobals *xg)
{
    xg->net->params.sc = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(button));
    hub_explore_initialise_network(xg);
}

/*----------------------------------------------------------------------------*/

static void callback_spin_st(GtkWidget *button, XGlobals *xg)
{
    xg->net->params.st = gtk_spin_button_get_value(GTK_SPIN_BUTTON(button));
    hub_explore_initialise_network(xg);
}

/*----------------------------------------------------------------------------*/

static void callback_spin_ticks(GtkWidget *button, XGlobals *xg)
{
    xg->net->params.ticks = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(button));
    hub_explore_initialise_network(xg);
}

/******************************************************************************/
/* Network parameter widgets: *************************************************/

void populate_parameters_page(GtkWidget *page, XGlobals *xg)
{
    GtkWidget *hbox, *tmp, *button;
    GSList *group;
    long i;

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
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tmp), xg->net->params.lr);
    gtk_widget_set_size_request(GTK_WIDGET(tmp), 75, -1);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(tmp), "value-changed", G_CALLBACK(callback_spin_lr), xg);
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
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tmp), xg->net->params.momentum);
    gtk_widget_set_size_request(GTK_WIDGET(tmp), 75, -1);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(tmp), "value-changed", G_CALLBACK(callback_spin_momentum), xg);
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
    tmp = gtk_spin_button_new_with_range(0.0, 1.0, 0.0001);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tmp), TRUE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tmp), xg->net->params.wd);
    gtk_widget_set_size_request(GTK_WIDGET(tmp), 75, -1);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(tmp), "value-changed", G_CALLBACK(callback_spin_wd), xg);
    gtk_widget_show(tmp);
    /*--- Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /**** Widgets for adjusting the error function: ****/

    hbox = gtk_hbox_new(FALSE, 0);
     /*--- The label: ---*/    tmp = gtk_label_new("    Error function:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    /*--- The Error Function radio buttons: ---*/
    group = NULL;
    for (i = 0; i < EF_MAX; i++) {
        button = gtk_radio_button_new_with_label(group, net_ef_label[i]);
        group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), (xg->net->params.ef == (ErrorFunction) i));
        gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 5);
        g_object_set_data(G_OBJECT(button), "globals", xg);
        g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(callback_toggle_ef), (void *)i);
        gtk_widget_show(button);
    }
    /*--- Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /**** Widgets for adjusting the weight update time: ****/

    hbox = gtk_hbox_new(FALSE, 0);
    /*--- The label: ---*/
    tmp = gtk_label_new("    Update weights by:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    /*--- The update time radio buttons: ---*/
    group = NULL;
    for (i = 0; i < WU_MAX; i++) {
        button = gtk_radio_button_new_with_label(group, net_wu_label[i]);
        group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), (xg->net->params.wut == (WeightUpdateTime) i));
        gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 5);
        g_object_set_data(G_OBJECT(button), "globals", xg);
        g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(callback_toggle_wut), (void *)i);
        gtk_widget_show(button);
    }
    /*--- Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /**** Widgets for adjusting the training criterion: ****/

    hbox = gtk_hbox_new(FALSE, 0);
    /*--- The label: ---*/
    tmp = gtk_label_new("    Training criterion:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    /*--- The spin button: ---*/
    tmp = gtk_spin_button_new_with_range(0.0, 1.0, 0.001);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tmp), TRUE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tmp), xg->net->params.criterion);
    gtk_widget_set_size_request(GTK_WIDGET(tmp), 75, -1);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(tmp), "value-changed", G_CALLBACK(callback_spin_criterion), xg);
    gtk_widget_show(tmp);
    /*--- Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /**** Widgets for adjusting the training epochs: ****/

    hbox = gtk_hbox_new(FALSE, 0);
    /*--- The label: ---*/
    tmp = gtk_label_new("    Training epochs:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    /*--- The spin button: ---*/
    tmp = gtk_spin_button_new_with_range(0.0, 100000.0, 1);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tmp), TRUE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tmp), xg->net->params.epochs);
    gtk_widget_set_size_request(GTK_WIDGET(tmp), 75, -1);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(tmp), "value-changed", G_CALLBACK(callback_spin_epochs), xg);
    gtk_widget_show(tmp);
    /*--- Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /*--- Separator : ---*/

    tmp = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(page), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);

    hbox = gtk_hbox_new(FALSE, 0);
    /*--- The label: ---*/
    tmp = gtk_label_new("Other parameters:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /**** Widgets for adjusting unit initialisation: ****/

    hbox = gtk_hbox_new(FALSE, 0);
    /*--- The label: ---*/
    tmp = gtk_label_new("    Unit initialisation:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    /*--- The first update time radio button: ---*/
    button = gtk_radio_button_new_with_label(NULL, "Random");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), (xg->net->params.ui == UI_RANDOM));
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    g_object_set_data(G_OBJECT(button), "globals", xg);
    g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(callback_toggle_wut), (void *)UI_RANDOM);
    gtk_widget_show(button);
    /*--- The second update time radio button: ---*/
    button = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(button)), "Fixed");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), (xg->net->params.ui == UI_FIXED));
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    g_object_set_data(G_OBJECT(button), "globals", xg);
    g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(callback_toggle_wut), (void *)UI_FIXED);
    gtk_widget_show(button);
    /*--- Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /**** Widgets for adjusting the ticks per cycle (time averaging): *********/

    hbox = gtk_hbox_new(FALSE, 0);
    /*--- The label: ---*/
    tmp = gtk_label_new("    Ticks per cycle:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    /*--- The spin button: ---*/
    tmp = gtk_spin_button_new_with_range(1.0, 10.0, 1);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tmp), TRUE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tmp), xg->net->params.ticks);
    gtk_widget_set_size_request(GTK_WIDGET(tmp), 100, -1);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(tmp), "value-changed", G_CALLBACK(callback_spin_ticks), xg);
    gtk_widget_show(tmp);
    /*--- Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /**** Widgets for number of cycles for settling (if zero then settle until threshold) */

    hbox = gtk_hbox_new(FALSE, 0);
    /*--- The label: ---*/
    tmp = gtk_label_new("    Cycles to settle:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    /*--- The spin button: ---*/
    tmp = gtk_spin_button_new_with_range(0.0, 100.0, 1);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tmp), TRUE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tmp), xg->net->params.sc);
    gtk_widget_set_size_request(GTK_WIDGET(tmp), 100, -1);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(tmp), "value-changed", G_CALLBACK(callback_spin_sc), xg);
    gtk_widget_show(tmp);
    /*--- Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /**** Widgets for adjusting the settling threshold: ****/

    hbox = gtk_hbox_new(FALSE, 0);
    /*--- The label: ---*/
    tmp = gtk_label_new("    Settling threshold:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    /*--- The spin button: ---*/
    tmp = gtk_spin_button_new_with_range(0.0, 1.0, 0.00001);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tmp), TRUE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tmp), xg->net->params.st);
    gtk_widget_set_size_request(GTK_WIDGET(tmp), 100, -1);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(tmp), "value-changed", G_CALLBACK(callback_spin_st), xg);
    gtk_widget_show(tmp);
    /*--- Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);
}

/******************************************************************************/
/* The frame and its notebook: ************************************************/

static void exit_gtk(GtkWidget *button, XGlobals *xg)
{
    if (xg->pattern_set != NULL) {
        hub_pattern_set_free(xg->pattern_set);
        xg->pattern_set = NULL;
        xg->current_pattern = NULL;
    }
    if (xg->pattern_set_name != NULL) {
        free(xg->pattern_set_name);
        xg->pattern_set_name = NULL;
    }
    gtk_main_quit();
}


/*----------------------------------------------------------------------------*/

Boolean xg_make_widgets(XGlobals *xg)
{
    GtkWidget *notes, *page;

    if ((xg->frame = gtk_window_new(GTK_WINDOW_TOPLEVEL)) == NULL) {
        return(FALSE);
    }
    else {
        gtk_container_set_border_width(GTK_CONTAINER(xg->frame), 0);
        gtk_widget_set_size_request(xg->frame, 900, 600);
        gtk_window_set_title(GTK_WINDOW(xg->frame), "Hub Model");
        g_signal_connect(G_OBJECT(xg->frame), "destroy", G_CALLBACK(exit_gtk), xg);

        /* The notebook: */
        notes = gtk_notebook_new();
        gtk_container_add(GTK_CONTAINER(xg->frame), notes);

        /* Configuration and weight history: */
        page = gtk_vbox_new(FALSE, 0);
        populate_weight_history_page(page, xg);
        populate_parameters_page(page, xg);
        gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("Configuration"));
        gtk_widget_show(page);

        /* The current pattern set: */
        page = gtk_vbox_new(FALSE, 0);
        hub_pattern_set_widgets(page, xg);
        gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("Patterns"));
        gtk_widget_show(page);

        /* The error graph and training interface: */
        page = gtk_vbox_new(FALSE, 0);
        hub_train_create_widgets(page, xg);
        gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("Train"));
        gtk_widget_show(page);

        /* Network testing view: */
        page = gtk_vbox_new(FALSE, 0);
        hub_explore_create_widgets(page, xg);
        gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("Explore"));
        gtk_widget_show(page);

        /* Graphs based on Tyler et al., 2000: */
        page = gtk_vbox_new(FALSE, 0);
        hub_lesion_create_widgets(page, xg);
        gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("Lesion Studies"));
        gtk_widget_show(page);

        /* End of notebook pages */
        gtk_notebook_set_page(GTK_NOTEBOOK(notes), 0);
        gtk_widget_show(notes);
 
        /* And position the window: */
        gtk_window_set_position(GTK_WINDOW(xg->frame), GTK_WIN_POS_CENTER);

        return(TRUE);
    }
}

/******************************************************************************/
/* Initialise the network and pattern set: ************************************/

void xg_initialise(XGlobals *xg)
{
    /* Load the training set (and initialise the current pattern): */
    xpattern_set_read(xg, PATTERN_SET);

    /* And load a set of trained weights: */
    weight_file_read(xg, WEIGHT_FILE);
}

/******************************************************************************/
