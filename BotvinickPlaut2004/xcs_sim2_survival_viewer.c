
#include "xframe.h"
#include "lib_cairox_2_0.h"
#include "lib_cairoxg_2_5.h"
#include "xcs_sequences.h"

// Average data over a maximum of 12 networks;
#define MAX_NETS 12
// Consider at most 40 steps in any task:
#define MAX_STEPS 40

static GtkWidget *viewer_widget = NULL;
static cairo_surface_t *viewer_surface = NULL;

static TaskType sv_task = {TASK_COFFEE, DAMAGE_ACTIVATION_NOISE, {FALSE, FALSE, FALSE, FALSE, FALSE}};
static int sv_data[MAX_NETS][TASK_MAX][MAX_STEPS];
static double sv_level = 0.000;

static char *sv_label[7] = {
    "Activation Noise (s.d.)",
    "CH Weight Noise (s.d.)",
    "CH Connections Severed (prop.)",
    "IH Weight Noise (s.d.)",
    "IH Connections Severed (prop.)",
    "Context Units Removed (prop.)",
    "Weight Scaling (prop.)"
};

static GraphStruct *survival_graph;

// Defined in xcs_sim2.c
extern Boolean sim2_load_weights_from_disk(int net_count);

/******************************************************************************/

static void initialise_sv_data()
{
    int i, t, n;

    for (i = 0; i < MAX_STEPS; i++) {
        for (t = 0; t < TASK_MAX; t++) {
            for (n = 0; n < MAX_NETS; n++) {
                sv_data[n][t][i] = 0;
            }
        }
    }
    sv_task.base = TASK_COFFEE;
    sv_task.initial_state.bowl_closed = pars.sugar_closed;
    sv_task.initial_state.mug_contains_coffee = pars.mug_with_coffee; 
    sv_task.initial_state.mug_contains_tea = pars.mug_with_tea;
    sv_task.initial_state.mug_contains_cream = pars.mug_with_cream;
    sv_task.initial_state.mug_contains_sugar = pars.mug_with_sugar; 
}

/******************************************************************************/

static int get_first_difference(ActionType *this, ActionType *target, int max)
{
    int i = 0;

    while (i < max) {
        if (this[i] == target[i]) {
            i++;
        }
        else {
            return(i+1);
        }
    }
    return(i+1);
}

static int get_first_error(ActionType *this, TaskType *task)
{
    int i1, i2, i3, i4, i5, i6;

    if (task->base == TASK_COFFEE) {
        i1 = get_first_difference(this, sequence_coffee1, 37);
        i2 = get_first_difference(this, sequence_coffee2, 37);
        i3 = get_first_difference(this, sequence_coffee3, 37);
        i4 = get_first_difference(this, sequence_coffee4, 37);
        i5 = get_first_difference(this, sequence_coffee5, 31);
        i6 = get_first_difference(this, sequence_coffee6, 31);
        return(MAX(i1, MAX(i2, MAX(i3, MAX(i4, MAX(i5, i6))))));
    }
    else {
        i1 = get_first_difference(this, sequence_tea1, 20);
        i2 = get_first_difference(this, sequence_tea2, 20);
        i3 = get_first_difference(this, sequence_tea3, 14);
        return(MAX(i1, MAX(i2, i3)));
    }
}

#if DEBUG
static void print_script(int j, ActionType *this, int count)
{
    char buffer[64];
    FILE *fp;
    int i;

    g_snprintf(buffer, 64, "/tmp/bp%3d.dat", j);
    fp = fopen(buffer, "w");
    fprintf(fp, "Error on step %d\n", count);
    for (i = 0; i < MAX_STEPS; i++) {
        world_decode_action(buffer, 64, this[i]);
        fprintf(fp, "%2d: %s\n", i+1, buffer);
    }
    fclose(fp);
}
#endif

