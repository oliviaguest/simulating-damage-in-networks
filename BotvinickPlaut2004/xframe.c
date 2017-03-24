
#include <time.h>

#include "xframe.h"

extern GtkWidget *xnet_diagram_create();
extern void xgraph_create(GtkWidget *vbox);
extern void xgraph_set_error_scores();
extern void xgraph_initialise();

extern void action_list_clear();

extern void test_net_create_widgets(GtkWidget *vbox);
extern void test_net_redraw_all_views();

extern void create_attractor_viewer(GtkWidget *vbox);
extern void create_sim1_viewer(GtkWidget *vbox);
extern void create_cs_sim1_table(GtkWidget *vbox, BaseTaskType task);
extern void create_cs_sim2_table(GtkWidget *vbox);
extern void create_cs_sim3_viewer(GtkWidget *vbox);
extern void create_cs_sim4_viewer(GtkWidget *vbox);
extern void create_cs_sim5_table(GtkWidget *vbox);
extern void create_cs_sim5_graph(GtkWidget *vbox);
extern void create_cs_sim6_table(GtkWidget *vbox);
extern void create_bp_sim2_survival_plot_viewer(GtkWidget *vbox);
extern void create_bp_sim2_subtask_chart_viewer(GtkWidget *vbox);
extern void create_bp_independents_chart_viewer(GtkWidget *vbox);
extern void create_bp_error_frequencies_viewer(GtkWidget *vbox);
extern void create_bp_errors_per_trial_viewer(GtkWidget *vbox);
extern GtkWidget *create_jb_analysis_page();

XGlobals xg =   {NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        0,
                        0,
                        {},
                        {},
                        FALSE,
                        FALSE};

Parameters pars = {0.001,            /* Default learning rate      */
                   CROSS_ENTROPY,    /* Default error function     */
                   UPDATE_BY_ITEM,   /* Default weight update time */
		   FALSE,            /* Dont use the BP penalty term    */
                   TRUE,             /* Assume sugar bowl closed   */
                   FALSE,            /* Mug doesn't contain coffee */
                   FALSE,            /* Mug doesn't contain tea    */
                   FALSE,            /* Mug doesn't contain cream  */
                   FALSE};           /* Mug doesn't contain sugar  */

/******************************************************************************/
/* Callback to exit/kill the graphics interface *******************************/

static void exit_bp(GtkWidget *button, void *data)
{
    training_set_free(xg.first);
    network_tell_destroy(xg.net);
    gtk_main_quit();
}

/******************************************************************************/

void initialise_widgets()
{
    /* Initialise the error measures: */
    xg.cycle = 0;
    xgraph_initialise();
    /* Clear/redraw the state viewers: */
    test_net_redraw_all_views();
    /* Clear the action log: */
    action_list_clear();
}

/******************************************************************************/
/* Callback to load a set of weights ******************************************/

static void destroy_chooser(GtkWidget *button, GtkWidget *chooser)
{
    gtk_widget_destroy(chooser);
}

static void select_weight_file(GtkWidget *w, GtkWidget *chooser)
{
    char *file = (char *)gtk_file_selection_get_filename(GTK_FILE_SELECTION(chooser));
    char buffer[128];
    FILE *fp;
    int j;

    if ((fp = fopen(file, "r")) == NULL) {
        g_snprintf(buffer, 128, "ERROR: Cannot read file ... weights not restored");
    }
    else if ((j = network_restore_weights(fp, xg.net)) > 0) {
        g_snprintf(buffer, 128, "ERROR: Weight file format error %d ... weights not restored", j);
        fclose(fp);
    }
    else {
        gtk_label_set_text(GTK_LABEL(xg.weight_history_label), file);
        g_snprintf(buffer, 128, "Weights successfully restored from\n%s", file);

        /* Initialise the graph and various viewers: */
        xgraph_set_error_scores();
        initialise_widgets();

        fclose(fp);
    }
    gtkx_warn(chooser, buffer);
    gtk_widget_destroy(chooser);
}

