
#include "xframe.h"
#include "lib_string.h"
#include "lib_cairox.h"
#include "lib_cairoxg_2_3.h"
#include "xcs_sequences.h"

#define ERROR_CATEGORIES  4
#define MAX_LEVELS 11
#define MAX_STEPS 100

static GtkWidget *viewer_widget = NULL;
static cairo_surface_t *viewer_surface = NULL;

static TaskType sc_task = {TASK_COFFEE, DAMAGE_ACTIVATION_NOISE, {FALSE, FALSE, FALSE, FALSE, FALSE}};
static double sc_data[MAX_LEVELS][ERROR_CATEGORIES];
static int sc_count = 0;

typedef struct damage_details {
    char *label;
    int num_levels;
    double level[MAX_LEVELS];
    char *format;
} DamageDetails;

static DamageDetails sc_dd[3] = {
    {"Activation Noise (s.d.)",    10, { 0.00,  0.01,  0.02,  0.03,  0.04,  0.05,  0.10,  0.20,  0.30,  0.40,  0.00}, "%4.2f"},
    {"Weight Noise (s.d.)",         8, {0.000, 0.001, 0.002, 0.005, 0.010, 0.030, 0.050, 0.100, 0.000, 0.000,  0.00}, "%5.3f"},
    {"Connections Severed (proportion)",  9, {0.000, 0.001, 0.003, 0.006, 0.009, 0.015, 0.040, 0.080, 0.150, 0.000, 0.000}, "%5.3f"}
};

// Maximum number of errors on the vertical axis of the graph:
static int vertical_scale = 5;

extern void analyse_actions_with_acs1(TaskType *task, ACS1 *results);
extern GList *analyse_actions_with_acs2(TaskType *task, ACS2 *results);
extern void analyse_actions_code_free_error_list(GList *errors);

static GraphStruct *errors_per_trial_chart;

/******************************************************************************/

static void sc_run_and_score_activation_noise(Network *net, TaskType *task, int i)
{
    double    *vector_in;
    double    *vector_out;
    ActionType this[MAX_STEPS];
    ACS1       res1;
    ACS2       res2;
    int        count = 0;
    GList     *errors = NULL;

    vector_in = (double *)malloc(net->in_width * sizeof(double));
    vector_out = (double *)malloc(net->out_width * sizeof(double));

    world_initialise(task);
    network_tell_randomise_hidden_units(net);
    do {
	world_set_network_input_vector(vector_in);
	network_tell_input(net, vector_in);
	network_tell_propagate2(net);
	network_ask_output(net, vector_out);
	this[count] = world_get_network_output_action(NULL, vector_out);
	world_perform_action(this[count]);
	/* Now add the noise to the hidden units: */
	network_inject_noise(net, sc_dd[0].level[i]*sc_dd[0].level[i]);
    } while ((this[count] != ACTION_SAY_DONE) && (++count < MAX_STEPS));

    free(vector_in);
    free(vector_out);

    /* Now score the actions: */

    analyse_actions_with_acs1(task, &res1);
    errors = analyse_actions_with_acs2(task, &res2);
    analyse_actions_code_free_error_list(errors);

    sc_data[i][0] += res2.omissions;
    sc_data[i][1] += res2.anticipations;
    sc_data[i][1] += res2.perseverations;
    sc_data[i][1] += res2.reversals;
    sc_data[i][2] += res2.bizarre;
    sc_data[i][3] += res2.object_sub;
    sc_data[i][3] += res2.gesture_sub;
    sc_data[i][3] += res2.tool_omission;
    sc_data[i][3] += res2.action_addition;
    sc_data[i][3] += res2.quality;
}

