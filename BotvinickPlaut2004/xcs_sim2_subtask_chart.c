
#include "xframe.h"
#include "lib_string.h"
#include "lib_cairox.h"
#include "lib_cairoxg_2_4.h"
#include "xcs_sequences.h"

#define MAX_LEVELS 11
#define ERROR_CATEGORIES  5
#define MAX_STEPS 100

static GtkWidget *viewer_widget = NULL;
static cairo_surface_t *viewer_surface = NULL;

typedef enum subtask_type {
    ST_ERROR, STEEP_TEA, ADD_COFFEE, ADD_CREAM, ADD_SUGAR_FROM_BOWL, ADD_SUGAR_FROM_PACK, DRINK_BEVERAGE
} SubtaskType;

typedef struct subtask_list {
    int                  t0;
    int                  t1;
    SubtaskType          task;
    struct subtask_list *next;
} SubtaskList;

static ActionType ACTIONS_STEEP_TEA[4] = {ACTION_FIXATE_TEABAG, ACTION_PICK_UP, ACTION_FIXATE_CUP, ACTION_DIP};
static ActionType ACTIONS_ADD_COFFEE[10] = {ACTION_FIXATE_COFFEE_PACKET, ACTION_PICK_UP, ACTION_PULL_OPEN, ACTION_FIXATE_CUP, ACTION_POUR, ACTION_FIXATE_SPOON, ACTION_PUT_DOWN, ACTION_PICK_UP, ACTION_FIXATE_CUP, ACTION_STIR};
static ActionType ACTIONS_ADD_CREAM[11] = {ACTION_FIXATE_CARTON, ACTION_PUT_DOWN, ACTION_PICK_UP, ACTION_PEEL_OPEN, ACTION_FIXATE_CUP, ACTION_POUR, ACTION_FIXATE_SPOON, ACTION_PUT_DOWN, ACTION_PICK_UP, ACTION_FIXATE_CUP, ACTION_STIR};
static ActionType ACTIONS_ADD_SUGAR_FROM_BOWL[11] = {ACTION_FIXATE_SUGAR_PACKET, ACTION_PUT_DOWN, ACTION_PULL_OFF, ACTION_FIXATE_SPOON, ACTION_PUT_DOWN, ACTION_PICK_UP, ACTION_FIXATE_SUGAR_BOWL, ACTION_SCOOP, ACTION_FIXATE_CUP, ACTION_POUR, ACTION_STIR};
static ActionType ACTIONS_ADD_SUGAR_FROM_PACK[11] = {ACTION_FIXATE_SUGAR_PACKET, ACTION_PUT_DOWN, ACTION_PICK_UP, ACTION_TEAR_OPEN, ACTION_FIXATE_CUP, ACTION_POUR, ACTION_FIXATE_SPOON, ACTION_PUT_DOWN, ACTION_PICK_UP, ACTION_FIXATE_CUP, ACTION_STIR};
static ActionType ACTIONS_DRINK_BEVERAGE[4] = {ACTION_PUT_DOWN, ACTION_PICK_UP, ACTION_SIP, ACTION_SIP};

static TaskType sc_task = {TASK_COFFEE, DAMAGE_ACTIVATION_NOISE, {FALSE, FALSE, FALSE, FALSE, FALSE}};
static int sc_data[TASK_MAX][MAX_LEVELS][ERROR_CATEGORIES];
static int sc_count = 0;

typedef struct damage_details {
    char *label;
    int num_levels;
    double level[MAX_LEVELS];
    char *format;
} DamageDetails;

static DamageDetails sc_dd[5] = {
    {"Activation Noise (s.d.)",    10, { 0.00,  0.01,  0.02,  0.03,  0.04,  0.05,  0.10,  0.20,  0.30,  0.40,  0.00}, "%4.2f"},
    {"CH Weight Noise (s.d.)",        10, {0.000, 0.001, 0.002, 0.005, 0.010, 0.030, 0.050, 0.100, 0.200, 0.400,  0.00}, "%5.3f"},
    {"CH Connections Severed (proportion)", 10, {0.000, 0.001, 0.003, 0.006, 0.009, 0.015, 0.050, 0.100, 0.200, 0.400, 0.000}, "%5.3f"},
    {"IH Weight Noise (s.d.)",        10, {0.000, 0.001, 0.002, 0.005, 0.010, 0.030, 0.050, 0.100, 0.200, 0.400,  0.00}, "%5.3f"},
    {"IH Connections Severed (proportion)", 10, {0.000, 0.001, 0.003, 0.006, 0.009, 0.015, 0.050, 0.100, 0.200, 0.400, 0.000}, "%5.3f"}
};

