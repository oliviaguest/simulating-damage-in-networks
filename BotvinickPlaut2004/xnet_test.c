
#include "xframe.h"

extern double vector_maximum_activation(double *vector_out, int width);

extern void action_list_clear();
extern void action_list_report(Boolean success, char *world_error_string);
extern void action_list_create(GtkWidget *page);

extern void acs1_transcript1_create(GtkWidget *page);
extern void acs1_transcript2_create(GtkWidget *page);

extern Boolean net_2d_viewer_expose(GtkWidget *widget, GdkEvent *event, void *data);
extern void net_2d_viewer_create(GtkWidget *vbox);
extern Boolean net_3d_viewer_expose(GtkWidget *widget, GdkEvent *event, void *data);
extern void net_3d_viewer_create(GtkWidget *vbox);
extern Boolean net_output_viewer_expose(GtkWidget *widget, GdkEvent *event, void *data);
extern void net_output_viewer_create(GtkWidget *vbox);
extern Boolean world_state_viewer_expose(GtkWidget *widget, GdkEvent *event, void *data);
extern void world_state_viewer_create(GtkWidget *vbox);

extern void analyse_actions_with_acs1(TaskType *task, ACS1 *results);
extern GList *analyse_actions_with_acs2(TaskType *task, ACS2 *results);
extern void analyse_actions_code_free_error_list(GList *errors);
extern void print_analysis_acs1(ACS1 *results);
extern void print_analysis_acs2(ACS2 *results, GList *errors);
extern void action_log_print();

static double test_threshold = 0.0;
static double test_noise = 0.000;
static int lesion_severity = 0;
static TaskType test_state = {TASK_NONE, DAMAGE_NONE, {TRUE, FALSE, FALSE, FALSE, FALSE}};

/******************************************************************************/

void test_net_redraw_all_views()
{
    net_2d_viewer_expose(NULL, NULL, NULL);
    net_3d_viewer_expose(NULL, NULL, NULL);
    net_output_viewer_expose(NULL, NULL, NULL);
    world_state_viewer_expose(NULL, NULL, NULL);
}

/******************************************************************************/
/* Functions for creating/maintaining the results of BP Simulation 1 **********/

void initialise_state(TaskType *task)
{
    double *vector_in = (double *)malloc(IN_WIDTH * sizeof(double));

    /* Initialise the state of the world and the hidden layer: */
    xg.step = 0;
    world_initialise(task);
    network_tell_randomise_hidden_units(xg.net);
    /* Present the state of the world to the input units: */
    world_set_network_input_vector(vector_in);
    network_tell_input(xg.net, vector_in);
    free(vector_in);
    /* Clear/redraw the state viewers: */
    test_net_redraw_all_views();
    /* And set the initial task/state: */
    test_state.base = task->base;
    test_state.initial_state.bowl_closed = task->initial_state.bowl_closed;
    test_state.initial_state.mug_contains_coffee = task->initial_state.mug_contains_coffee;
    test_state.initial_state.mug_contains_tea = task->initial_state.mug_contains_tea;
    test_state.initial_state.mug_contains_cream = task->initial_state.mug_contains_cream;
    test_state.initial_state.mug_contains_sugar = task->initial_state.mug_contains_sugar;
}

/******************************************************************************/
/* Callbacks associated with the testing notebook *****************************/

static void record_step_output()
{
    if (xg.step < 100) {
        double *vector_out = (double *)malloc(OUT_WIDTH * sizeof(double));
        int j;

        network_ask_output(xg.net, vector_out);
        for (j = 0; j < OUT_WIDTH; j++) {
            xg.output[xg.step][j] = vector_out[j];
        }
        xg.conflict[xg.step] = vector_net_conflict(vector_out, OUT_WIDTH);
        free(vector_out);
    }
}

static void initialise_coffee_state_callback(GtkWidget *button, GtkWidget *frame)
{
    TaskType task;

    task.base = TASK_COFFEE;
    task.initial_state.bowl_closed = pars.sugar_closed;
    task.initial_state.mug_contains_coffee = pars.mug_with_coffee;
    task.initial_state.mug_contains_tea = pars.mug_with_tea;
    task.initial_state.mug_contains_cream = pars.mug_with_cream;
    task.initial_state.mug_contains_sugar = pars.mug_with_sugar;

    action_list_clear();
    initialise_state(&task);
    action_list_report(TRUE, "INITIALISE COFFEE TASK");
}

static void initialise_tea_state_callback(GtkWidget *button, GtkWidget *frame)
{
    TaskType task;

    task.base = TASK_TEA;
    task.initial_state.bowl_closed = pars.sugar_closed;
    task.initial_state.mug_contains_coffee = pars.mug_with_coffee;
    task.initial_state.mug_contains_tea = pars.mug_with_tea;
    task.initial_state.mug_contains_cream = pars.mug_with_cream;
    task.initial_state.mug_contains_sugar = pars.mug_with_sugar;

    action_list_clear();
    initialise_state(&task);
    action_list_report(TRUE, "INITIALISE TEA TASK");
}