static void sc_run_and_score_weight_noise(Network *net, TaskType *task, int i)
{
    double    *vector_in;
    double    *vector_out;
    ActionType this[MAX_STEPS];
    ACS1       res1;
    ACS2       res2;
    int        count = 0;
    GList     *errors = NULL;

    vector_in = (double *)malloc(net->in_width * sizeof(double));
    vector_out = (double *)malloc(net->out_width * sizeof(double));

    world_initialise(task);
    network_tell_randomise_hidden_units(net);
    /* Add noise to context to hidden connections: */
    network_perturb_weights_ch(net, sc_dd[1].level[i]);
    do {
	world_set_network_input_vector(vector_in);
	network_tell_input(net, vector_in);
	network_tell_propagate2(net);
	network_ask_output(net, vector_out);
	this[count] = world_get_network_output_action(NULL, vector_out);
	world_perform_action(this[count]);
    } while ((this[count] != ACTION_SAY_DONE) && (++count < MAX_STEPS));

    free(vector_in);
    free(vector_out);

    /* Now score the actions: */

    analyse_actions_with_acs1(task, &res1);
    errors = analyse_actions_with_acs2(task, &res2);
    analyse_actions_code_free_error_list(errors);

    sc_data[i][0] += res2.omissions;
    sc_data[i][1] += res2.anticipations;
    sc_data[i][1] += res2.perseverations;
    sc_data[i][1] += res2.reversals;
    sc_data[i][2] += res2.object_sub;
    sc_data[i][2] += res2.gesture_sub;
    sc_data[i][2] += res2.tool_omission;
    sc_data[i][2] += res2.action_addition;
    sc_data[i][2] += res2.quality;
    sc_data[i][3] += res2.bizarre;
}

static void sc_run_and_score_weight_lesion(Network *net, TaskType *task, int i)
{
    double    *vector_in;
    double    *vector_out;
    ActionType this[MAX_STEPS];
    ACS1       res1;
    ACS2       res2;
    int        count = 0;
    GList     *errors = NULL;

    vector_in = (double *)malloc(net->in_width * sizeof(double));
    vector_out = (double *)malloc(net->out_width * sizeof(double));

    world_initialise(task);
    network_tell_randomise_hidden_units(net);
    /* Lesion the context to hidden connections: */
    network_lesion_weights_ch(net, sc_dd[2].level[i]);
    do {
	world_set_network_input_vector(vector_in);
	network_tell_input(net, vector_in);
	network_tell_propagate2(net);
	network_ask_output(net, vector_out);
	this[count] = world_get_network_output_action(NULL, vector_out);
	world_perform_action(this[count]);
    } while ((this[count] != ACTION_SAY_DONE) && (++count < MAX_STEPS));

    free(vector_in);
    free(vector_out);

    /* Now score the actions: */

    analyse_actions_with_acs1(task, &res1);
    errors = analyse_actions_with_acs2(task, &res2);
    analyse_actions_code_free_error_list(errors);

    sc_data[i][0] += res2.omissions;
    sc_data[i][1] += res2.anticipations;
    sc_data[i][1] += res2.perseverations;
    sc_data[i][1] += res2.reversals;
    sc_data[i][2] += res2.object_sub;
    sc_data[i][2] += res2.gesture_sub;
    sc_data[i][2] += res2.tool_omission;
    sc_data[i][2] += res2.action_addition;
    sc_data[i][2] += res2.quality;
    sc_data[i][3] += res2.bizarre;
}

/*****************************************************************************/