static GraphStruct *subtask_chart;

static char *task_description[4] = {"(no task selected)", 
                                    "of the coffee task",
                                    "of the tea task",
                                    "of coffee and tea tasks"};

/******************************************************************************/

static void action_list_print(FILE *fp, ActionType *this)
{
    if (fp != NULL) {
        char buffer[64];
        int i = 0;

        while (i < MAX_STEPS) {
            world_decode_action(buffer, 64, this[i]);
            fprintf(fp, "%2d: %s\n", i+1, buffer);
            if (this[i] != ACTION_SAY_DONE) {
                i++;
            }
            else {
                break;
            }
        }
    }
}

/*----------------------------------------------------------------------------*/

static void stl_free(SubtaskList *subtasks)
{
    SubtaskList *next;

    while (subtasks != NULL) {
        next = subtasks->next;
        free(subtasks);
        subtasks = next;
    }
}

static void stl_print(FILE *fp, SubtaskList *subtasks)
{
    if (fp != NULL) {
        fprintf(fp, "Subtasks:\n");

        while (subtasks != NULL) {
            fprintf(fp, "  %3d - %3d: ", subtasks->t0, subtasks->t1);
            switch (subtasks->task) {
                case ST_ERROR: {
                    fprintf(fp, "Error\n");
                    break;
                }
                case STEEP_TEA: {
                    fprintf(fp, "Steep tea\n");
                    break;
                }
                case ADD_COFFEE: {
                    fprintf(fp, "Add coffee\n");
                    break;
                }
                case ADD_CREAM: {
                    fprintf(fp, "Add cream\n");
                    break;
                }
                case ADD_SUGAR_FROM_BOWL: {
                    fprintf(fp, "Add sugar from bowl\n");
                    break;
                }
                case ADD_SUGAR_FROM_PACK: {
                    fprintf(fp, "Add sugar from pack\n");
                    break;
                }
                case DRINK_BEVERAGE: {
                    fprintf(fp, "Drink beverage\n");
                    break;
                }
            }
            subtasks = subtasks->next;
        }
        fprintf(fp, "End subtasks\n");
    }
}

static int stl_length(SubtaskList *subtasks)
{
    int n = 0;

    while (subtasks != NULL) {
        subtasks = subtasks->next;
        n++;
    }
    return(n);
}

static SubtaskType stl_get_nth(SubtaskList *subtasks, int n)
{
    while ((--n > 0) && (subtasks != NULL)) {
        subtasks = subtasks->next;
    }
    if (subtasks != NULL) {
        return(subtasks->task);
    }
    else {
        return(ST_ERROR);
    }
}

static Boolean stl_contains_subtask(SubtaskList *subtasks, SubtaskType task)
{
    while (subtasks != NULL) {
        if (subtasks->task == task) {
            return(TRUE);
        }
        subtasks = subtasks->next;
    }
    return(FALSE);
}

static Boolean stl_contains_subtask_perseveration(SubtaskList *subtasks)
{
    if (subtasks == NULL) {
        return(FALSE);
    }
    else if (stl_contains_subtask(subtasks->next, subtasks->task)) {
        return(TRUE);
    }
    else if ((subtasks->task == ADD_SUGAR_FROM_PACK) &&  stl_contains_subtask(subtasks->next, ADD_SUGAR_FROM_BOWL)) {
        return(TRUE);
    }
    else if ((subtasks->task == ADD_SUGAR_FROM_BOWL) &&  stl_contains_subtask(subtasks->next, ADD_SUGAR_FROM_PACK)) {
        return(TRUE);
    }
    else {
        return(stl_contains_subtask_perseveration(subtasks->next));
    }
}