static void load_weights_callback(GtkWidget *mi, GtkWidget *frame)
{
    GtkWidget *chooser = gtk_file_selection_new("Select weight file");
    gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(chooser));
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(chooser)->ok_button), "clicked", G_CALLBACK(select_weight_file), chooser);
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(chooser)->cancel_button), "clicked", G_CALLBACK(destroy_chooser), chooser);
    gtkx_position_popup(frame, chooser);
    gtk_widget_show(chooser);
}

/******************************************************************************/
/* Callback to save a set of weights ******************************************/

static Boolean weight_file_dump_header_info(FILE *fp, char *training_data, int epochs)
{
    fprintf(fp, "%% Training set: %s\n", training_data);
    fprintf(fp, "%% Learning rate: %7.5f\n", pars.lr);
    fprintf(fp, "%% Penalty: %s\n", (pars.penalty == TRUE) ? "TRUE" : "FALSE");
    fprintf(fp, "%% Update time: %s\n", (pars.wut == UPDATE_BY_ITEM) ? "By item" : "By epoch");
    fprintf(fp, "%% Epochs: %d\n", epochs);
    return(TRUE);
}

static void save_weights_to_file(GtkWidget *w, GtkWidget *chooser)
{
    char *file = (char *)gtk_file_selection_get_filename(GTK_FILE_SELECTION(chooser));
    char buffer[128];
    FILE *fp;

    if ((fp = fopen(file, "w")) == NULL) {
        g_snprintf(buffer, 128, "ERROR: Cannot write to %s", file);
    }
    else if (!weight_file_dump_header_info(fp, TRAINING_SET, xg.cycle)) {
        g_snprintf(buffer, 128, "ERROR: Cannot dump weight header information ... weights not saved");
        fclose(fp);
        remove(file);
    }
    else if (!network_dump_weights(fp, xg.net)) {
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

static void save_weights_callback(GtkWidget *mi, GtkWidget *frame)
{
    GtkWidget *chooser = gtk_file_selection_new("Save weight file");
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(chooser)->ok_button), "clicked", G_CALLBACK(save_weights_to_file), chooser);
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(chooser)->cancel_button), "clicked", G_CALLBACK(destroy_chooser), chooser);
    gtkx_position_popup(frame, chooser);
    gtk_widget_show(chooser);
}

/******************************************************************************/

static void initialise_weights_callback(GtkWidget *button, GtkWidget *frame)
{
    TaskType task;

    /* Initialise the world and network: */
    xg.step = 0;
    task.base = TASK_NONE;
    task.initial_state.bowl_closed = pars.sugar_closed;
    task.initial_state.mug_contains_coffee = pars.mug_with_coffee;
    task.initial_state.mug_contains_tea = pars.mug_with_tea;
    task.initial_state.mug_contains_cream = pars.mug_with_cream;
    task.initial_state.mug_contains_sugar = pars.mug_with_sugar;
    world_initialise(&task);
    gtk_label_set_text(GTK_LABEL(xg.weight_history_label), "Randomised");
    network_tell_initialise(xg.net);
    /* Initialise the graph and various viewers: */
    initialise_widgets();
}

/******************************************************************************/
/* Callback to load a set of patterns/sequences *******************************/

static TrainingDataList *xtraining_set_read(char *file, int in_width, int out_width)
{
    TrainingDataList *tmp = NULL;

    if ((tmp = training_set_read(file, in_width, out_width)) != NULL) {
        gtk_label_set_text(GTK_LABEL(xg.training_set_label), file);
    }
    else {
        gtk_label_set_text(GTK_LABEL(xg.training_set_label), "NONE");
    }
    return(tmp);
}

int reload_training_data(char *file)
{
    training_set_free(xg.first);
    xg.first = xtraining_set_read(file, IN_WIDTH, OUT_WIDTH);
    return(training_set_length(xg.first));
}