static void errors_per_trial_chart_write_to_cairo(cairo_t *cr, int width, int height)
{
    char buffer[128];
    char *labels[(2*MAX_LEVELS)+1];
    int i, j, num_categories;

    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);

    graph_set_extent(errors_per_trial_chart, 0, 0, width, height);

    graph_set_axis_properties(errors_per_trial_chart, GTK_ORIENTATION_VERTICAL, 0.0, 5.0, 11, "%3.1f", "Errors");
    graph_set_axis_properties(errors_per_trial_chart, GTK_ORIENTATION_HORIZONTAL, 0, sc_dd[sc_task.damage-1].num_levels, 2*sc_dd[sc_task.damage-1].num_levels+1, "%2.0f", sc_dd[sc_task.damage-1].label);

    for (j = 0; j < sc_dd[sc_task.damage-1].num_levels; j++) {
        labels[2*j] = "";
        g_snprintf(buffer, 128, sc_dd[sc_task.damage-1].format, sc_dd[sc_task.damage-1].level[j]);
        labels[2*j+1] = string_copy(buffer);
    }
    labels[2*j] = "";
    graph_set_axis_tick_marks(errors_per_trial_chart, GTK_ORIENTATION_HORIZONTAL, (2*sc_dd[sc_task.damage-1].num_levels)+1, labels);

    if (sc_count == 1) {
        g_snprintf(buffer, 128, "Average number of errors per trial for %d trial of the %s task", sc_count, (sc_task.base == TASK_COFFEE ? "coffee" : "tea"));
    }
    else {
        g_snprintf(buffer, 128, "Average number of errors per trial for %d trials of the %s task", sc_count, (sc_task.base == TASK_COFFEE ? "coffee" : "tea"));
    }
    graph_set_title(errors_per_trial_chart, buffer);

    num_categories = sc_dd[sc_task.damage-1].num_levels;

    for (j = 0; j < ERROR_CATEGORIES; j++) {
        for (i = 0; i < num_categories; i++) {
            errors_per_trial_chart->dataset[j].x[i] = 0.5 + i;
            if (sc_count == 0) {
                errors_per_trial_chart->dataset[j].y[i] = 0.0;
            }
            else {
                errors_per_trial_chart->dataset[j].y[i] = sc_data[i][j] / (double) sc_count;
            }
        }
        errors_per_trial_chart->dataset[j].points = sc_dd[sc_task.damage-1].num_levels;
    }

    cairox_draw_graph(cr, errors_per_trial_chart, TRUE);
}


static Boolean errors_per_trial_chart_expose(GtkWidget *widget, GdkEvent *event, void *data)
{
    if (!GTK_WIDGET_MAPPED(viewer_widget) || (viewer_surface == NULL)) {
        return(TRUE);
    }
    else {
	int height = viewer_widget->allocation.height;
	int width  = viewer_widget->allocation.width;
        cairo_t *cr;

        cr = cairo_create(viewer_surface);

        errors_per_trial_chart_write_to_cairo(cr, width, height);

        cairo_destroy(cr);

        /* Now copy the surface to the window: */
        if ((viewer_widget->window != NULL) && ((cr = gdk_cairo_create(viewer_widget->window)) != NULL)) {
            cairo_set_source_surface(cr, viewer_surface, 0, 0);
            cairo_paint(cr);
            cairo_destroy(cr);
        }
        return(FALSE);
    }
}

/******************************************************************************/

static void initialise_sc_data()
{
    int i, j;

    sc_count = 0;
    for (i = 0; i < MAX_LEVELS; i++) {
        for (j = 0; j < ERROR_CATEGORIES; j++) {
            sc_data[i][j] = 0.0;
        }
    }
}

static void errors_per_trial_chart_reset_callback(GtkWidget *mi, void *dummy)
{
    initialise_sc_data();
    errors_per_trial_chart_expose(NULL, NULL, NULL);
}

static void errors_per_trial_chart_set_task_callback(GtkWidget *button, void *task)
{
    if (GTK_TOGGLE_BUTTON(button)->active) {
        sc_task.base = (BaseTaskType) task;
        sc_task.initial_state.bowl_closed = pars.sugar_closed;
        sc_task.initial_state.mug_contains_coffee = pars.mug_with_coffee; 
        sc_task.initial_state.mug_contains_tea = pars.mug_with_tea;
        sc_task.initial_state.mug_contains_cream = pars.mug_with_cream;
        sc_task.initial_state.mug_contains_sugar = pars.mug_with_sugar; 
        errors_per_trial_chart_reset_callback(NULL, NULL);
    }
}