static Boolean stl_contains_subtask_omission(SubtaskList *subtasks, TaskType *task)
{
    if (task->base == TASK_TEA) {
        if (!stl_contains_subtask(subtasks, STEEP_TEA)) {
            return(TRUE);
        }
        else if (!stl_contains_subtask(subtasks, ADD_SUGAR_FROM_BOWL) && !stl_contains_subtask(subtasks, ADD_SUGAR_FROM_PACK)) {
            return(TRUE);
        }
        else if (!stl_contains_subtask(subtasks, DRINK_BEVERAGE)) {
            return(TRUE);
        }
        else {
            return(FALSE);
        }
    }
    else if (task->base == TASK_COFFEE) {
        if (!stl_contains_subtask(subtasks, ADD_COFFEE)) {
            return(TRUE);
        }
        else if (!stl_contains_subtask(subtasks, ADD_SUGAR_FROM_BOWL) && !stl_contains_subtask(subtasks, ADD_SUGAR_FROM_PACK)) {
            return(TRUE);
        }
        else if (!stl_contains_subtask(subtasks, ADD_CREAM)) {
            return(TRUE);
        }
        else if (!stl_contains_subtask(subtasks, DRINK_BEVERAGE)) {
            return(TRUE);
        }
        else {
            return(FALSE);
        }
    }
    else {
        return(FALSE);
    }
}

static Boolean stl_contains_subtask_intrusion(SubtaskList *subtasks, TaskType *task)
{
    if (task->base == TASK_TEA) {
        if (stl_contains_subtask(subtasks, ADD_CREAM)) {
            return(TRUE);
        }
    }
    return(FALSE);
}

static Boolean stl_contains_subtask_displacement(SubtaskList *subtasks, TaskType *task)
{
    if (task->base == TASK_TEA) {
        if (stl_length(subtasks) != 3) {
            return(TRUE);
        }
        else if (stl_get_nth(subtasks, 1) != STEEP_TEA) {
            return(TRUE);
        }
        else if (stl_get_nth(subtasks, 3) != DRINK_BEVERAGE) {
            return(TRUE);
        }
        else if (stl_get_nth(subtasks, 2) == ADD_SUGAR_FROM_BOWL) {
            return(FALSE);
        }
        else if (stl_get_nth(subtasks, 2) == ADD_SUGAR_FROM_PACK) {
            return(FALSE);
        }
        else {
            return(TRUE);
        }
    }
    else if (task->base == TASK_COFFEE) {
        if (stl_length(subtasks) != 4) {
            return(TRUE);
        }
        else if (stl_get_nth(subtasks, 1) != ADD_COFFEE) {
            return(TRUE);
        }
        else if (stl_get_nth(subtasks, 4) != DRINK_BEVERAGE) {
            return(TRUE);
        }
        else if ((stl_get_nth(subtasks, 2) == ADD_SUGAR_FROM_BOWL) && (stl_get_nth(subtasks, 3) == ADD_CREAM)) {
            return(FALSE);
        }
        else if ((stl_get_nth(subtasks, 2) == ADD_SUGAR_FROM_PACK) && (stl_get_nth(subtasks, 3) == ADD_CREAM)) {
            return(FALSE);
        }
        else if ((stl_get_nth(subtasks, 3) == ADD_SUGAR_FROM_BOWL) && (stl_get_nth(subtasks, 2) == ADD_CREAM)) {
            return(FALSE);
        }
        else if ((stl_get_nth(subtasks, 3) == ADD_SUGAR_FROM_PACK) && (stl_get_nth(subtasks, 2) == ADD_CREAM)) {
            return(FALSE);
        }
        else {
            return(TRUE);
        }
    }
    else {
        return(FALSE);
    }
}

/*----------------------------------------------------------------------------*/

static Boolean action_sequence_compare(ActionType *sequence, int t0, ActionType *list, int l)
{
    while (l-- > 0) {
        if (sequence[t0+l] != list[l]) {
            return(FALSE);
        }
    }
    return(TRUE);
}