static void sv_run_and_score_activation_noise(Network *net, int net_count, TaskType *task)
{
    double    *vector_in;
    double    *vector_out;
    ActionType this[MAX_STEPS];
    int        count = 0;

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
	network_inject_noise(net, sv_level*sv_level);
    } while ((this[count] != ACTION_SAY_DONE) && (++count < MAX_STEPS));

    free(vector_in);
    free(vector_out);

    /* Now score the actions: */

    count = get_first_error(this, task);
#if DEBUG
    print_script(j, this, count);
#endif
    while (count-- > 0) {
	sv_data[net_count][task->base][count]++;
    }
}

static void sv_run_and_score_ch_weight_noise(Network *net, int net_count, TaskType *task)
{
    double    *vector_in;
    double    *vector_out;
    ActionType this[MAX_STEPS];
    int        count = 0;

    vector_in = (double *)malloc(IN_WIDTH * sizeof(double));
    vector_out = (double *)malloc(OUT_WIDTH * sizeof(double));

    world_initialise(task);
    network_tell_randomise_hidden_units(net);
    /* Add noise to context to hidden connections: */
    network_perturb_weights_ch(net, sv_level);
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

    count = get_first_error(this, task);
#if DEBUG
    print_script(j, this, count);
#endif
    while (count-- > 0) {
	sv_data[net_count][task->base][count]++;
    }
}

static void sv_run_and_score_ch_weight_lesion(Network *net, int net_count, TaskType *task)
{
    double    *vector_in;
    double    *vector_out;
    ActionType this[MAX_STEPS];
    int        count = 0;

    vector_in = (double *)malloc(IN_WIDTH * sizeof(double));
    vector_out = (double *)malloc(OUT_WIDTH * sizeof(double));

    world_initialise(task);
    network_tell_randomise_hidden_units(net);
    /* Lesion the context to hidden connections: */
    network_lesion_weights_ch(net, sv_level);
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

    count = get_first_error(this, task);
#if DEBUG
    print_script(j, this, count);
#endif
    while (count-- > 0) {
	sv_data[net_count][task->base][count]++;
    }
}

static void sv_run_and_score_ih_weight_noise(Network *net, int net_count, TaskType *task)
{
    double    *vector_in;
    double    *vector_out;
    ActionType this[MAX_STEPS];
    int        count = 0;

    vector_in = (double *)malloc(IN_WIDTH * sizeof(double));
    vector_out = (double *)malloc(OUT_WIDTH * sizeof(double));

    world_initialise(task);
    network_tell_randomise_hidden_units(net);
    /* Add noise to context to hidden connections: */
    network_perturb_weights_ih(net, sv_level);
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

    count = get_first_error(this, task);
#if DEBUG
    print_script(j, this, count);
#endif
    while (count-- > 0) {
	sv_data[net_count][task->base][count]++;
    }
}

static void sv_run_and_score_ih_weight_lesion(Network *net, int net_count, TaskType *task)
{
    double    *vector_in;
    double    *vector_out;
    ActionType this[MAX_STEPS];
    int        count = 0;

    vector_in = (double *)malloc(IN_WIDTH * sizeof(double));
    vector_out = (double *)malloc(OUT_WIDTH * sizeof(double));

    world_initialise(task);
    network_tell_randomise_hidden_units(net);
    /* Lesion the context to hidden connections: */
    network_lesion_weights_ih(net, sv_level);
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

    count = get_first_error(this, task);
#if DEBUG
    print_script(j, this, count);
#endif
    while (count-- > 0) {
	sv_data[net_count][task->base][count]++;
    }
}

static void sv_run_and_score_context_ablate(Network *net, int net_count, TaskType *task)
{
    double    *vector_in;
    double    *vector_out;
    ActionType this[MAX_STEPS];
    int        count = 0;

    vector_in = (double *)malloc(IN_WIDTH * sizeof(double));
    vector_out = (double *)malloc(OUT_WIDTH * sizeof(double));

    world_initialise(task);
    network_tell_randomise_hidden_units(net);
    /* Ablate units in the context layer: */
    network_ablate_context(net, sv_level);
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

    count = get_first_error(this, task);
#if DEBUG
    print_script(j, this, count);
#endif
    while (count-- > 0) {
	sv_data[net_count][task->base][count]++;
    }
}

