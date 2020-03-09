
#include "xframe.h"
#include "lib_string.h"
#include "lib_cairox.h"
#include "lib_cairoxg_2_4.h"
#include "xcs_sequences.h"

#define ERROR_CATEGORIES  4
#define MAX_LEVELS 11
#define MAX_STEPS 100

static GtkWidget *viewer_widget = NULL;
static cairo_surface_t *viewer_surface = NULL;

static TaskType sc_task = {TASK_COFFEE, DAMAGE_ACTIVATION_NOISE, {FALSE, FALSE, FALSE, FALSE, FALSE}};
static double sc_data[TASK_MAX][MAX_LEVELS][ERROR_CATEGORIES];
static int sc_count = 0;

typedef struct damage_details {
    char *label;
    int num_levels;
    double level[MAX_LEVELS];
    char *format;
} DamageDetails;

static DamageDetails sc_dd[7] = {
    {"Activation Noise (s.d.)",    10, { 0.10,  0.20,  0.30,  0.40,  0.50,  0.60,  0.70,  0.80,  0.90,  1.00,  0.00}, "%4.2f"},
    {"CH Weight Noise (s.d.)",        10, { 0.10,  0.20,  0.30,  0.40,  0.50,  0.60,  0.70,  0.80,  0.90,  1.00,  0.00}, "%4.2f"},
    {"CH Connections Severed (proportion)", 11, { 0.10,  0.15,  0.20,  0.25,  0.30,  0.40,  0.50,  0.60,  0.70,  0.80,  0.90}, "%4.2f"},
    {"IH Weight Noise (s.d.)",        10, { 0.10,  0.20,  0.30,  0.40,  0.50,  0.60,  0.70,  0.80,  0.90,  1.00,  0.00}, "%4.2f"},
    {"IH Connections Severed (proportion)", 11, { 0.10,  0.15,  0.20,  0.25,  0.30,  0.35,  0.40,  0.45,  0.50,  0.55,  0.60}, "%4.2f"},
    {"Context Units Removed (proportion)", 10, {0.000, 0.001, 0.003, 0.006, 0.009, 0.015, 0.050, 0.100, 0.200, 0.400, 0.000}, "%5.3f"},
    {"Weight Scaling Factor (proportion)", 10, {1.00, 0.90, 0.80, 0.70, 0.60, 0.50, 0.40, 0.30, 0.20, 0.10, 0.00}, "%5.3f"}
};

extern void analyse_actions_with_acs1(TaskType *task, ACS1 *results);

static GraphStruct *independents_chart;

/******************************************************************************/

static void sc_run_and_score_activation_noise(Network *net, TaskType *task, int i)
{
    double     *vector_in;
    double     *vector_out;
    ActionType  this[MAX_STEPS];
    ACS1        results;
    int         count = 0;

    vector_in = (double *)malloc(IN_WIDTH * sizeof(double));
    vector_out = (double *)malloc(OUT_WIDTH * sizeof(double));
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

    analyse_actions_with_acs1(task, &results);

    sc_data[task->base][i][0] += results.actions;
    if (results.actions > 0) {
        sc_data[task->base][i][1] += results.independents / ((double) results.actions);
        sc_data[task->base][i][2] += results.errors_crux / ((double) results.actions);
        sc_data[task->base][i][3] += results.errors_non_crux / ((double) results.actions);
    }
}

static void sc_run_and_score_ch_weight_noise(Network *net, TaskType *task, int i)
{
    double     *vector_in;
    double     *vector_out;
    ActionType  this[MAX_STEPS];
    ACS1        results;
    int         count = 0;

    vector_in = (double *)malloc(IN_WIDTH * sizeof(double));
    vector_out = (double *)malloc(OUT_WIDTH * sizeof(double));
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

    analyse_actions_with_acs1(task, &results);

    sc_data[task->base][i][0] += results.actions;
    if (results.actions > 0) {
        sc_data[task->base][i][1] += results.independents / ((double) results.actions);
        sc_data[task->base][i][2] += results.errors_crux / ((double) results.actions);
        sc_data[task->base][i][3] += results.errors_non_crux / ((double) results.actions);
    }
}