static SubtaskList *get_subtask(ActionType *this, int t0)
{
    SubtaskList *new;

    if ((new = (SubtaskList *)malloc(sizeof(SubtaskList))) == NULL) {
        fprintf(stdout, "Memory allocation failure: aborting\n");
    }
    else if (action_sequence_compare(this, t0, ACTIONS_STEEP_TEA, 4)) {
        new->t0 = t0;
        new->task = STEEP_TEA;
        new->t1 = t0 + 3;
    }
    else if (action_sequence_compare(this, t0, ACTIONS_ADD_COFFEE, 10)) {
        new->t0 = t0;
        new->task = ADD_COFFEE;
        new->t1 = t0 + 9;
    }
    else if (action_sequence_compare(this, t0, ACTIONS_ADD_CREAM, 11)) {
        new->t0 = t0;
        new->task = ADD_CREAM;
        new->t1 = t0 + 10;
    }
    else if (action_sequence_compare(this, t0, ACTIONS_ADD_SUGAR_FROM_BOWL, 11)) {
        new->t0 = t0;
        new->task = ADD_SUGAR_FROM_BOWL;
        new->t1 = t0 + 10;
    }
    else if (action_sequence_compare(this, t0, ACTIONS_ADD_SUGAR_FROM_PACK, 11)) {
        new->t0 = t0;
        new->task = ADD_SUGAR_FROM_PACK;
        new->t1 = t0 + 10;
    }
    else if (action_sequence_compare(this, t0, ACTIONS_DRINK_BEVERAGE, 4)) {
        new->t0 = t0;
        new->task = DRINK_BEVERAGE;
        new->t1 = t0 + 3;
    }
    else {
        free(new);
        new = NULL;
    }
    return(new);
}

/*----------------------------------------------------------------------------*/

static SubtaskList *st_error(int t0)
{
    SubtaskList *new;

    if ((new = (SubtaskList *)malloc(sizeof(SubtaskList))) == NULL) {
        fprintf(stdout, "Memory allocation failure: aborting\n");
    }
    else {
        new->task = ST_ERROR;
        new->t0 = t0;
        new->t1 = t0;
        new->next = NULL;
    }
    return(new);
}

static SubtaskList *parse_subtasks(ActionType *this, int t0)
{
    SubtaskList *st, *new;

    if (t0 >= MAX_STEPS) {
        return(st_error(t0));
    }
    else if (this[t0] == ACTION_SAY_DONE) {
        return(NULL);
    }
    else if ((st = get_subtask(this, t0)) != NULL) {
        st->next = parse_subtasks(this, st->t1 + 1);
        return(st);
    }
    else if ((new = st_error(t0)) != NULL) {
        while ((new->t1 < MAX_STEPS) && ((st = get_subtask(this, new->t1 + 1)) == NULL)) {
            new->t1++;
        }
        new->next = st;
        if (st != NULL) {
            st->next = parse_subtasks(this, st->t1 + 1);
        }
        return(new);
    }
    else {
        return(NULL);
    }
}

/*----------------------------------------------------------------------------*/

static void action_list_score(ActionType this[MAX_STEPS], TaskType *task, int i)
{
    SubtaskList *subtasks;
    int t = (int) task->base;

    subtasks = parse_subtasks(this, 0);

    // Print the actionlist and subtask list to the given file pointer:
    action_list_print(NULL, this);
    stl_print(NULL, subtasks);

    if (stl_contains_subtask(subtasks, ST_ERROR)) {
        sc_data[t][i][4]++;
    }
    else if (stl_contains_subtask_perseveration(subtasks)) {
        sc_data[t][i][2]++;
    }
    else if (stl_contains_subtask_omission(subtasks, task)) {
        sc_data[t][i][1]++;
    }
    else if (stl_contains_subtask_intrusion(subtasks, task)) {
        sc_data[t][i][0]++;
    }
    else if (stl_contains_subtask_displacement(subtasks, task)) {
        sc_data[t][i][3]++;
    }

    stl_free(subtasks);
}

/*----------------------------------------------------------------------------*/

static void sc_run_and_score_activation_noise(Network *net, TaskType *task, int i)
{
    double *vector_in, *vector_out;
    ActionType this[MAX_STEPS];
    int count = 0;

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
    action_list_score(this, task, i);
}

/*----------------------------------------------------------------------------*/