static void sv_run_and_score_weight_scale(Network *net, int net_count, TaskType *task)
{
    double    *vector_in;
    double    *vector_out;
    ActionType this[MAX_STEPS];
    int        count = 0;

    vector_in = (double *)malloc(IN_WIDTH * sizeof(double));
    vector_out = (double *)malloc(OUT_WIDTH * sizeof(double));

    world_initialise(task);
    network_tell_randomise_hidden_units(net);
    /* Scale network weights: */
    network_scale_weights(net, sv_level);
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

    count = get_first_error(this, task);
#if DEBUG
    print_script(j, this, count);
#endif
    while (count-- > 0) {
	sv_data[net_count][task->base][count]++;
    }
}

/******************************************************************************/

static double sim2_get_survival_baseline(BaseTaskType task)
{
    int n;
    double sum = 0.0;
    for (n = 0; n < MAX_NETS; n++) {
        sum += sv_data[n][task][0];
    }

    return(sum / (double) n);
}


static double sim2_get_survival_stats_mean(BaseTaskType task, int i)
{
    double sum;
    int n;
    
    sum = 0.0;
    for (n = 0; n < MAX_NETS; n++) {
        sum += sv_data[n][task][i];
    }
    return(sum / (double) n);
}

static void survival_graph_write_to_cairo(cairo_t *cr, int width, int height)
{
    char buffer[128];
    int i, n, ms, num_trials, steps;
    double baseline, mean;

    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);

    if ((sv_task.base == TASK_COFFEE) || (sv_task.base == TASK_MAX)) {
        num_trials = sv_data[0][TASK_COFFEE][0];
    }
    else if ((sv_task.base == TASK_TEA) || (sv_task.base == TASK_MAX)) {
        num_trials = sv_data[0][TASK_TEA][0];
    }
    else {
        num_trials = 0;
    }

    if (num_trials == 1) {
        g_snprintf(buffer, 128, "Survival plot [%d trial; %d nets; %s = %4.2f]", num_trials, MAX_NETS, sv_label[sv_task.damage-1], sv_level);
    }
    else {
        g_snprintf(buffer, 128, "Survival plot [%d trials; %d nets; %s = %4.2f]", num_trials, MAX_NETS, sv_label[sv_task.damage-1], sv_level);
    }
    graph_set_title(survival_graph, buffer);

    graph_set_extent(survival_graph, 0, 0, width, height);

    graph_set_axis_properties(survival_graph, GTK_ORIENTATION_VERTICAL, 0.0, 100.0, 11, "%3.0f%%", "Percentage of Correct Trials");
    g_snprintf(buffer, 128, "Step in task");
    ms = 40;            // Maximum steps to show on the horizontal axis
    graph_set_axis_properties(survival_graph, GTK_ORIENTATION_HORIZONTAL, 0, ms, 1+(ms/5), "%2.0f", buffer);
    graph_set_dataset_properties(survival_graph, 2*MAX_NETS, "% Correct (Coffee)", 1.0, 0.0, 0.0, 0, LS_SOLID, MARK_SQUARE_FILLED);
    graph_set_dataset_properties(survival_graph, 2*MAX_NETS+1, "% Correct (Tea)", 0.0, 0.0, 1.0, 0, LS_SOLID, MARK_SQUARE_OPEN);
    for (i = 0; i < MAX_NETS; i++) {
        graph_set_dataset_properties(survival_graph, 2*i, NULL, 1.0, 0.7, 0.7, 0, LS_SOLID, MARK_SQUARE_FILLED);
        graph_set_dataset_properties(survival_graph, 2*i+1, NULL, 0.7, 0.7, 1.0, 0, LS_SOLID, MARK_SQUARE_OPEN);
    }
    graph_set_legend_properties(survival_graph, TRUE, 0.71, 0.00, NULL);

    if ((sv_task.base == TASK_COFFEE) || (sv_task.base == TASK_MAX)) {
        steps = 37;
        if (sv_data[0][TASK_COFFEE][0] > 0) {
            baseline = sim2_get_survival_baseline(TASK_COFFEE);
            for (i = 0; i < steps; i++) {
                survival_graph->dataset[2*MAX_NETS].x[i] = i;
                mean = sim2_get_survival_stats_mean(TASK_COFFEE, i);
                survival_graph->dataset[2*MAX_NETS].y[i] = 100 * mean / baseline;
                survival_graph->dataset[2*MAX_NETS].dy1[i] = 0;
                survival_graph->dataset[2*MAX_NETS].dy2[i] = 0;
            }
            survival_graph->dataset[2*MAX_NETS].points = steps;

            // Set data points for all other lines
            for (n = 0; n < MAX_NETS; n++) {
                for (i = 0; i < steps; i++) {
                    survival_graph->dataset[2*n].x[i] = i;
                    survival_graph->dataset[2*n].y[i] = 100 * sv_data[n][TASK_COFFEE][i] / baseline;
                    survival_graph->dataset[2*n].dy1[i] = 0;
                    survival_graph->dataset[2*n].dy2[i] = 0;
                }
                survival_graph->dataset[2*n].points = steps;
            }
        }
        else {
            survival_graph->dataset[2*MAX_NETS].points = 0;
            for (n = 0; n < MAX_NETS; n++) {
                survival_graph->dataset[2*n].points = 0;
            }
        }
    }
    else {
        survival_graph->dataset[2*MAX_NETS].points = 0;
        for (n = 0; n < MAX_NETS; n++) {
            survival_graph->dataset[2*n].points = 0;
        }
    }

    if ((sv_task.base == TASK_TEA) || (sv_task.base == TASK_MAX)) {
        steps = 20;
        if (sv_data[0][TASK_TEA][0] > 0) {
            baseline = sim2_get_survival_baseline(TASK_TEA);
            for (i = 0; i < steps; i++) {
                survival_graph->dataset[2*MAX_NETS+1].x[i] = i;
                mean = sim2_get_survival_stats_mean(TASK_TEA, i);
                survival_graph->dataset[2*MAX_NETS+1].y[i] = 100 * mean / baseline;
                survival_graph->dataset[2*MAX_NETS+1].dy1[i] = 0;
                survival_graph->dataset[2*MAX_NETS+1].dy2[i] = 0;
            }
            survival_graph->dataset[2*MAX_NETS+1].points = steps;

            // Set data points for all other lines ... FIX
            for (n = 0; n < MAX_NETS; n++) {
                for (i = 0; i < steps; i++) {
                    survival_graph->dataset[2*n+1].x[i] = i;
                    survival_graph->dataset[2*n+1].y[i] = 100 * sv_data[n][TASK_TEA][i] / baseline;
                    survival_graph->dataset[2*n+1].dy1[i] = 0;
                    survival_graph->dataset[2*n+1].dy2[i] = 0;
                }
                survival_graph->dataset[2*n+1].points = steps;
            }
        }
        else {
            survival_graph->dataset[2*MAX_NETS+1].points = 0;
            for (n = 0; n < MAX_NETS; n++) {
                survival_graph->dataset[2*n+1].points = 0;
            }
        }
    }
    else {
        survival_graph->dataset[2*MAX_NETS+1].points = 0;
        for (n = 0; n < MAX_NETS; n++) {
            survival_graph->dataset[2*n+1].points = 0;
        }
    }

    cairox_draw_graph(cr, survival_graph, xg.colour);
}