static void select_training_file(GtkWidget *w, GtkWidget *chooser)
{
    char *file = (char *)gtk_file_selection_get_filename(GTK_FILE_SELECTION(chooser));
    char buffer[128];

    g_snprintf(buffer, 128, "%d item(s) read from %s", reload_training_data(file), file);
    xg.next = xg.first;
    gtkx_warn(chooser, buffer);
    gtk_widget_destroy(chooser);
}

static void load_patterns_callback(GtkWidget *mi, GtkWidget *frame)
{
    GtkWidget *chooser = gtk_file_selection_new("Select training file");
    gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(chooser));
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(chooser)->ok_button), "clicked", G_CALLBACK(select_training_file), chooser);
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(chooser)->cancel_button), "clicked", G_CALLBACK(destroy_chooser), chooser);
    gtkx_position_popup(frame, chooser);
    gtk_widget_show(chooser);
}

/******************************************************************************/

static void populate_weight_history_page(GtkWidget *page)
{
    GtkWidget *tmp, *hbox;
    char buffer[16];

    /*--- The Patterns widgets: ---*/

    hbox = gtk_hbox_new(FALSE, 0);

    tmp = gtk_label_new("Training Patterns:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    tmp = gtk_button_new_with_label("  Load  ");
    g_signal_connect(G_OBJECT(tmp), "clicked", G_CALLBACK(load_patterns_callback), xg.frame);
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
    xg.training_set_label = gtk_label_new("NONE");
    gtk_box_pack_start(GTK_BOX(hbox), xg.training_set_label, FALSE, FALSE, 5);
    gtk_widget_show(xg.training_set_label);
    /*---Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /*--- Separator : ---*/

    tmp = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(page), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);

    /*--- The Weight History widgets: ---*/

    hbox = gtk_hbox_new(FALSE, 0);

    tmp = gtk_label_new("Weight history:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    tmp = gtk_button_new_with_label("  Save  ");
    g_signal_connect(G_OBJECT(tmp), "clicked", G_CALLBACK(save_weights_callback), xg.frame);
    gtk_box_pack_end(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    tmp = gtk_button_new_with_label("  Load  ");
    g_signal_connect(G_OBJECT(tmp), "clicked", G_CALLBACK(load_weights_callback), xg.frame);
    gtk_box_pack_end(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    tmp = gtk_button_new_with_label("  Randomise  ");
    g_signal_connect(G_OBJECT(tmp), "clicked", G_CALLBACK(initialise_weights_callback), xg.frame);
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
    xg.weight_history_label = gtk_label_new("Randomised");
    gtk_box_pack_start(GTK_BOX(hbox), xg.weight_history_label, FALSE, FALSE, 5);
    gtk_widget_show(xg.weight_history_label);
    /*---Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    hbox = gtk_hbox_new(FALSE, 0);
    /*--- The label: ---*/
    tmp = gtk_label_new("    Cycles since initialisation:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    g_snprintf(buffer, 16, "%d", xg.cycle);
    xg.weight_history_cycles = gtk_label_new(buffer);
    gtk_box_pack_start(GTK_BOX(hbox), xg.weight_history_cycles, FALSE, FALSE, 5);
    gtk_widget_show(xg.weight_history_cycles);
    /*---Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /*--- Separator : ---*/

    tmp = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(page), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
}

/******************************************************************************/

static void callback_spin_lr(GtkWidget *widget, GtkSpinButton *tmp)
{
    pars.lr = gtk_spin_button_get_value(tmp);
}

static void callback_toggle_ef(GtkWidget *button, void *data)
{
    if (GTK_TOGGLE_BUTTON(button)->active) {
        pars.ef = (ErrorFunction) data;
    }
}

static void callback_toggle_wut(GtkWidget *button, void *data)
{
    if (GTK_TOGGLE_BUTTON(button)->active) {
        pars.wut = (WeightUpdateTime) data;
    }
}

static void callback_toggle_penalty(GtkWidget *button, void *data)
{
    pars.penalty = (Boolean) (GTK_TOGGLE_BUTTON(button)->active);
}

static void callback_toggle_boolean_parameter(GtkWidget *button, Boolean *data)
{
    *data = (Boolean) (GTK_TOGGLE_BUTTON(button)->active);
}

static void populate_parameters_page(GtkWidget *page)
{
    GtkWidget *hbox, *tmp, *button;
    GtkAdjustment *adj;

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
    adj = (GtkAdjustment *)gtk_adjustment_new(pars.lr, 0.0, 1.0, 0.001, 0.01, 0);
    tmp = gtk_spin_button_new(adj, 0.1, 5);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tmp), TRUE);
    gtk_widget_set_size_request(GTK_WIDGET(tmp), 75, -1);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(adj), "value_changed", G_CALLBACK(callback_spin_lr), tmp);
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
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), (pars.ef == SUM_SQUARE_ERROR));
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(callback_toggle_ef), (void *)SUM_SQUARE_ERROR);
    gtk_widget_show(button);
    /*--- The second error function radio button: ---*/
    button = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(button)), "Cross Entropy");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), (pars.ef == CROSS_ENTROPY));
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(callback_toggle_ef), (void *)CROSS_ENTROPY);
    gtk_widget_show(button);
    /*--- The third error function radio button: ---*/
    button = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(button)), "SoftMax");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), (pars.ef == SOFT_MAX_ERROR));
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(callback_toggle_ef), (void *)SOFT_MAX_ERROR);
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
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), (pars.wut == UPDATE_BY_ITEM));
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(callback_toggle_wut), (void *)UPDATE_BY_ITEM);
    gtk_widget_show(button);
    /*--- The second update time radio button: ---*/
    button = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(button)), "by epoch");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), (pars.wut == UPDATE_BY_EPOCH));
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(callback_toggle_wut), (void *)UPDATE_BY_EPOCH);
    gtk_widget_show(button);
    /*--- Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /**** Widgets for selecting the penalty term: ****/

    hbox = gtk_hbox_new(FALSE, 0);
    /*--- The label: ---*/
    tmp = gtk_label_new("    Hidden unit penalty:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    /*--- The check button: ---*/
    tmp = gtk_check_button_new();
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), pars.penalty);
    g_signal_connect(G_OBJECT(tmp), "toggled", G_CALLBACK(callback_toggle_penalty), tmp);
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
    tmp = gtk_label_new("Miscellaneous parameters:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    hbox = gtk_hbox_new(FALSE, 0);
    /*--- The label: ---*/
    tmp = gtk_label_new("    Sugar bowl initially closed?");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    /*--- The check button: ---*/
    tmp = gtk_check_button_new();
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), pars.sugar_closed);
    g_signal_connect(G_OBJECT(tmp), "toggled", G_CALLBACK(callback_toggle_boolean_parameter), &(pars.sugar_closed));
    gtk_widget_show(tmp);
    /*--- Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    hbox = gtk_hbox_new(FALSE, 0);
    /*--- The label: ---*/
    tmp = gtk_label_new("    Mug initially contains coffee?");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    /*--- The check button: ---*/
    tmp = gtk_check_button_new();
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), pars.mug_with_coffee);
    g_signal_connect(G_OBJECT(tmp), "toggled", G_CALLBACK(callback_toggle_boolean_parameter), &(pars.mug_with_coffee));
    gtk_widget_show(tmp);
    /*--- Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    hbox = gtk_hbox_new(FALSE, 0);
    /*--- The label: ---*/
    tmp = gtk_label_new("    Mug initially contains tea?");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    /*--- The check button: ---*/
    tmp = gtk_check_button_new();
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), pars.mug_with_tea);
    g_signal_connect(G_OBJECT(tmp), "toggled", G_CALLBACK(callback_toggle_boolean_parameter), &(pars.mug_with_tea));
    gtk_widget_show(tmp);
    /*--- Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    hbox = gtk_hbox_new(FALSE, 0);
    /*--- The label: ---*/
    tmp = gtk_label_new("    Mug initially contains cream?");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    /*--- The check button: ---*/
    tmp = gtk_check_button_new();
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), pars.mug_with_cream);
    g_signal_connect(G_OBJECT(tmp), "toggled", G_CALLBACK(callback_toggle_boolean_parameter), &(pars.mug_with_cream));
    gtk_widget_show(tmp);
    /*--- Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    hbox = gtk_hbox_new(FALSE, 0);
    /*--- The label: ---*/
    tmp = gtk_label_new("    Mug initially contains sugar?");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    /*--- The check button: ---*/
    tmp = gtk_check_button_new();
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), pars.mug_with_sugar);
    g_signal_connect(G_OBJECT(tmp), "toggled", G_CALLBACK(callback_toggle_boolean_parameter), &(pars.mug_with_sugar));
    gtk_widget_show(tmp);
    /*--- Pack and show the whole horizontal box ---*/
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 5);
    gtk_widget_show(hbox);

    /*--- Separator : ---*/

    tmp = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(page), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
}