static void sc_run_and_score_ch_weight_noise(Network *net, TaskType *task, int i)
{
    double *vector_in, *vector_out;
    ActionType this[MAX_STEPS];
    int count = 0;

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
    action_list_score(this, task, i);
}

/*----------------------------------------------------------------------------*/

static void sc_run_and_score_ch_weight_lesion(Network *net, TaskType *task, int i)
{
    double *vector_in, *vector_out;
    ActionType this[MAX_STEPS];
    int count = 0;

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
    action_list_score(this, task, i);
}

/*----------------------------------------------------------------------------*/

static void sc_run_and_score_ih_weight_noise(Network *net, TaskType *task, int i)
{
    double *vector_in, *vector_out;
    ActionType this[MAX_STEPS];
    int count = 0;

    vector_in = (double *)malloc(net->in_width * sizeof(double));
    vector_out = (double *)malloc(net->out_width * sizeof(double));

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
    action_list_score(this, task, i);
}

/*----------------------------------------------------------------------------*/

static void sc_run_and_score_ih_weight_lesion(Network *net, TaskType *task, int i)
{
    double *vector_in, *vector_out;
    ActionType this[MAX_STEPS];
    int count = 0;

    vector_in = (double *)malloc(net->in_width * sizeof(double));
    vector_out = (double *)malloc(net->out_width * sizeof(double));

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
    action_list_score(this, task, i);
}

/*****************************************************************************/

static void subtask_chart_write_to_cairo(cairo_t *cr, int width, int height)
{
    char buffer[64];
    char *labels[(2*MAX_LEVELS)+1];
    int i, j, num_categories;

    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);

    graph_set_extent(subtask_chart, 0, 0, width, height);

    graph_set_axis_properties(subtask_chart, GTK_ORIENTATION_VERTICAL, 0.0, 100.0, 11, "%3.0f%%", NULL); // "Percentage Error");
    graph_set_axis_properties(subtask_chart, GTK_ORIENTATION_HORIZONTAL, 0, sc_dd[sc_task.damage-1].num_levels, 2*sc_dd[sc_task.damage-1].num_levels+1, "%2.0f", sc_dd[sc_task.damage-1].label);

    for (j = 0; j < sc_dd[sc_task.damage-1].num_levels; j++) {
        labels[2*j] = "";
        g_snprintf(buffer, 64, sc_dd[sc_task.damage-1].format, sc_dd[sc_task.damage-1].level[j]);
        labels[2*j+1] = string_copy(buffer);
    }
    labels[2*j] = "";
    graph_set_axis_tick_marks(subtask_chart, GTK_ORIENTATION_HORIZONTAL, (2*sc_dd[sc_task.damage-1].num_levels)+1, labels);

    if (sc_count == 1) {
        g_snprintf(buffer, 64, "Subtask analysis for %d trial %s", sc_count, task_description[sc_task.base]);
    }
    else {
        g_snprintf(buffer, 64, "Subtask analysis for %d trials %s", sc_count, task_description[sc_task.base]);
    }
    graph_set_title(subtask_chart, buffer);

    num_categories = sc_dd[sc_task.damage-1].num_levels;

    for (j = 0; j < ERROR_CATEGORIES; j++) {
        for (i = 0; i < num_categories; i++) {
            subtask_chart->dataset[j].x[i] = 0.5 + i;
            subtask_chart->dataset[ERROR_CATEGORIES+j].x[i] = 0.5 + i;
            if ((sc_count == 0) || (sc_task.base == TASK_NONE)) {
                subtask_chart->dataset[j].y[i] = 0.0;
                subtask_chart->dataset[ERROR_CATEGORIES+j].y[i] = 0.0;
            }
            else {
                if (sc_task.base == TASK_MAX) {
                    subtask_chart->dataset[j].y[i] = 100*(sc_data[TASK_COFFEE][i][j] / (double) sc_count);
                    subtask_chart->dataset[ERROR_CATEGORIES+j].y[i] = 100*(sc_data[TASK_TEA][i][j] / (double) sc_count);
                }
                else {
                    subtask_chart->dataset[j].y[i] = 100*(sc_data[(int) sc_task.base][i][j] / (double) sc_count);
                }
            }
            subtask_chart->dataset[j].se[i] = 0.0;
            subtask_chart->dataset[ERROR_CATEGORIES+j].se[i] = 0.0;
        }
        if (sc_task.base == TASK_NONE) {
            subtask_chart->dataset[j].points = 0;
            subtask_chart->dataset[ERROR_CATEGORIES+j].points = 0;
        }
        else if (sc_task.base == TASK_MAX) {
            subtask_chart->dataset[j].points = sc_dd[sc_task.damage-1].num_levels;
            subtask_chart->dataset[ERROR_CATEGORIES+j].points = sc_dd[sc_task.damage-1].num_levels;
        }
        else {
            subtask_chart->dataset[j].points = sc_dd[sc_task.damage-1].num_levels;
            subtask_chart->dataset[ERROR_CATEGORIES+j].points = 0;
        }
    }

    cairox_draw_graph(cr, subtask_chart, xg.colour);
}