static void initialise_state_callback(GtkWidget *button, GtkWidget *frame)
{
    TaskType task;

    task.base = TASK_NONE;
    task.initial_state.bowl_closed = pars.sugar_closed;
    task.initial_state.mug_contains_coffee = pars.mug_with_coffee;
    task.initial_state.mug_contains_tea = pars.mug_with_tea;
    task.initial_state.mug_contains_cream = pars.mug_with_cream;
    task.initial_state.mug_contains_sugar = pars.mug_with_sugar;

    action_list_clear();
    initialise_state(&task);
    action_list_report(TRUE, "INITIALISE NULL TASK");
}

static void act_once_callback(GtkWidget *button, GtkWidget *frame)
{
    double *vector_in = (double *)malloc(IN_WIDTH * sizeof(double));
    double *vector_out = (double *)malloc(OUT_WIDTH * sizeof(double));
    ActionType action;

    world_set_network_input_vector(vector_in);
    network_tell_input(xg.net, vector_in);
    network_tell_propagate2(xg.net);
    network_ask_output(xg.net, vector_out);
    record_step_output();
    xg.step++;

    if (vector_maximum_activation(vector_out, OUT_WIDTH) > test_threshold) {
        action = world_get_network_output_action(NULL, vector_out);
        action_list_report(world_perform_action(action), world_error_string);
    }

    /* Now add the noise to the hidden units: */
    network_inject_noise(xg.net, test_noise*test_noise);

    test_net_redraw_all_views();

    free(vector_in);
    free(vector_out);
}

static void act_many_callback(GtkWidget *button, GtkWidget *frame)
{
    double *vector_in = (double *)malloc(IN_WIDTH * sizeof(double));
    double *vector_out = (double *)malloc(OUT_WIDTH * sizeof(double));
    ACS1 acs1_results;
    ACS2 acs2_results;
    int count = 100;
    ActionType action;
    GList      *errors = NULL;

    do {
        world_set_network_input_vector(vector_in);
        network_tell_input(xg.net, vector_in);
        network_tell_propagate2(xg.net);
        network_ask_output(xg.net, vector_out);
        record_step_output();
        xg.step++;
        if (vector_maximum_activation(vector_out, OUT_WIDTH) > test_threshold) {
            action = world_get_network_output_action(NULL, vector_out);
            action_list_report(world_perform_action(action), world_error_string);
	}
	else {
	    action = ACTION_NONE;
	}
	/* Now add the noise to the hidden units: */
	network_inject_noise(xg.net, test_noise*test_noise);

        test_net_redraw_all_views();
    } while ((count-- > 0) && (action != ACTION_SAY_DONE));

    analyse_actions_with_acs1(&test_state, &acs1_results);
    action_log_print();
    print_analysis_acs1(&acs1_results);

    errors = analyse_actions_with_acs2(&test_state, &acs2_results);
    print_analysis_acs2(&acs2_results, errors);
    analyse_actions_code_free_error_list(errors);

    free(vector_in);
    free(vector_out);
}

static void lesion_ih_callback(GtkWidget *mi, GtkWidget *frame)
{
    network_lesion_weights_ih(xg.net, lesion_severity / 100.0);
}

static void lesion_ch_callback(GtkWidget *mi, GtkWidget *frame)
{
    network_lesion_weights_ch(xg.net, lesion_severity / 100.0);
}

static void scale_weights_callback(GtkWidget *mi, GtkWidget *frame)
{
    network_scale_weights(xg.net, 1.0 - (lesion_severity / 100.0));
}

static void callback_spin_noise(GtkWidget *widget, GtkSpinButton *tmp)
{
    test_noise = gtk_spin_button_get_value(tmp);
}

static void callback_spin_threshold(GtkWidget *widget, GtkSpinButton *tmp)
{
    test_threshold = gtk_spin_button_get_value(tmp);
}

static void callback_spin_lesion_level(GtkWidget *widget, GtkSpinButton *tmp)
{
    lesion_severity = gtk_spin_button_get_value_as_int(tmp);
}

static void callback_toggle_colour(GtkWidget *button, void *data)
{
    xg.colour = (Boolean) (GTK_TOGGLE_BUTTON(button)->active);
    test_net_redraw_all_views();
}

/******************************************************************************/