/******************************************************************************/
/* The analyses notebooks *****************************************************/

static GtkWidget *create_bp_analysis_notebook()
{
    GtkWidget *notes, *page, *vbox;

    vbox = gtk_vbox_new(FALSE, 0);

    notes = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(vbox), notes);

    /* Page C1: Simulation 1 result viewer: */

    page = gtk_vbox_new(FALSE, 0);
    create_sim1_viewer(page);
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("T4: Sequences"));
    gtk_widget_show(page);

    /* Page C2: Simulation 2 displacement bar chart: */

    page = gtk_vbox_new(FALSE, 0);
    create_bp_sim2_subtask_chart_viewer(page);
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("F7: Subtasks"));
    gtk_widget_show(page);

    /* Page C3: Simulation 2 survivial plot viewer: */

    page = gtk_vbox_new(FALSE, 0);
    create_bp_sim2_survival_plot_viewer(page);
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("F8: Survival plot"));
    gtk_widget_show(page);

    /* Page C4: Simulation 3 independent proportion bar chart: */

    page = gtk_vbox_new(FALSE, 0);
    create_bp_independents_chart_viewer(page);
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("Independents"));
    gtk_widget_show(page);

    /* Page C5: Simulation 3 independent proportion bar chart: */

    page = gtk_vbox_new(FALSE, 0);
    create_bp_error_frequencies_viewer(page);
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("T6: Error frequencies"));
    gtk_widget_show(page);

    /* Page C6: Simulation 3 sequence/omission bar chart: */

    page = gtk_vbox_new(FALSE, 0);
    create_bp_errors_per_trial_viewer(page);
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("F15: Errors per trial"));
    gtk_widget_show(page);

    gtk_widget_show(notes);

    return(vbox);
}