static void errors_per_trial_chart_step_callback(GtkWidget *mi, void *count)
{
    int j = (long) count;
    int i;

    sc_task.initial_state.bowl_closed = pars.sugar_closed;
    sc_task.initial_state.mug_contains_coffee = pars.mug_with_coffee; 
    sc_task.initial_state.mug_contains_tea = pars.mug_with_tea;
    sc_task.initial_state.mug_contains_cream = pars.mug_with_cream;
    sc_task.initial_state.mug_contains_sugar = pars.mug_with_sugar; 
    while (j-- > 0) {
        if (sc_task.damage == DAMAGE_ACTIVATION_NOISE) {
            for (i = 0; i < sc_dd[sc_task.damage-1].num_levels; i++) {
                sc_run_and_score_activation_noise(xg.net, &sc_task, i);
            }
        }
        else if (sc_task.damage == DAMAGE_WEIGHT_NOISE) {
            for (i = 0; i < sc_dd[sc_task.damage-1].num_levels; i++) {
                Network *tmp_net = network_copy(xg.net);
                sc_run_and_score_weight_noise(tmp_net, &sc_task, i);
                network_tell_destroy(tmp_net);
            }
        }
        else if (sc_task.damage == DAMAGE_WEIGHT_LESION) {
            for (i = 0; i < sc_dd[sc_task.damage-1].num_levels; i++) {
                Network *tmp_net = network_copy(xg.net);
                sc_run_and_score_weight_lesion(tmp_net, &sc_task, i);
                network_tell_destroy(tmp_net);
            }
        }
        sc_count++;
        errors_per_trial_chart_expose(NULL, NULL, NULL);
    }
}

static void callback_spin_vertical_scale(GtkWidget *widget, void *dummy)
{
    vertical_scale = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
    errors_per_trial_chart_expose(NULL, NULL, NULL);
}

static void set_damage_callback(GtkWidget *cb, void *dummy)
{
    sc_task.damage = (DamageType) (1 + gtk_combo_box_get_active(GTK_COMBO_BOX(cb)));
    sc_task.initial_state.bowl_closed = pars.sugar_closed;
    sc_task.initial_state.mug_contains_coffee = pars.mug_with_coffee; 
    sc_task.initial_state.mug_contains_tea = pars.mug_with_tea;
    sc_task.initial_state.mug_contains_cream = pars.mug_with_cream;
    sc_task.initial_state.mug_contains_sugar = pars.mug_with_sugar; 
    errors_per_trial_chart_reset_callback(NULL, NULL);
}

static void viewer_widget_snap_callback(GtkWidget *button, void *dummy)
{
    if (viewer_surface != NULL) {
        cairo_surface_t *pdf_surface;
        cairo_t *cr;
        char filename[64];
        int width = 700;
        int height = 400;

        g_snprintf(filename, 64, "%schart_errors_per_trial_%d.png", OUTPUT_FOLDER, (int) sc_task.damage);
        cairo_surface_write_to_png(viewer_surface, filename);

        g_snprintf(filename, 64, "%schart_errors_per_trial_%d.pdf", OUTPUT_FOLDER, (int) sc_task.damage);
        pdf_surface = cairo_pdf_surface_create(filename, width, height);
        cr = cairo_create(pdf_surface);
        errors_per_trial_chart_write_to_cairo(cr, width, height);
        cairo_destroy(cr);
        cairo_surface_destroy(pdf_surface);
    }
}

/******************************************************************************/