void test_net_create_widgets(GtkWidget *vbox)
{
    /* The buttons for testing mode: */

    GtkWidget *notebook, *tmp, *page, *hbox;
    GtkAdjustment *adj;

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    gtkx_button_create(hbox, "Initialise Coffee", G_CALLBACK(initialise_coffee_state_callback), xg.frame);
    gtkx_button_create(hbox, "Initialise Tea", G_CALLBACK(initialise_tea_state_callback), xg.frame);
    gtkx_button_create(hbox, "Initialise Neither", G_CALLBACK(initialise_state_callback), xg.frame);

    /* The "propagate" buttons: */

    gtkx_button_create(hbox, " > ", G_CALLBACK(act_once_callback), xg.frame);
    gtkx_button_create(hbox, " >> ", G_CALLBACK(act_many_callback), xg.frame);

    /* Widgets for setting colour/greyscale: */
    /*--- The checkbox: ---*/
    tmp = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), xg.colour);
    gtk_box_pack_end(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(tmp), "toggled", G_CALLBACK(callback_toggle_colour), tmp);
    gtk_widget_show(tmp);
    /*--- The label: ---*/
    tmp = gtk_label_new("Colour:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    hbox = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    gtkx_button_create(hbox, "Lesion: I -> H", G_CALLBACK(lesion_ih_callback), xg.frame);
    gtkx_button_create(hbox, "Lesion: C -> H", G_CALLBACK(lesion_ch_callback), xg.frame);
    gtkx_button_create(hbox, "Scale Weights", G_CALLBACK(scale_weights_callback), xg.frame);
    /* Widgets for adjusting the lesion severity: */
    /*--- The label: ---*/
    tmp = gtk_label_new("Severity (%):");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    /*--- The spin button: ---*/
    adj = (GtkAdjustment *)gtk_adjustment_new(lesion_severity, 0.0, 100.0, 1.0, 1.0, 0);
    tmp = gtk_spin_button_new(adj, 1, 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tmp), TRUE);
    gtk_widget_set_size_request(GTK_WIDGET(tmp), 42, -1);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(adj), "value_changed", G_CALLBACK(callback_spin_lesion_level), tmp);
    gtk_widget_show(tmp);

    /* Widgets for adjusting the context noise: */
    /*--- The label: ---*/
    tmp = gtk_label_new("Noise (s.d.):");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    /*--- The spin button: ---*/
    adj = (GtkAdjustment *)gtk_adjustment_new(test_noise, 0.0, 1.0, 0.01, 0.10, 0);
    tmp = gtk_spin_button_new(adj, 0.1, 4);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tmp), TRUE);
    gtk_widget_set_size_request(GTK_WIDGET(tmp), 60, -1);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(adj), "value_changed", G_CALLBACK(callback_spin_noise), tmp);
    gtk_widget_show(tmp);

    /* Widgets for adjusting the threshold: */
    /*--- The label: ---*/
    tmp = gtk_label_new("Threshold:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    /*--- The spin button: ---*/
    adj = (GtkAdjustment *)gtk_adjustment_new(test_noise, 0.0, 1.0, 0.01, 0.10, 0);
    tmp = gtk_spin_button_new(adj, 0.1, 2);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tmp), TRUE);
    gtk_widget_set_size_request(GTK_WIDGET(tmp), 48, -1);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(adj), "value_changed", G_CALLBACK(callback_spin_threshold), tmp);
    gtk_widget_show(tmp);

    notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

    /* Page B1: The 2D network view: */

    page = gtk_vbox_new(FALSE, 0);
    net_2d_viewer_create(page);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, gtk_label_new("2D Network View"));
    gtk_widget_show(page);

    /* Page B2: The 3D network view: */

    page = gtk_vbox_new(FALSE, 0);
    net_3d_viewer_create(page);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, gtk_label_new("3D Network View"));
    gtk_widget_show(page);

    /* Page B3: The 2D output vector view: */

    page = gtk_vbox_new(FALSE, 0);
    net_output_viewer_create(page);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, gtk_label_new("Output View"));
    gtk_widget_show(page);

    /* Page B4: State of the World: */

    page = gtk_vbox_new(FALSE, 0);
    world_state_viewer_create(page);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, gtk_label_new("World State"));
    gtk_widget_show(page);

    /* Page B5: Actions attempted: */

    page = gtk_vbox_new(FALSE, 0);
    action_list_create(page);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, gtk_label_new("Actions"));
    gtk_widget_show(page);

    /* Page B6: Action transcript (a la Table 5 in BP2004): */

    page = gtk_vbox_new(FALSE, 0);
    acs1_transcript1_create(page);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, gtk_label_new("ACS1 Coding"));
    gtk_widget_show(page);

    /* Page B7: Analyses: */

    page = gtk_vbox_new(FALSE, 0);
    acs1_transcript2_create(page);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, gtk_label_new("Analyses"));
    gtk_widget_show(page);

    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_BOTTOM);

    gtk_widget_show(notebook);
}

/******************************************************************************/