static GtkWidget *create_cs_analysis_notebook()
{
    GtkWidget *notes, *page, *vbox;

    vbox = gtk_vbox_new(FALSE, 0);

    notes = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(vbox), notes);

    /* Page C1a: Simulation 1 result viewer: */

    page = gtk_vbox_new(FALSE, 0);
    create_cs_sim1_table(page, TASK_TEA);
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("Sim. 1 (Tea)"));
    gtk_widget_show(page);

    /* Page C1a: Simulation 1 result viewer: */

    page = gtk_vbox_new(FALSE, 0);
    create_cs_sim1_table(page, TASK_COFFEE);
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("Sim. 1 (Coffee)"));
    gtk_widget_show(page);

    /* Page C2: Simulation 2 result table: */

    page = gtk_vbox_new(FALSE, 0);
    create_cs_sim2_table(page);
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("Sim. 2 (Table)"));
    gtk_widget_show(page);

    /* Page C3: Simulation 3 result viewer: */

    page = gtk_vbox_new(FALSE, 0);
    create_cs_sim3_viewer(page);
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("Sim. 3 (Table)"));
    gtk_widget_show(page);

    /* Page C4: Simulation 4 result viewer: */

    page = gtk_vbox_new(FALSE, 0);
    create_cs_sim4_viewer(page);
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("Sim. 4 (Table)"));
    gtk_widget_show(page);

    /* Page C5a: Simulation 5 result table: */

    page = gtk_vbox_new(FALSE, 0);
    create_cs_sim5_table(page);
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("Sim. 5 (Table)"));
    gtk_widget_show(page);

    /* Page C5b: Simulation 5 result graph: */

    page = gtk_vbox_new(FALSE, 0);
    create_cs_sim5_graph(page);
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("Sim. 5 (Graph)"));
    gtk_widget_show(page);

    /* Page C6: Simulation 6 result table: */

    page = gtk_vbox_new(FALSE, 0);
    create_cs_sim6_table(page);
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("Sim. 6 (Table)"));
    gtk_widget_show(page);

    gtk_widget_show(notes);

    return(vbox);
}