static Boolean subtask_chart_expose(GtkWidget *widget, GdkEvent *event, void *data)
{
    if (!GTK_WIDGET_MAPPED(viewer_widget) || (viewer_surface == NULL)) {
        return(TRUE);
    }
    else {
        cairo_t *cr;
        int width  = viewer_widget->allocation.width;
        int height = viewer_widget->allocation.height;

        cr = cairo_create(viewer_surface);
        subtask_chart_write_to_cairo(cr, width, height);
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
    for (j = 0; j < ERROR_CATEGORIES; j++) {
        for (i = 0; i < MAX_LEVELS; i++) {
            for (t = 0; t < TASK_MAX; t++) {
                sc_data[t][i][j] = 0;
            }
        }
    }
}

static void viewer_widget_reset_callback(GtkWidget *mi, void *dummy)
{
    initialise_sc_data();
    subtask_chart_expose(NULL, NULL, NULL);
}

static void adjust_stacked_barchart(GraphStruct *chart, int num_bars, int bar_width)
{
    int i;

    graph_set_stack_bars(chart, num_bars);
    for (i = 0; i < ERROR_CATEGORIES * num_bars; i++) {
        graph_set_dataset_barwidth(chart, i, bar_width);
    }
}

static void viewer_widget_set_task_callback(GtkWidget *button, void *task)
{
    if (GTK_TOGGLE_BUTTON(button)->active) {
        // Add the selected task from sc_task.base:
        if (sc_task.base == TASK_NONE) {
            sc_task.base = (BaseTaskType) task;
            adjust_stacked_barchart(subtask_chart, 1, 40);
        }
        else if ((sc_task.base == TASK_COFFEE) && ((BaseTaskType) task == TASK_TEA)) {
            sc_task.base = TASK_MAX;
            adjust_stacked_barchart(subtask_chart, 2, 20);
        }
        else if ((sc_task.base == TASK_TEA) && ((BaseTaskType) task == TASK_COFFEE)) {
            sc_task.base = TASK_MAX;
            adjust_stacked_barchart(subtask_chart, 2, 20);
        }
    }
    else {
        // Remove the selected task from sc_task.base:
        if ((sc_task.base == TASK_MAX) && ((BaseTaskType) task == TASK_TEA)) {
            sc_task.base = TASK_COFFEE;
            adjust_stacked_barchart(subtask_chart, 1, 40);
        }
        else if ((sc_task.base == TASK_MAX) && ((BaseTaskType) task == TASK_COFFEE)) {
            sc_task.base = TASK_TEA;
            adjust_stacked_barchart(subtask_chart, 1, 40);
        }
        else if (sc_task.base == (BaseTaskType) task) {
            sc_task.base = TASK_NONE;
            adjust_stacked_barchart(subtask_chart, 0, 0);
        }
    }
    sc_task.initial_state.bowl_closed = pars.sugar_closed;
    sc_task.initial_state.mug_contains_coffee = pars.mug_with_coffee; 
    sc_task.initial_state.mug_contains_tea = pars.mug_with_tea;
    sc_task.initial_state.mug_contains_cream = pars.mug_with_cream;
    sc_task.initial_state.mug_contains_sugar = pars.mug_with_sugar; 
    viewer_widget_reset_callback(NULL, NULL);
}

static void set_damage_callback(GtkWidget *cb, void *dummy)
{
    sc_task.damage = (DamageType) (1 + gtk_combo_box_get_active(GTK_COMBO_BOX(cb)));
    sc_task.initial_state.bowl_closed = pars.sugar_closed;
    sc_task.initial_state.mug_contains_coffee = pars.mug_with_coffee; 
    sc_task.initial_state.mug_contains_tea = pars.mug_with_tea;
    sc_task.initial_state.mug_contains_cream = pars.mug_with_cream;
    sc_task.initial_state.mug_contains_sugar = pars.mug_with_sugar; 
    viewer_widget_reset_callback(NULL, NULL);
}

static void viewer_widget_step_callback(GtkWidget *mi, void *count)
{
    if (sc_task.base != TASK_NONE) {
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
                    else {
                        sc_run_and_score_activation_noise(xg.net, &sc_task, i);
                    }
                }
            }
            else if (sc_task.damage == DAMAGE_CH_WEIGHT_NOISE) {
                Network *tmp_net = NULL;
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
                    else {
                        tmp_net = network_copy(xg.net);
                        sc_run_and_score_ch_weight_noise(tmp_net, &sc_task, i);
                        network_tell_destroy(tmp_net);
                    }
                }
            }
            else if (sc_task.damage == DAMAGE_CH_WEIGHT_LESION) {
                Network *tmp_net = NULL;
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
                    else {
                        tmp_net = network_copy(xg.net);
                        sc_run_and_score_ch_weight_lesion(tmp_net, &sc_task, i);
                        network_tell_destroy(tmp_net);
                    }
                }
            }
            else if (sc_task.damage == DAMAGE_IH_WEIGHT_NOISE) {
                Network *tmp_net = NULL;
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
                    else {
                        tmp_net = network_copy(xg.net);
                        sc_run_and_score_ih_weight_noise(tmp_net, &sc_task, i);
                        network_tell_destroy(tmp_net);
                    }
                }
            }
            else if (sc_task.damage == DAMAGE_IH_WEIGHT_LESION) {
                Network *tmp_net = NULL;
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
                    else {
                        tmp_net = network_copy(xg.net);
                        sc_run_and_score_ih_weight_lesion(tmp_net, &sc_task, i);
                        network_tell_destroy(tmp_net);
                    }
                }
            }
            sc_count++;
            subtask_chart_expose(NULL, NULL, NULL);
        }
    }
}