static void sc_run_and_score_ch_weight_lesion(Network *net, TaskType *task, int i)
{
    double     *vector_in;
    double     *vector_out;
    ActionType  this[MAX_STEPS];
    ACS1        results;
    int         count = 0;

    vector_in = (double *)malloc(IN_WIDTH * sizeof(double));
    vector_out = (double *)malloc(OUT_WIDTH * sizeof(double));
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

    analyse_actions_with_acs1(task, &results);

    sc_data[task->base][i][0] += results.actions;
    if (results.actions > 0) {
        sc_data[task->base][i][1] += results.independents / ((double) results.actions);
        sc_data[task->base][i][2] += results.errors_crux / ((double) results.actions);
        sc_data[task->base][i][3] += results.errors_non_crux / ((double) results.actions);
    }
}

static void sc_run_and_score_ih_weight_noise(Network *net, TaskType *task, int i)
{
    double     *vector_in;
    double     *vector_out;
    ActionType  this[MAX_STEPS];
    ACS1        results;
    int         count = 0;

    vector_in = (double *)malloc(IN_WIDTH * sizeof(double));
    vector_out = (double *)malloc(OUT_WIDTH * sizeof(double));
    world_initialise(task);
    network_tell_randomise_hidden_units(net);
    /* Add noise to context to hidden connections: */
    network_perturb_weights_ih(net, sc_dd[3].level[i]);
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

    analyse_actions_with_acs1(task, &results);

    sc_data[task->base][i][0] += results.actions;
    if (results.actions > 0) {
        sc_data[task->base][i][1] += results.independents / ((double) results.actions);
        sc_data[task->base][i][2] += results.errors_crux / ((double) results.actions);
        sc_data[task->base][i][3] += results.errors_non_crux / ((double) results.actions);
    }
}

static void sc_run_and_score_ih_weight_lesion(Network *net, TaskType *task, int i)
{
    double     *vector_in;
    double     *vector_out;
    ActionType  this[MAX_STEPS];
    ACS1        results;
    int         count = 0;

    vector_in = (double *)malloc(IN_WIDTH * sizeof(double));
    vector_out = (double *)malloc(OUT_WIDTH * sizeof(double));
    world_initialise(task);
    network_tell_randomise_hidden_units(net);
    /* Lesion the context to hidden connections: */
    network_lesion_weights_ih(net, sc_dd[4].level[i]);
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

    analyse_actions_with_acs1(task, &results);

    sc_data[task->base][i][0] += results.actions;
    if (results.actions > 0) {
        sc_data[task->base][i][1] += results.independents / ((double) results.actions);
        sc_data[task->base][i][2] += results.errors_crux / ((double) results.actions);
        sc_data[task->base][i][3] += results.errors_non_crux / ((double) results.actions);
    }
}

static void sc_run_and_score_context_ablate(Network *net, TaskType *task, int i)
{
    double     *vector_in;
    double     *vector_out;
    ActionType  this[MAX_STEPS];
    ACS1        results;
    int         count = 0;

    vector_in = (double *)malloc(IN_WIDTH * sizeof(double));
    vector_out = (double *)malloc(OUT_WIDTH * sizeof(double));
    world_initialise(task);
    network_tell_randomise_hidden_units(net);
    /* Ablate the context units: */
    network_ablate_context(net, sc_dd[5].level[i]);
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

    analyse_actions_with_acs1(task, &results);

    sc_data[task->base][i][0] += results.actions;
    if (results.actions > 0) {
        sc_data[task->base][i][1] += results.independents / ((double) results.actions);
        sc_data[task->base][i][2] += results.errors_crux / ((double) results.actions);
        sc_data[task->base][i][3] += results.errors_non_crux / ((double) results.actions);
    }
}