void create_bp_errors_per_trial_viewer(GtkWidget *vbox)
{
    GtkWidget *page = gtk_vbox_new(FALSE, 0);
    GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
    GtkWidget *toolbar, *tmp;
    CairoxFontProperties *fp;

    initialise_sc_data();

    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_TEXT);
    gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(hbox), toolbar, FALSE, FALSE, 0);
    gtk_widget_show(toolbar);

    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Reset", NULL, NULL, NULL, G_CALLBACK(errors_per_trial_chart_reset_callback), NULL);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "1", NULL, NULL, NULL, G_CALLBACK(errors_per_trial_chart_step_callback), (void *)1);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "10", NULL, NULL, NULL, G_CALLBACK(errors_per_trial_chart_step_callback), (void *)10);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "100", NULL, NULL, NULL, G_CALLBACK(errors_per_trial_chart_step_callback), (void *)100);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "500", NULL, NULL, NULL, G_CALLBACK(errors_per_trial_chart_step_callback), (void *)500);

    /*--- Padding: ---*/
    tmp = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 5);
    gtk_widget_show(tmp);

    /**** Widgets for setting the type of damage: ****/

    tmp = gtk_label_new("Damage:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    tmp = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(tmp), sc_dd[0].label);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(tmp), sc_dd[1].label);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(tmp), sc_dd[2].label);
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), sc_task.damage - 1);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(tmp), "changed", G_CALLBACK(set_damage_callback), NULL);
    gtk_widget_show(tmp);

    /*--- Padding: ---*/
    tmp = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 5);
    gtk_widget_show(tmp);

    /**** A spin button for setting the vertical scale: ****/

    tmp = gtk_label_new("Vertical scale:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    tmp = gtk_spin_button_new_with_range(1.0, 20.0, 1.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tmp), vertical_scale);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tmp), TRUE);
    gtk_widget_set_size_request(GTK_WIDGET(tmp), 40, -1);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(tmp), "value_changed", G_CALLBACK(callback_spin_vertical_scale), NULL);
    gtk_widget_show(tmp);

    /*--- Padding: ---*/
    tmp = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 5);
    gtk_widget_show(tmp);

    /**** Widgets for setting the task: ****/

    tmp = gtk_radio_button_new_with_label(NULL, "Coffee");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), (sc_task.base == TASK_COFFEE));
    g_signal_connect(G_OBJECT(tmp), "toggled", G_CALLBACK(errors_per_trial_chart_set_task_callback), (void *)TASK_COFFEE);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 3);
    gtk_widget_show(tmp);

    tmp = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(tmp)), "Tea");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), (sc_task.base == TASK_TEA));
    g_signal_connect(G_OBJECT(tmp), "toggled", G_CALLBACK(errors_per_trial_chart_set_task_callback), (void *)TASK_TEA);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 3);
    gtk_widget_show(tmp);

    /*--- Padding: ---*/
    tmp = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 5);
    gtk_widget_show(tmp);

    /*--- Take a snapshot of the canvas: ---*/
    tmp = gtk_button_new_with_label("Snap");
    g_signal_connect(G_OBJECT(tmp), "clicked", G_CALLBACK(viewer_widget_snap_callback), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    /**** The subtask chart: ****/

    viewer_widget = gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(viewer_widget), "expose_event", G_CALLBACK(errors_per_trial_chart_expose), NULL);
    g_signal_connect(G_OBJECT(viewer_widget), "configure_event", G_CALLBACK(gtkx_configure_surface_callback), &viewer_surface);
    g_signal_connect(G_OBJECT(viewer_widget), "destroy", G_CALLBACK(gtkx_destroy_surface_callback), &viewer_surface);
    gtk_widget_set_events(viewer_widget, GDK_EXPOSURE_MASK);
    gtk_box_pack_start(GTK_BOX(page), viewer_widget, TRUE, TRUE, 0);
    gtk_widget_show(viewer_widget);

    errors_per_trial_chart = graph_create(ERROR_CATEGORIES);
    if ((fp = font_properties_create("Sans", 22, PANGO_STYLE_NORMAL, PANGO_WEIGHT_NORMAL, "black")) != NULL) {
        graph_set_title_font_properties(errors_per_trial_chart, fp);
        font_properties_set_size(fp, 18);
        graph_set_legend_font_properties(errors_per_trial_chart, fp);
        graph_set_axis_font_properties(errors_per_trial_chart, GTK_ORIENTATION_HORIZONTAL, fp);
        graph_set_axis_font_properties(errors_per_trial_chart, GTK_ORIENTATION_VERTICAL, fp);
        font_properties_destroy(fp);
    }
    graph_set_margins(errors_per_trial_chart, 72, 26, 30, 45);
    graph_set_dataset_properties(errors_per_trial_chart, 0, "Omission", 1.0, 1.0, 1.0, 40, LS_SOLID, MARK_NONE);
    graph_set_dataset_properties(errors_per_trial_chart, 1, "Sequence", 0.6, 0.6, 0.6, 40, LS_SOLID, MARK_NONE);
    graph_set_dataset_properties(errors_per_trial_chart, 2, "Bizarre", 0.3, 0.3, 0.3, 40, LS_SOLID, MARK_NONE);
    graph_set_dataset_properties(errors_per_trial_chart, 3, "Other", 0.0, 0.0, 0.0, 40, LS_SOLID, MARK_NONE);
    graph_set_legend_properties(errors_per_trial_chart, TRUE, 0.0, 0.0, NULL);
    graph_set_stack_bars(errors_per_trial_chart, TRUE);

    gtk_container_add(GTK_CONTAINER(vbox), page);
    gtk_widget_show(page);
}

/******************************************************************************/