static void viewer_widget_snap_callback(GtkWidget *button, void *dummy)
{
    if (viewer_surface != NULL) {
        cairo_surface_t *pdf_surface;
        cairo_t *cr;
        char filename[64];
        int width = 700;
        int height = 400;

        g_snprintf(filename, 64, "%schart_subtasks_%d.png", OUTPUT_FOLDER, (int) sc_task.damage);
        cairo_surface_write_to_png(viewer_surface, filename);
        fprintf(stdout, "Chart written to %s\n", filename);
        g_snprintf(filename, 64, "%schart_subtasks_%d.pdf", OUTPUT_FOLDER, (int) sc_task.damage);
        pdf_surface = cairo_pdf_surface_create(filename, width, height);
        cr = cairo_create(pdf_surface);
        subtask_chart_write_to_cairo(cr, width, height);
        fprintf(stdout, "Chart written to %s\n", filename);
        cairo_destroy(cr);
        cairo_surface_destroy(pdf_surface);
    }
}

/*----------------------------------------------------------------------------*/

static void destroy_chart(GtkWidget *viewer, void *dummy)
{
    graph_destroy(subtask_chart);
}

/******************************************************************************/

void create_bp_sim2_subtask_chart_viewer(GtkWidget *vbox)
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

    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Reset", NULL, NULL, NULL, G_CALLBACK(viewer_widget_reset_callback), NULL);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "1", NULL, NULL, NULL, G_CALLBACK(viewer_widget_step_callback), (void *)1);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "10", NULL, NULL, NULL, G_CALLBACK(viewer_widget_step_callback), (void *)10);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "100", NULL, NULL, NULL, G_CALLBACK(viewer_widget_step_callback), (void *)100);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "500", NULL, NULL, NULL, G_CALLBACK(viewer_widget_step_callback), (void *)500);

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
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), ((sc_task.base == TASK_COFFEE) || (sc_task.base == TASK_MAX)));
    g_signal_connect(G_OBJECT(tmp), "toggled", G_CALLBACK(viewer_widget_set_task_callback), (void *)TASK_COFFEE);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 3);
    gtk_widget_show(tmp);

    tmp = gtk_check_button_new_with_label("Tea");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), ((sc_task.base == TASK_TEA) || (sc_task.base == TASK_MAX)));
    g_signal_connect(G_OBJECT(tmp), "toggled", G_CALLBACK(viewer_widget_set_task_callback), (void *)TASK_TEA);
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
    g_signal_connect(G_OBJECT(viewer_widget), "expose_event", G_CALLBACK(subtask_chart_expose), NULL);
    g_signal_connect(G_OBJECT(viewer_widget), "configure_event", G_CALLBACK(gtkx_configure_surface_callback), &viewer_surface);
    g_signal_connect(G_OBJECT(viewer_widget), "destroy", G_CALLBACK(gtkx_destroy_surface_callback), &viewer_surface);
    g_signal_connect(G_OBJECT(viewer_widget), "destroy", G_CALLBACK(destroy_chart), NULL);
    gtk_widget_set_events(viewer_widget, GDK_EXPOSURE_MASK);
    gtk_box_pack_start(GTK_BOX(page), viewer_widget, TRUE, TRUE, 0);
    gtk_widget_show(viewer_widget);

    subtask_chart = graph_create(ERROR_CATEGORIES*2);
    if ((fp = font_properties_create("Sans", 22, PANGO_STYLE_NORMAL, PANGO_WEIGHT_NORMAL, "black")) != NULL) {
        graph_set_title_font_properties(subtask_chart, fp);
        font_properties_set_size(fp, 18);
        graph_set_legend_font_properties(subtask_chart, fp);
        graph_set_axis_font_properties(subtask_chart, GTK_ORIENTATION_HORIZONTAL, fp);
        graph_set_axis_font_properties(subtask_chart, GTK_ORIENTATION_VERTICAL, fp);
        font_properties_destroy(fp);
    }
    graph_set_margins(subtask_chart, 72, 26, 30, 45);
    graph_set_dataset_properties(subtask_chart, 0, "Subtask intrusion", 0.3, 0.3, 0.3, 40, LS_SOLID, MARK_NONE);
    graph_set_dataset_properties(subtask_chart, 1, "Subtask omission", 0.0, 0.0, 0.0, 40, LS_SOLID, MARK_NONE);
    graph_set_dataset_properties(subtask_chart, 2, "Subtask perseveration", 1.0, 1.0, 1.0, 40, LS_SOLID, MARK_NONE);
    graph_set_dataset_properties(subtask_chart, 3, "Subtask displacement", 0.6, 0.6, 0.6, 40, LS_SOLID, MARK_NONE);
    graph_set_dataset_properties(subtask_chart, 4, "Within subtask error", 0.9, 0.9, 0.9, 40, LS_SOLID, MARK_NONE);
    graph_set_dataset_properties(subtask_chart, 5, NULL, 0.3, 0.3, 0.3, 40, LS_SOLID, MARK_NONE);
    graph_set_dataset_properties(subtask_chart, 6, NULL, 0.0, 0.0, 0.0, 40, LS_SOLID, MARK_NONE);
    graph_set_dataset_properties(subtask_chart, 7, NULL, 1.0, 1.0, 1.0, 40, LS_SOLID, MARK_NONE);
    graph_set_dataset_properties(subtask_chart, 8, NULL, 0.6, 0.6, 0.6, 40, LS_SOLID, MARK_NONE);
    graph_set_dataset_properties(subtask_chart, 9, NULL, 0.9, 0.9, 0.9, 40, LS_SOLID, MARK_NONE);
    graph_set_legend_properties(subtask_chart, TRUE, 0.0, 0.0, NULL); // "Error Type");
    graph_set_stack_bars(subtask_chart, 1);

    gtk_container_add(GTK_CONTAINER(vbox), page);
    gtk_widget_show(page);
}

/******************************************************************************/