static void sc_run_and_score_weight_scale(Network *net, TaskType *task, int i)
{
    double     *vector_in;
    double     *vector_out;
    ActionType  this[MAX_STEPS];
    ACS1        results;
    int         count = 0;

    vector_in = (double *)malloc(IN_WIDTH * sizeof(double));
    vector_out = (double *)malloc(OUT_WIDTH * sizeof(double));
    world_initialise(task);
    network_tell_randomise_hidden_units(net);
    /* Scale weights: */
    network_scale_weights(net, sc_dd[6].level[i]);
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

    analyse_actions_with_acs1(task, &results);

    sc_data[task->base][i][0] += results.actions;
    if (results.actions > 0) {
        sc_data[task->base][i][1] += results.independents / ((double) results.actions);
        sc_data[task->base][i][2] += results.errors_crux / ((double) results.actions);
        sc_data[task->base][i][3] += results.errors_non_crux / ((double) results.actions);
    }
}


/*****************************************************************************/

static void independents_chart_write_to_cairo(cairo_t *cr, int width, int height)
{
    char buffer[128];
    char *labels[(2*MAX_LEVELS)+1];
    int i, j, num_categories;

    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);

    graph_set_extent(independents_chart, 0, 0, width, height);

    graph_set_axis_properties(independents_chart, GTK_ORIENTATION_VERTICAL, 0.0, 1.0, 11, "%4.2f", NULL);
    graph_set_axis_properties(independents_chart, GTK_ORIENTATION_HORIZONTAL, 0, sc_dd[sc_task.damage-1].num_levels, 2*sc_dd[sc_task.damage-1].num_levels+1, "%2.0f", sc_dd[sc_task.damage-1].label);

    for (j = 0; j < sc_dd[sc_task.damage-1].num_levels; j++) {
        labels[2*j] = "";
        g_snprintf(buffer, 128, sc_dd[sc_task.damage-1].format, sc_dd[sc_task.damage-1].level[j]);
        labels[2*j+1] = string_copy(buffer);
    }
    labels[2*j] = "";
    graph_set_axis_tick_marks(independents_chart, GTK_ORIENTATION_HORIZONTAL, (2*sc_dd[sc_task.damage-1].num_levels)+1, labels);

    if (sc_count == 1) {
        g_snprintf(buffer, 128, "Proportion of independents/errors for %d trial", sc_count);
    }
    else {
        g_snprintf(buffer, 128, "Proportion of independents/errors for %d trials", sc_count);
    }
    graph_set_title(independents_chart, buffer);

    num_categories = sc_dd[sc_task.damage-1].num_levels;

    independents_chart->dataset[0].points = 0;
    independents_chart->dataset[ERROR_CATEGORIES].points = 0;
    for (j = 1; j < ERROR_CATEGORIES; j++) {
        for (i = 0; i < num_categories; i++) {
            independents_chart->dataset[2*j].x[i] = 0.5 + i;
            independents_chart->dataset[2*j+1].x[i] = 0.5 + i;
            if ((sc_count == 0) || (sc_task.base == TASK_NONE)) { 
                independents_chart->dataset[2*j].y[i] = 0.0;
                independents_chart->dataset[2*j+1].y[i] = 0.0;
            }
            else {
                independents_chart->dataset[2*j].y[i] = sc_data[TASK_COFFEE][i][j] / (double) sc_count;
                independents_chart->dataset[2*j+1].y[i] = sc_data[TASK_TEA][i][j] / (double) sc_count;
            }
            independents_chart->dataset[2*j].se[i] = 0.0;
            independents_chart->dataset[2*j+1].se[i] = 0.0;
        }
        if ((sc_count == 0) || (sc_task.base == TASK_NONE)) {
            independents_chart->dataset[2*j].points = 0;
            independents_chart->dataset[2*j+1].points = 0;
        }
        else if (sc_task.base == TASK_COFFEE) {
            independents_chart->dataset[2*j].points = sc_dd[sc_task.damage-1].num_levels;
            independents_chart->dataset[2*j+1].points = 0;
        }
        else if (sc_task.base == TASK_TEA) {
            independents_chart->dataset[2*j].points = 0;
            independents_chart->dataset[2*j+1].points = sc_dd[sc_task.damage-1].num_levels;
        }
        else { // BOTH
            independents_chart->dataset[2*j].points = sc_dd[sc_task.damage-1].num_levels;
            independents_chart->dataset[2*j+1].points = sc_dd[sc_task.damage-1].num_levels;
        }
    }
    cairox_draw_graph(cr, independents_chart, xg.colour);
}