static Boolean survival_viewer_expose(GtkWidget *widg, GdkEvent *event, void *data)
{
    if (!GTK_WIDGET_MAPPED(viewer_widget) || (viewer_surface == NULL)) {
        return(TRUE);
    }
    else {
	int height = viewer_widget->allocation.height;
	int width  = viewer_widget->allocation.width;
        cairo_t *cr;

        cr = cairo_create(viewer_surface);
        survival_graph_write_to_cairo(cr, width, height);
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

static void survival_viewer_reset_callback(GtkWidget *mi, void *dummy)
{
    int i, t, n;

    for (i = 0; i < MAX_STEPS; i++) {
        for (t = 0; t < TASK_MAX; t++) {
            for (n = 0; n < MAX_NETS; n++) {
                sv_data[n][t][i] = 0;
            }
        }
    }
    survival_viewer_expose(NULL, NULL, NULL);
}

static void survival_viewer_set_task_callback(GtkWidget *button, void *task)
{
    if (GTK_TOGGLE_BUTTON(button)->active) {
        // Add the selected task from sv_task.base:
        if (sv_task.base == TASK_NONE) {
            sv_task.base = (BaseTaskType) task;
        }
        else if ((sv_task.base == TASK_COFFEE) && ((BaseTaskType) task == TASK_TEA)) {
            sv_task.base = TASK_MAX;
        }
        else if ((sv_task.base == TASK_TEA) && ((BaseTaskType) task == TASK_COFFEE)) {
            sv_task.base = TASK_MAX;
        }
    }
    else {
        // Remove the selected task from sv_task.base:
        if ((sv_task.base == TASK_MAX) && ((BaseTaskType) task == TASK_TEA)) {
            sv_task.base = TASK_COFFEE;
        }
        else if ((sv_task.base == TASK_MAX) && ((BaseTaskType) task == TASK_COFFEE)) {
            sv_task.base = TASK_TEA;
        }
        else if (sv_task.base == (BaseTaskType) task) {
            sv_task.base = TASK_NONE;
        }
    }
    sv_task.initial_state.bowl_closed = pars.sugar_closed;
    sv_task.initial_state.mug_contains_coffee = pars.mug_with_coffee; 
    sv_task.initial_state.mug_contains_tea = pars.mug_with_tea;
    sv_task.initial_state.mug_contains_cream = pars.mug_with_cream;
    sv_task.initial_state.mug_contains_sugar = pars.mug_with_sugar; 
    survival_viewer_reset_callback(NULL, NULL);
}

static void survival_viewer_run_callback(GtkWidget *mi, void *count)
{
    int j;
    int net_count = 0;
    Network *saved_net;

    sv_task.initial_state.bowl_closed = pars.sugar_closed;
    sv_task.initial_state.mug_contains_coffee = pars.mug_with_coffee; 
    sv_task.initial_state.mug_contains_tea = pars.mug_with_tea;
    sv_task.initial_state.mug_contains_cream = pars.mug_with_cream;
    sv_task.initial_state.mug_contains_sugar = pars.mug_with_sugar; 

    // Save existing network so it can be restored once we're done:
    saved_net = network_copy(xg.net);

    while ((net_count < MAX_NETS) && (sim2_load_weights_from_disk(net_count))) {
        j = (long) count;
        while (j-- > 0) {
            if (sv_task.damage == DAMAGE_ACTIVATION_NOISE) {
                if (sv_task.base == TASK_MAX) {
                    sv_task.base = TASK_COFFEE;
                    sv_run_and_score_activation_noise(xg.net, net_count, &sv_task);
                    sv_task.base = TASK_TEA;
                    sv_run_and_score_activation_noise(xg.net, net_count, &sv_task);
                    sv_task.base = TASK_MAX;
                }
                else if (sv_task.base != TASK_NONE) {
                    sv_run_and_score_activation_noise(xg.net, net_count, &sv_task);
                }
            }
            else if (sv_task.damage == DAMAGE_CH_WEIGHT_NOISE) {
                Network *tmp_net;
                if (sv_task.base == TASK_MAX) {
                    sv_task.base = TASK_COFFEE;
                    tmp_net = network_copy(xg.net);
                    sv_run_and_score_ch_weight_noise(tmp_net, net_count, &sv_task);
                    network_tell_destroy(tmp_net);
                    sv_task.base = TASK_TEA;
                    tmp_net = network_copy(xg.net);
                    sv_run_and_score_ch_weight_noise(tmp_net, net_count, &sv_task);
                    network_tell_destroy(tmp_net);
                    sv_task.base = TASK_MAX;
                }
                else if (sv_task.base != TASK_NONE) {
                    tmp_net = network_copy(xg.net);
                    sv_run_and_score_ch_weight_noise(tmp_net, net_count, &sv_task);
                    network_tell_destroy(tmp_net);
                }
            }
            else if (sv_task.damage == DAMAGE_CH_WEIGHT_LESION) {
                Network *tmp_net;
                if (sv_task.base == TASK_MAX) {
                    sv_task.base = TASK_COFFEE;
                    tmp_net = network_copy(xg.net);
                    sv_run_and_score_ch_weight_lesion(tmp_net, net_count, &sv_task);
                    network_tell_destroy(tmp_net);
                    sv_task.base = TASK_TEA;
                    tmp_net = network_copy(xg.net);
                    sv_run_and_score_ch_weight_lesion(tmp_net, net_count, &sv_task);
                    network_tell_destroy(tmp_net);
                    sv_task.base = TASK_MAX;
                }
                else if (sv_task.base != TASK_NONE) {
                    tmp_net = network_copy(xg.net);
                    sv_run_and_score_ch_weight_lesion(tmp_net, net_count, &sv_task);
                    network_tell_destroy(tmp_net);
                }
            }
            else if (sv_task.damage == DAMAGE_IH_WEIGHT_NOISE) {
                Network *tmp_net;
                if (sv_task.base == TASK_MAX) {
                    sv_task.base = TASK_COFFEE;
                    tmp_net = network_copy(xg.net);
                    sv_run_and_score_ih_weight_noise(tmp_net, net_count, &sv_task);
                    network_tell_destroy(tmp_net);
                    sv_task.base = TASK_TEA;
                    tmp_net = network_copy(xg.net);
                    sv_run_and_score_ih_weight_noise(tmp_net, net_count, &sv_task);
                    network_tell_destroy(tmp_net);
                    sv_task.base = TASK_MAX;
                }
                else if (sv_task.base != TASK_NONE) {
                    tmp_net = network_copy(xg.net);
                    sv_run_and_score_ih_weight_noise(tmp_net, net_count, &sv_task);
                    network_tell_destroy(tmp_net);
                }
            }
            else if (sv_task.damage == DAMAGE_IH_WEIGHT_LESION) {
                Network *tmp_net;
                if (sv_task.base == TASK_MAX) {
                    sv_task.base = TASK_COFFEE;
                    tmp_net = network_copy(xg.net);
                    sv_run_and_score_ih_weight_lesion(tmp_net, net_count, &sv_task);
                    network_tell_destroy(tmp_net);
                    sv_task.base = TASK_TEA;
                    tmp_net = network_copy(xg.net);
                    sv_run_and_score_ih_weight_lesion(tmp_net, net_count, &sv_task);
                    network_tell_destroy(tmp_net);
                    sv_task.base = TASK_MAX;
                }
                else if (sv_task.base != TASK_NONE) {
                    tmp_net = network_copy(xg.net);
                    sv_run_and_score_ih_weight_lesion(tmp_net, net_count, &sv_task);
                    network_tell_destroy(tmp_net);
                }
            }
            else if (sv_task.damage == DAMAGE_CONTEXT_ABLATE) {
                Network *tmp_net;
                if (sv_task.base == TASK_MAX) {
                    sv_task.base = TASK_COFFEE;
                    tmp_net = network_copy(xg.net);
                    sv_run_and_score_context_ablate(tmp_net, net_count, &sv_task);
                    network_tell_destroy(tmp_net);
                    sv_task.base = TASK_TEA;
                    tmp_net = network_copy(xg.net);
                    sv_run_and_score_context_ablate(tmp_net, net_count, &sv_task);
                    network_tell_destroy(tmp_net);
                    sv_task.base = TASK_MAX;
                }
                else if (sv_task.base != TASK_NONE) {
                    tmp_net = network_copy(xg.net);
                    sv_run_and_score_context_ablate(tmp_net, net_count, &sv_task);
                    network_tell_destroy(tmp_net);
                }
            }
            else if (sv_task.damage == DAMAGE_WEIGHT_SCALE) {
                Network *tmp_net;
                if (sv_task.base == TASK_MAX) {
                    sv_task.base = TASK_COFFEE;
                    tmp_net = network_copy(xg.net);
                    sv_run_and_score_weight_scale(tmp_net, net_count, &sv_task);
                    network_tell_destroy(tmp_net);
                    sv_task.base = TASK_TEA;
                    tmp_net = network_copy(xg.net);
                    sv_run_and_score_weight_scale(tmp_net, net_count, &sv_task);
                    network_tell_destroy(tmp_net);
                    sv_task.base = TASK_MAX;
                }
                else if (sv_task.base != TASK_NONE) {
                    tmp_net = network_copy(xg.net);
                    sv_run_and_score_weight_scale(tmp_net, net_count, &sv_task);
                    network_tell_destroy(tmp_net);
                }
            }
            else {
                fprintf(stdout, "WARNING: Damage type %d not implemented\n", sv_task.damage);
            }
        }
        net_count++;
    }
    // Expose the viewer
    survival_viewer_expose(NULL, NULL, NULL);
    // Restore the saved network
    network_tell_destroy(xg.net);
    xg.net = saved_net;
}

static void set_damage_callback(GtkWidget *cb, void *dummy)
{
    sv_task.damage = (DamageType) (1 + gtk_combo_box_get_active(GTK_COMBO_BOX(cb)));
    sv_task.initial_state.bowl_closed = pars.sugar_closed;
    sv_task.initial_state.mug_contains_coffee = pars.mug_with_coffee; 
    sv_task.initial_state.mug_contains_tea = pars.mug_with_tea;
    sv_task.initial_state.mug_contains_cream = pars.mug_with_cream;
    sv_task.initial_state.mug_contains_sugar = pars.mug_with_sugar; 
    survival_viewer_reset_callback(NULL, NULL);
}

static void callback_spin_noise(GtkWidget *widget, GtkSpinButton *tmp)
{
    sv_level = gtk_spin_button_get_value(tmp);
}

static void viewer_widget_snap_callback(GtkWidget *button, void *dummy)
{
    if (viewer_surface != NULL) {
        cairo_surface_t *pdf_surface;
        cairo_t *cr;
        char filename[128];
        int width = 710;
        int height = 355; // Was 400;

        g_snprintf(filename, 128, "%sbp_survival_%d.png", OUTPUT_FOLDER, (int) sv_task.damage);
        cairo_surface_write_to_png(viewer_surface, filename);
        fprintf(stdout, "Chart written to %s\n", filename);
        g_snprintf(filename, 128, "%sbp_survival_%d.pdf", OUTPUT_FOLDER, (int) sv_task.damage);
        pdf_surface = cairo_pdf_surface_create(filename, width, height);
        cr = cairo_create(pdf_surface);
        survival_graph_write_to_cairo(cr, width, height);
        fprintf(stdout, "Chart written to %s\n", filename);
        cairo_destroy(cr);
        cairo_surface_destroy(pdf_surface);
    }
}

/*----------------------------------------------------------------------------*/

static void destroy_graph(GtkWidget *viewer, void *dummy)
{
    graph_destroy(survival_graph);
}

/*----------------------------------------------------------------------------*/

void create_bp_sim2_survival_plot_viewer(GtkWidget *vbox)
{
    GtkWidget *page = gtk_vbox_new(FALSE, 0);
    GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
    GtkWidget *toolbar, *tmp;
    GtkAdjustment *adj;
    CairoxFontProperties *fp;

    initialise_sv_data();

    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_TEXT);
    gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(hbox), toolbar, FALSE, FALSE, 0);
    gtk_widget_show(toolbar);

    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Reset", NULL, NULL, NULL, G_CALLBACK(survival_viewer_reset_callback), NULL);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "1", NULL, NULL, NULL, G_CALLBACK(survival_viewer_run_callback), (void *)1);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "10", NULL, NULL, NULL, G_CALLBACK(survival_viewer_run_callback), (void *)10);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "100", NULL, NULL, NULL, G_CALLBACK(survival_viewer_run_callback), (void *)100);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "500", NULL, NULL, NULL, G_CALLBACK(survival_viewer_run_callback), (void *)500);

    /*--- Padding: ---*/
    tmp = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 5);
    gtk_widget_show(tmp);

    /**** Widgets for setting the type of damage: ****/

    tmp = gtk_label_new("Damage:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    tmp = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(tmp), sv_label[0]);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(tmp), sv_label[1]);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(tmp), sv_label[2]);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(tmp), sv_label[3]);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(tmp), sv_label[4]);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(tmp), sv_label[5]);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(tmp), sv_label[6]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), sv_task.damage - 1);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(tmp), "changed", G_CALLBACK(set_damage_callback), NULL);
    gtk_widget_show(tmp);

    /*--- Padding: ---*/
    tmp = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 5);
    gtk_widget_show(tmp);

    /**** Widgets for adjusting the context noise: ****/
    /*--- The label: ---*/
    tmp = gtk_label_new("Level:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    /*--- The spin button: ---*/
    adj = (GtkAdjustment *)gtk_adjustment_new(sv_level, 0.0, 1.0, 0.005, 0.10, 0);
    tmp = gtk_spin_button_new(adj, 0.1, 3);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tmp), TRUE);
    gtk_widget_set_size_request(GTK_WIDGET(tmp), 75, -1);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(adj), "value_changed", G_CALLBACK(callback_spin_noise), tmp);
    gtk_widget_show(tmp);

    /*--- Padding: ---*/
    tmp = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 5);
    gtk_widget_show(tmp);

    /**** Widgets for setting the task: ****/

    tmp = gtk_check_button_new_with_label("Coffee");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), ((sv_task.base == TASK_COFFEE) || (sv_task.base == TASK_MAX)));
    g_signal_connect(G_OBJECT(tmp), "toggled", G_CALLBACK(survival_viewer_set_task_callback), (void *)TASK_COFFEE);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 3);
    gtk_widget_show(tmp);

    tmp = gtk_check_button_new_with_label("Tea");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), ((sv_task.base == TASK_TEA) || (sv_task.base == TASK_MAX)));
    g_signal_connect(G_OBJECT(tmp), "toggled", G_CALLBACK(survival_viewer_set_task_callback), (void *)TASK_TEA);
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

    /**** The survival plot graph: ****/

    viewer_widget = gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(viewer_widget), "expose_event", G_CALLBACK(survival_viewer_expose), NULL);
    g_signal_connect(G_OBJECT(viewer_widget), "configure_event", G_CALLBACK(gtkx_configure_surface_callback), &viewer_surface);
    g_signal_connect(G_OBJECT(viewer_widget), "destroy", G_CALLBACK(gtkx_destroy_surface_callback), &viewer_surface);
    g_signal_connect(G_OBJECT(viewer_widget), "destroy", G_CALLBACK(destroy_graph), NULL);
    gtk_widget_set_events(viewer_widget, GDK_EXPOSURE_MASK);
    gtk_box_pack_start(GTK_BOX(page), viewer_widget, TRUE, TRUE, 0);
    gtk_widget_show(viewer_widget);

    survival_graph = graph_create(2*(MAX_NETS+1));
    if ((fp = font_properties_create("Sans", 22, PANGO_STYLE_NORMAL, PANGO_WEIGHT_NORMAL, "black")) != NULL) {
        graph_set_title_font_properties(survival_graph, fp);
        font_properties_set_size(fp, 18);
        graph_set_legend_font_properties(survival_graph, fp);
        graph_set_axis_font_properties(survival_graph, GTK_ORIENTATION_HORIZONTAL, fp);
        graph_set_axis_font_properties(survival_graph, GTK_ORIENTATION_VERTICAL, fp);
        font_properties_destroy(fp);
    }
    graph_set_margins(survival_graph, 72, 26, 30, 45);

    gtk_container_add(GTK_CONTAINER(vbox), page);
    gtk_widget_show(page);
}

/******************************************************************************/