/* The top-level widget creation routine **************************************/

GtkWidget *make_widgets()
{
    GtkWidget *page, *notes;

    xg.frame = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_container_set_border_width(GTK_CONTAINER(xg.frame), 0);
//    gtk_widget_set_size_request(xg.frame, 1100, 790);
    gtk_window_set_title(GTK_WINDOW(xg.frame), "Botvinick/Plaut SRN");
    g_signal_connect(G_OBJECT(xg.frame), "destroy", G_CALLBACK(exit_bp), NULL);

    /* The notebook: */

    notes = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(xg.frame), notes);

    /* Page A0: The network diagram: */

    page = xnet_diagram_create();
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("Diagram"));
    gtk_widget_show(page);

    /* Page A1: Configuration - weights, training and parameters: */

    page = gtk_vbox_new(FALSE, 0);
    populate_weight_history_page(page);
    populate_parameters_page(page);
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("Configuration"));
    gtk_widget_show(page);

    /* Page A2: The error graph and training interface: */

    page = gtk_vbox_new(FALSE, 0);
    xgraph_create(page);
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("Train"));
    gtk_widget_show(page);

    /* Page A3: Notebook of various network testing views: */

    page = gtk_vbox_new(FALSE, 0);
    test_net_create_widgets(page);
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("Test"));
    gtk_widget_show(page);

    /* Page A4: Across trial hidden unit viewer: */

    page = gtk_vbox_new(FALSE, 0);
    create_attractor_viewer(page);
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("Attractors"));
    gtk_widget_show(page);

    /* Page A5: BP Analyses notebook: */

    page = create_bp_analysis_notebook();
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("BP Analyses"));
    gtk_widget_show(page);

    /* Page A6: CS Analyses notebook: */

    page = create_cs_analysis_notebook();
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("CS Analyses"));
    gtk_widget_show(page);

    /* Page A7: Jeff Bower Analysis notebook: */

    page = create_jb_analysis_page();
    gtk_notebook_append_page(GTK_NOTEBOOK(notes), page, gtk_label_new("JB Analysis"));
    gtk_widget_show(page);

    /* End of notebook pages */

    gtk_widget_show(notes);
 
    xg.first = xtraining_set_read(TRAINING_SET, IN_WIDTH, OUT_WIDTH);
    xg.net = network_create(IN_WIDTH, HIDDEN_WIDTH, OUT_WIDTH);

    gtk_window_set_position(GTK_WINDOW(xg.frame), GTK_WIN_POS_CENTER);
    return(xg.frame);
}

/******************************************************************************/