static Boolean independents_chart_expose(GtkWidget *widget, GdkEvent *event, void *data)
{
    if (!GTK_WIDGET_MAPPED(viewer_widget) || (viewer_surface == NULL)) {
        return(TRUE);
    }
    else {
        cairo_t *cr;
	int height = viewer_widget->allocation.height;
	int width  = viewer_widget->allocation.width;

        cr = cairo_create(viewer_surface);
        independents_chart_write_to_cairo(cr, width, height);
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
    int i, j, t;

    sc_count = 0;
    for (t = 0; t < TASK_MAX; t++) {
        for (i = 0; i < MAX_LEVELS; i++) {
            for (j = 0; j < ERROR_CATEGORIES; j++) {
                sc_data[t][i][j] = 0.0;
            }
        }
    }
}

static void independents_chart_reset_callback(GtkWidget *mi, void *dummy)
{
    initialise_sc_data();
    independents_chart_expose(NULL, NULL, NULL);
}

static void independents_chart_set_task_callback(GtkWidget *button, void *task)
{
    if (GTK_TOGGLE_BUTTON(button)->active) {
        // Add the selected task from sv_task.base:
        if (sc_task.base == TASK_NONE) {
            sc_task.base = (BaseTaskType) task;
        }
        else if ((sc_task.base == TASK_COFFEE) && ((BaseTaskType) task == TASK_TEA)) {
            sc_task.base = TASK_MAX;
        }
        else if ((sc_task.base == TASK_TEA) && ((BaseTaskType) task == TASK_COFFEE)) {
            sc_task.base = TASK_MAX;
        }
    }
    else {
        // Remove the selected task from sc_task.base:
        if ((sc_task.base == TASK_MAX) && ((BaseTaskType) task == TASK_TEA)) {
            sc_task.base = TASK_COFFEE;
        }
        else if ((sc_task.base == TASK_MAX) && ((BaseTaskType) task == TASK_COFFEE)) {
            sc_task.base = TASK_TEA;
        }
        else if (sc_task.base == (BaseTaskType) task) {
            sc_task.base = TASK_NONE;
        }
    }

    sc_task.initial_state.bowl_closed = pars.sugar_closed;
    sc_task.initial_state.mug_contains_coffee = pars.mug_with_coffee; 
    sc_task.initial_state.mug_contains_tea = pars.mug_with_tea;
    sc_task.initial_state.mug_contains_cream = pars.mug_with_cream;
    sc_task.initial_state.mug_contains_sugar = pars.mug_with_sugar; 
    independents_chart_reset_callback(NULL, NULL);
}

static void set_damage_callback(GtkWidget *cb, void *dummy)
{
    sc_task.damage = (DamageType) (1 + gtk_combo_box_get_active(GTK_COMBO_BOX(cb)));
    sc_task.initial_state.bowl_closed = pars.sugar_closed;
    sc_task.initial_state.mug_contains_coffee = pars.mug_with_coffee; 
    sc_task.initial_state.mug_contains_tea = pars.mug_with_tea;
    sc_task.initial_state.mug_contains_cream = pars.mug_with_cream;
    sc_task.initial_state.mug_contains_sugar = pars.mug_with_sugar; 
    independents_chart_reset_callback(NULL, NULL);
}

static void independents_chart_step_callback(GtkWidget *mi, void *count)
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
                if (sc_task.base == TASK_MAX) {
                    sc_task.base = TASK_COFFEE;
                    sc_run_and_score_activation_noise(xg.net, &sc_task, i);
                    sc_task.base = TASK_TEA;
                    sc_run_and_score_activation_noise(xg.net, &sc_task, i);
                    sc_task.base = TASK_MAX;
                }
                else if (sc_task.base != TASK_NONE) {
                    sc_run_and_score_activation_noise(xg.net, &sc_task, i);
                }
            }
        }
        else if (sc_task.damage == DAMAGE_CH_WEIGHT_NOISE) {
            Network *tmp_net;
            for (i = 0; i < sc_dd[sc_task.damage-1].num_levels; i++) {
                if (sc_task.base == TASK_MAX) {
                    sc_task.base = TASK_COFFEE;
                    tmp_net = network_copy(xg.net);
                    sc_run_and_score_ch_weight_noise(tmp_net, &sc_task, i);
                    network_tell_destroy(tmp_net);
                    sc_task.base = TASK_TEA;
                    tmp_net = network_copy(xg.net);
                    sc_run_and_score_ch_weight_noise(tmp_net, &sc_task, i);
                    network_tell_destroy(tmp_net);
                    sc_task.base = TASK_MAX;
                }
                else if (sc_task.base != TASK_NONE) {
                    tmp_net = network_copy(xg.net);
                    sc_run_and_score_ch_weight_noise(tmp_net, &sc_task, i);
                    network_tell_destroy(tmp_net);
                }
            }
	}
        else if (sc_task.damage == DAMAGE_CH_WEIGHT_LESION) {
            Network *tmp_net;
            for (i = 0; i < sc_dd[sc_task.damage-1].num_levels; i++) {
                if (sc_task.base == TASK_MAX) {
                    sc_task.base = TASK_COFFEE;
                    tmp_net = network_copy(xg.net);
                    sc_run_and_score_ch_weight_lesion(tmp_net, &sc_task, i);
                    network_tell_destroy(tmp_net);
                    sc_task.base = TASK_TEA;
                    tmp_net = network_copy(xg.net);
                    sc_run_and_score_ch_weight_lesion(tmp_net, &sc_task, i);
                    network_tell_destroy(tmp_net);
                    sc_task.base = TASK_MAX;
                }
                else if (sc_task.base != TASK_NONE) {
                    tmp_net = network_copy(xg.net);
                    sc_run_and_score_ch_weight_lesion(tmp_net, &sc_task, i);
                    network_tell_destroy(tmp_net);
                }
            }
	}
        else if (sc_task.damage == DAMAGE_IH_WEIGHT_NOISE) {
            Network *tmp_net;
            for (i = 0; i < sc_dd[sc_task.damage-1].num_levels; i++) {
                if (sc_task.base == TASK_MAX) {
                    sc_task.base = TASK_COFFEE;
                    tmp_net = network_copy(xg.net);
                    sc_run_and_score_ih_weight_noise(tmp_net, &sc_task, i);
                    network_tell_destroy(tmp_net);
                    sc_task.base = TASK_TEA;
                    tmp_net = network_copy(xg.net);
                    sc_run_and_score_ih_weight_noise(tmp_net, &sc_task, i);
                    network_tell_destroy(tmp_net);
                    sc_task.base = TASK_MAX;
                }
                else if (sc_task.base != TASK_NONE) {
                    tmp_net = network_copy(xg.net);
                    sc_run_and_score_ih_weight_noise(tmp_net, &sc_task, i);
                    network_tell_destroy(tmp_net);
                }
            }
	}
        else if (sc_task.damage == DAMAGE_IH_WEIGHT_LESION) {
            Network *tmp_net;
            for (i = 0; i < sc_dd[sc_task.damage-1].num_levels; i++) {
                if (sc_task.base == TASK_MAX) {
                    sc_task.base = TASK_COFFEE;
                    tmp_net = network_copy(xg.net);
                    sc_run_and_score_ih_weight_lesion(tmp_net, &sc_task, i);
                    network_tell_destroy(tmp_net);
                    sc_task.base = TASK_TEA;
                    tmp_net = network_copy(xg.net);
                    sc_run_and_score_ih_weight_lesion(tmp_net, &sc_task, i);
                    network_tell_destroy(tmp_net);
                    sc_task.base = TASK_MAX;
                }
                else if (sc_task.base != TASK_NONE) {
                    tmp_net = network_copy(xg.net);
                    sc_run_and_score_ih_weight_lesion(tmp_net, &sc_task, i);
                    network_tell_destroy(tmp_net);
                }
            }
	}
        else if (sc_task.damage == DAMAGE_CONTEXT_ABLATE) {
            Network *tmp_net;
            for (i = 0; i < sc_dd[sc_task.damage-1].num_levels; i++) {
                if (sc_task.base == TASK_MAX) {
                    sc_task.base = TASK_COFFEE;
                    tmp_net = network_copy(xg.net);
                    sc_run_and_score_context_ablate(tmp_net, &sc_task, i);
                    network_tell_destroy(tmp_net);
                    sc_task.base = TASK_TEA;
                    tmp_net = network_copy(xg.net);
                    sc_run_and_score_context_ablate(tmp_net, &sc_task, i);
                    network_tell_destroy(tmp_net);
                    sc_task.base = TASK_MAX;
                }
                else if (sc_task.base != TASK_NONE) {
                    tmp_net = network_copy(xg.net);
                    sc_run_and_score_context_ablate(tmp_net, &sc_task, i);
                    network_tell_destroy(tmp_net);
                }
            }
        }
        else if (sc_task.damage == DAMAGE_WEIGHT_SCALE) {
            Network *tmp_net;
            for (i = 0; i < sc_dd[sc_task.damage-1].num_levels; i++) {
                if (sc_task.base == TASK_MAX) {
                    sc_task.base = TASK_COFFEE;
                    tmp_net = network_copy(xg.net);
                    sc_run_and_score_weight_scale(tmp_net, &sc_task, i);
                    network_tell_destroy(tmp_net);
                    sc_task.base = TASK_TEA;
                    tmp_net = network_copy(xg.net);
                    sc_run_and_score_weight_scale(tmp_net, &sc_task, i);
                    network_tell_destroy(tmp_net);
                    sc_task.base = TASK_MAX;
                }
                else if (sc_task.base != TASK_NONE) {
                    tmp_net = network_copy(xg.net);
                    sc_run_and_score_weight_scale(tmp_net, &sc_task, i);
                    network_tell_destroy(tmp_net);
                }
            }
        }
        else {
            fprintf(stdout, "WARNING: Damage type %d not implemented\n", sc_task.damage);
        }
        sc_count++;
        independents_chart_expose(NULL, NULL, NULL);
    }
}

static void viewer_widget_snap_callback(GtkWidget *button, void *dummy)
{
    if (viewer_surface != NULL) {
        cairo_surface_t *pdf_surface;
        cairo_t *cr;
        char filename[64];
        int width = 700;
        int height = 350; // Was 400;

        g_snprintf(filename, 64, "%sbp_independents_%d.png", OUTPUT_FOLDER, (int) sc_task.damage);
        cairo_surface_write_to_png(viewer_surface, filename);
        fprintf(stdout, "Chart written to %s\n", filename);
        g_snprintf(filename, 64, "%sbp_independents_%d.pdf", OUTPUT_FOLDER, (int) sc_task.damage);
        pdf_surface = cairo_pdf_surface_create(filename, width, height);
        cr = cairo_create(pdf_surface);
        independents_chart_write_to_cairo(cr, width, height);
        fprintf(stdout, "Chart written to %s\n", filename);
        cairo_destroy(cr);
        cairo_surface_destroy(pdf_surface);
    }
}

/******************************************************************************/

void create_bp_independents_chart_viewer(GtkWidget *vbox)
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

    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Reset", NULL, NULL, NULL, G_CALLBACK(independents_chart_reset_callback), NULL);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "1", NULL, NULL, NULL, G_CALLBACK(independents_chart_step_callback), (void *)1);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "10", NULL, NULL, NULL, G_CALLBACK(independents_chart_step_callback), (void *)10);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "100", NULL, NULL, NULL, G_CALLBACK(independents_chart_step_callback), (void *)100);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "500", NULL, NULL, NULL, G_CALLBACK(independents_chart_step_callback), (void *)500);

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
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(tmp), sc_dd[3].label);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(tmp), sc_dd[4].label);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(tmp), sc_dd[5].label);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(tmp), sc_dd[6].label);
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), sc_task.damage - 1);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(tmp), "changed", G_CALLBACK(set_damage_callback), NULL);
    gtk_widget_show(tmp);

    /*--- Padding: ---*/
    tmp = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 5);
    gtk_widget_show(tmp);

    /**** Widgets for setting the task: ****/

    tmp = gtk_check_button_new_with_label("Coffee");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), (sc_task.base == TASK_COFFEE) || (sc_task.base == TASK_MAX));
    g_signal_connect(G_OBJECT(tmp), "toggled", G_CALLBACK(independents_chart_set_task_callback), (void *)TASK_COFFEE);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 3);
    gtk_widget_show(tmp);

    tmp = gtk_check_button_new_with_label("Tea");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), (sc_task.base == TASK_TEA) || (sc_task.base == TASK_MAX));
    g_signal_connect(G_OBJECT(tmp), "toggled", G_CALLBACK(independents_chart_set_task_callback), (void *)TASK_TEA);
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

    /**** The independents chart: ****/

    viewer_widget = gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(viewer_widget), "expose_event", G_CALLBACK(independents_chart_expose), NULL);
    g_signal_connect(G_OBJECT(viewer_widget), "configure_event", G_CALLBACK(gtkx_configure_surface_callback), &viewer_surface);
    g_signal_connect(G_OBJECT(viewer_widget), "destroy", G_CALLBACK(gtkx_destroy_surface_callback), &viewer_surface);
    gtk_widget_set_events(viewer_widget, GDK_EXPOSURE_MASK);
    gtk_box_pack_start(GTK_BOX(page), viewer_widget, TRUE, TRUE, 0);
    gtk_widget_show(viewer_widget);

    independents_chart = graph_create(ERROR_CATEGORIES*2);
    if ((fp = font_properties_create("Sans", 22, PANGO_STYLE_NORMAL, PANGO_WEIGHT_NORMAL, "black")) != NULL) {
        graph_set_title_font_properties(independents_chart, fp);
        font_properties_set_size(fp, 18);
        graph_set_legend_font_properties(independents_chart, fp);
        graph_set_axis_font_properties(independents_chart, GTK_ORIENTATION_HORIZONTAL, fp);
        graph_set_axis_font_properties(independents_chart, GTK_ORIENTATION_VERTICAL, fp);
        font_properties_destroy(fp);
    }
    graph_set_margins(independents_chart, 72, 26, 30, 45);
    graph_set_dataset_properties(independents_chart, 0, "Actions (Coffee)", 1.0, 0.0, 0.0, 0, LS_SOLID, MARK_NONE); // Note that we don't plot this data set
    graph_set_dataset_properties(independents_chart, 1, "Actions (Tea)", 0.0, 0.0, 1.0, 0, LS_SOLID, MARK_NONE); // Note that we don't plot this data set
    graph_set_dataset_properties(independents_chart, 2, "Independents per action (Coffee)", 1.0, 0.0, 0.0, 0, LS_SOLID, MARK_SQUARE_FILLED);
    graph_set_dataset_properties(independents_chart, 3, "Independents per action (Tea)", 0.0, 0.0, 1.0, 0, LS_SOLID, MARK_SQUARE_OPEN);
    graph_set_dataset_properties(independents_chart, 4, "Crux errors per action (Coffee)", 1.0, 0.0, 0.0, 0, LS_DOTTED, MARK_DIAMOND_FILLED);
    graph_set_dataset_properties(independents_chart, 5, "Crux errors per action (Tea)", 0.0, 0.0, 1.0, 0, LS_DOTTED, MARK_DIAMOND_OPEN);
    graph_set_dataset_properties(independents_chart, 6, "Non-crux errors per action (Coffee)", 1.0, 0.0, 0.0, 0, LS_DASHED, MARK_CIRCLE_FILLED);
    graph_set_dataset_properties(independents_chart, 7, "Non-crux errors per action (Tea)", 0.0, 0.0, 1.0, 0, LS_DASHED, MARK_CIRCLE_OPEN);
    graph_set_legend_properties(independents_chart, TRUE, 0.0, 0.0, NULL);
    graph_set_stack_bars(independents_chart, 0);

    gtk_container_add(GTK_CONTAINER(vbox), page);
    gtk_widget_show(page);
}

/******************************************************************************/
