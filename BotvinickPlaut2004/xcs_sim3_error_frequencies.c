
#include "xframe.h"
#include "lib_cairox.h"
#include "xcs_sequences.h"

#define ERROR_CATEGORIES  10
#define MAX_LEVELS 11
#define MAX_STEPS 100

static GtkWidget *viewer_widget = NULL;
static cairo_surface_t *viewer_surface = NULL;

static TaskType sc_task = {TASK_COFFEE, DAMAGE_ACTIVATION_NOISE, {FALSE, FALSE, FALSE, FALSE, FALSE}};
static int sc_data[MAX_LEVELS][ERROR_CATEGORIES];
static int sc_accomplishment[MAX_LEVELS];
static int sc_count = 0;

typedef struct damage_details {
    char *label;
    int num_levels;
    double level[MAX_LEVELS];
    char *format;
} DamageDetails;

static DamageDetails sc_dd[5] = {
    {"Activation Noise (s.d.)",    10, { 0.00,  0.01,  0.02,  0.03,  0.04,  0.05,  0.10,  0.20,  0.30,  0.40,  0.00}, "%4.2f"},
    {"CH Weight Noise (s.d.)",        10, {0.000, 0.001, 0.002, 0.005, 0.010, 0.030, 0.050, 0.100, 0.250, 0.400,  0.00}, "%5.3f"},
    {"CH Connections Severed (proportion)", 11, {0.000, 0.001, 0.003, 0.006, 0.009, 0.015, 0.040, 0.080, 0.120, 0.200, 0.400}, "%5.3f"},
    {"IH Weight Noise (s.d.)",        10, {0.000, 0.001, 0.002, 0.005, 0.010, 0.030, 0.050, 0.100, 0.250, 0.400,  0.00}, "%5.3f"},
    {"IH Connections Severed (proportion)", 11, {0.000, 0.001, 0.003, 0.006, 0.009, 0.015, 0.040, 0.080, 0.120, 0.200, 0.400}, "%5.3f"}
};

static char *category_names[ERROR_CATEGORIES] = {
    "Omission",
    "Sequence: Anticipation",
    "Sequence: Perseveration",
    "Sequence: Reversal",
    "Object substitution",
    "Gesture subtitution",
    "Tool omission",
    "Action addition",
    "Quality",
    "Bizarre"};

extern void analyse_actions_with_acs1(TaskType *task, ACS1 *results);
extern GList *analyse_actions_with_acs2(TaskType *task, ACS2 *results);
extern void analyse_actions_code_free_error_list(GList *errors);

/******************************************************************************/

static void sc_run_and_score_activation_noise(Network *net, TaskType *task, int i)
{
    double *vector_in = (double *)malloc(IN_WIDTH * sizeof(double));
    double *vector_out = (double *)malloc(OUT_WIDTH * sizeof(double));
    ActionType this[MAX_STEPS];
    ACS1        res1;
    ACS2        res2;
    int count = 0;
    GList      *errors = NULL;

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
    sc_data[i][2] += res2.perseverations;
    sc_data[i][3] += res2.reversals;
    sc_data[i][4] += res2.object_sub;
    sc_data[i][5] += res2.gesture_sub;
    sc_data[i][6] += res2.tool_omission;
    sc_data[i][7] += res2.action_addition;
    sc_data[i][8] += res2.quality;
    sc_data[i][9] += res2.bizarre;
    if (res2.accomplished) {
        sc_accomplishment[i] += 1;
    }
}

static void sc_run_and_score_ch_weight_noise(Network *net, TaskType *task, int i)
{
    double *vector_in = (double *)malloc(IN_WIDTH * sizeof(double));
    double *vector_out = (double *)malloc(OUT_WIDTH * sizeof(double));
    ActionType this[MAX_STEPS];
    ACS1        res1;
    ACS2        res2;
    int count = 0;
    GList      *errors = NULL;

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
    sc_data[i][2] += res2.perseverations;
    sc_data[i][3] += res2.reversals;
    sc_data[i][4] += res2.object_sub;
    sc_data[i][5] += res2.gesture_sub;
    sc_data[i][6] += res2.tool_omission;
    sc_data[i][7] += res2.action_addition;
    sc_data[i][8] += res2.quality;
    sc_data[i][9] += res2.bizarre;
    if (res2.accomplished) {
        sc_accomplishment[i] += 1;
    }
}

static void sc_run_and_score_ch_weight_lesion(Network *net, TaskType *task, int i)
{
    double *vector_in = (double *)malloc(IN_WIDTH * sizeof(double));
    double *vector_out = (double *)malloc(OUT_WIDTH * sizeof(double));
    ActionType this[MAX_STEPS];
    ACS1        res1;
    ACS2        res2;
    int count = 0;
    GList      *errors = NULL;

    world_initialise(task);
    network_tell_randomise_hidden_units(net);
    /* Just lesion the context to hidden connections: */
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
    sc_data[i][2] += res2.perseverations;
    sc_data[i][3] += res2.reversals;
    sc_data[i][4] += res2.object_sub;
    sc_data[i][5] += res2.gesture_sub;
    sc_data[i][6] += res2.tool_omission;
    sc_data[i][7] += res2.action_addition;
    sc_data[i][8] += res2.quality;
    sc_data[i][9] += res2.bizarre;
    if (res2.accomplished) {
        sc_accomplishment[i] += 1;
    }
}

static void sc_run_and_score_ih_weight_noise(Network *net, TaskType *task, int i)
{
    double *vector_in = (double *)malloc(IN_WIDTH * sizeof(double));
    double *vector_out = (double *)malloc(OUT_WIDTH * sizeof(double));
    ActionType this[MAX_STEPS];
    ACS1        res1;
    ACS2        res2;
    int count = 0;
    GList      *errors = NULL;

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

    analyse_actions_with_acs1(task, &res1);
    errors = analyse_actions_with_acs2(task, &res2);
    analyse_actions_code_free_error_list(errors);

    sc_data[i][0] += res2.omissions;
    sc_data[i][1] += res2.anticipations;
    sc_data[i][2] += res2.perseverations;
    sc_data[i][3] += res2.reversals;
    sc_data[i][4] += res2.object_sub;
    sc_data[i][5] += res2.gesture_sub;
    sc_data[i][6] += res2.tool_omission;
    sc_data[i][7] += res2.action_addition;
    sc_data[i][8] += res2.quality;
    sc_data[i][9] += res2.bizarre;
    if (res2.accomplished) {
        sc_accomplishment[i] += 1;
    }
}

static void sc_run_and_score_ih_weight_lesion(Network *net, TaskType *task, int i)
{
    double *vector_in = (double *)malloc(IN_WIDTH * sizeof(double));
    double *vector_out = (double *)malloc(OUT_WIDTH * sizeof(double));
    ActionType this[MAX_STEPS];
    ACS1        res1;
    ACS2        res2;
    int count = 0;
    GList      *errors = NULL;

    world_initialise(task);
    network_tell_randomise_hidden_units(net);
    /* Just lesion the context to hidden connections: */
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

    analyse_actions_with_acs1(task, &res1);
    errors = analyse_actions_with_acs2(task, &res2);
    analyse_actions_code_free_error_list(errors);

    sc_data[i][0] += res2.omissions;
    sc_data[i][1] += res2.anticipations;
    sc_data[i][2] += res2.perseverations;
    sc_data[i][3] += res2.reversals;
    sc_data[i][4] += res2.object_sub;
    sc_data[i][5] += res2.gesture_sub;
    sc_data[i][6] += res2.tool_omission;
    sc_data[i][7] += res2.action_addition;
    sc_data[i][8] += res2.quality;
    sc_data[i][9] += res2.bizarre;
    if (res2.accomplished) {
        sc_accomplishment[i] += 1;
    }
}

/*****************************************************************************/

static Boolean error_frequencies_table_expose(GtkWidget *widget, GdkEvent *event, void *data)
{
    if (!GTK_WIDGET_MAPPED(viewer_widget) || (viewer_surface == NULL)) {
        return(TRUE);
    }
    else {
        PangoLayout *layout;
        cairo_t *cr;
        CairoxTextParameters tp;
        CairoxLineParameters lp;
        char buffer[128];
	int width  = viewer_widget->allocation.width;
	int i, j, x, y, dx;
	int total_errors, num_categories;

        cr = cairo_create(viewer_surface);
        layout = pango_cairo_create_layout(cr);
        pangox_layout_set_font_size(layout, 12);

        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_paint(cr);

        /* Now draw the table's row headings: */
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairox_line_parameters_set(&lp, 1.0, LS_SOLID, FALSE);

        y = 65;
        cairox_paint_line(cr, &lp, 40, y, 192, y);
        y += 25;
        cairox_text_parameters_set(&tp, 43, y-3, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, "Error Type");
        cairox_paint_line(cr, &lp, 40, y, 192, y);
        y += 2;
        cairox_paint_line(cr, &lp, 40, y, 192, y);
        for (j = 0; j < ERROR_CATEGORIES; j++) {
            y += 25;
            cairox_text_parameters_set(&tp, 43, y-3, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, category_names[j]);
            cairox_paint_line(cr, &lp, 40, y, 192, y);
        }
        y += 2;
        cairox_paint_line(cr, &lp, 40, y, 192, y);
        y += 25;
        cairox_text_parameters_set(&tp, 43, y-3, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, "Errors per trial");
        cairox_paint_line(cr, &lp, 40, y, 192, y);
        y += 25;
        cairox_text_parameters_set(&tp, 43, y-3, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, "Accomplishment");
        cairox_paint_line(cr, &lp, 40, y, 192, y);

        cairox_paint_line(cr, &lp, 40, 65, 40, y);
        cairox_paint_line(cr, &lp, 192, 65, 192, y);

	/* The table data: */
        num_categories = sc_dd[sc_task.damage-1].num_levels;
        x = 192; dx = (int) ((width-232) / num_categories);
        for (i = 0; i < num_categories; i++) {
            y = 65;

            cairox_paint_line(cr, &lp, x, y, x+dx, y);
            y += 25;
            g_snprintf(buffer, 128, sc_dd[sc_task.damage-1].format, sc_dd[sc_task.damage-1].level[i]);
            cairox_text_parameters_set(&tp, x+dx/2, y - 3, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, buffer);
            cairox_paint_line(cr, &lp, x, y, x+dx, y);
            y += 2;
            cairox_paint_line(cr, &lp, x, y, x+dx, y);
            total_errors = 0;
            for (j = 0; j < ERROR_CATEGORIES; j++) {
                total_errors += sc_data[i][j];
            }
            for (j = 0; j < ERROR_CATEGORIES; j++) {
                y += 25;
                g_snprintf(buffer, 128, "%4.1f%%", (total_errors > 0) ? (sc_data[i][j] * 100.0) / (double) total_errors : 0.0);
                cairox_text_parameters_set(&tp, x+dx-3, y - 3, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_BOTTOM, 0.0);
                cairox_paint_pango_text(cr, &tp, layout, buffer);
                cairox_paint_line(cr, &lp, x, y, x+dx, y);
            }
            y += 2;
            cairox_paint_line(cr, &lp, x, y, x+dx, y);

            /* Accomplishment score: */
            y += 25;
            if (sc_count > 0) {
                g_snprintf(buffer, 128, "%4.1f", total_errors / (double) sc_count);
                cairox_text_parameters_set(&tp, x+dx-3, y - 3, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_BOTTOM, 0.0);
                cairox_paint_pango_text(cr, &tp, layout, buffer);
            }
            cairox_paint_line(cr, &lp, x, y, x+dx, y);

            y += 25;
            if (sc_count > 0) {
                g_snprintf(buffer, 128, "%4.1f%%", (sc_accomplishment[i] * 100.0) / (double) sc_count);
                cairox_text_parameters_set(&tp, x+dx-3, y - 3, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_BOTTOM, 0.0);
                cairox_paint_pango_text(cr, &tp, layout, buffer);
            }
            cairox_paint_line(cr, &lp, x, y, x+dx, y);

            cairox_paint_line(cr, &lp, x+dx, 65, x+dx, y);

            x += dx;
        }
        cairox_paint_line(cr, &lp, 192, 40, x, 40);
        cairox_paint_line(cr, &lp, 192, 40, 192, 65);
        cairox_paint_line(cr, &lp, x, 40, x, 65);
        cairox_text_parameters_set(&tp, (192+x)/2, 62, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, sc_dd[sc_task.damage-1].label);

	/* The table caption: */

	if (sc_count == 1) {
	    g_snprintf(buffer, 128, "Frequencies of occurrence of error types for %d trial of the %s task", sc_count, (sc_task.base == TASK_COFFEE ? "coffee" : "tea"));
	}
	else {
	    g_snprintf(buffer, 128, "Frequencies of occurrence of error types for %d trials of the %s task", sc_count, (sc_task.base == TASK_COFFEE ? "coffee" : "tea"));
	}
        cairox_text_parameters_set(&tp, width/2, y + 25, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);

        g_object_unref(layout);
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

static void save_error_frequencies_to_file(GtkWidget *w, GtkWidget *chooser)
{
    char *file = (char *)gtk_file_selection_get_filename(GTK_FILE_SELECTION(chooser));
    char buffer[128];
    double total_errors;
    int i, j, k;
    FILE *fp;

    if ((fp = fopen(file, "w")) == NULL) {
        g_snprintf(buffer, 128, "ERROR: Cannot write to %s", file);
        gtkx_warn(chooser, buffer);
    }
    else {
        fprintf(fp, "%s", sc_dd[sc_task.damage-1].label);
        for (i = 0; i < sc_dd[sc_task.damage-1].num_levels; i++) {
            fprintf(fp, "\t");
            fprintf(fp, sc_dd[sc_task.damage-1].format, sc_dd[sc_task.damage-1].level[i]);
        }
        fprintf(fp, "\n");

        for (j = 0; j < ERROR_CATEGORIES; j++) {
            fprintf(fp, "%s", category_names[j]);
            for (i = 0; i < sc_dd[sc_task.damage-1].num_levels; i++) {
                total_errors = 0;
                for (k = 0; k < ERROR_CATEGORIES; k++) {
                    total_errors += sc_data[i][k];
                }
                fprintf(fp, "\t%4.1f", (total_errors > 0) ? (sc_data[i][j] * 100.0) / (double) total_errors : 0.0);
	    }
	    fprintf(fp, "\n");
	}
        fprintf(fp, "Errors per trial");
        for (i = 0; i < sc_dd[sc_task.damage-1].num_levels; i++) {
            total_errors = 0;
            for (k = 0; k < ERROR_CATEGORIES; k++) {
                total_errors += sc_data[i][k];
            }
            if (sc_count > 0) {
                fprintf(fp, "\t%4.1f", total_errors / (double) sc_count);
            }
            else {
                fprintf(fp, "\t%4.1f", 0.0);
            }
        }
        fprintf(fp, "\n");

        fprintf(fp, "Accomplishment");
        for (i = 0; i < sc_dd[sc_task.damage-1].num_levels; i++) {
            total_errors = 0;
            if (sc_count > 0) {
                fprintf(fp, "\t%4.1f%%", sc_accomplishment[i] * 100.0 / (double) sc_count);
            }
            else {
                fprintf(fp, "\t%4.1f%%", 0.0);
            }
        }
        fprintf(fp, "\n");
        fclose(fp);
    }

    gtk_widget_destroy(chooser);
}

static void destroy_chooser(GtkWidget *button, GtkWidget *chooser)
{
    gtk_widget_destroy(chooser);
}

static void error_frequencies_table_save_callback(GtkWidget *mi, void *frame)
{
    GtkWidget *chooser = gtk_file_selection_new("Save error frequency table");
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(chooser)->ok_button), "clicked", G_CALLBACK(save_error_frequencies_to_file), chooser);
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(chooser)->cancel_button), "clicked", G_CALLBACK(destroy_chooser), chooser);
    gtk_widget_show(chooser);
}

/******************************************************************************/

static void initialise_sc_data()
{
    int i, j;

    sc_count = 0;
    for (i = 0; i < MAX_LEVELS; i++) {
        for (j = 0; j < ERROR_CATEGORIES; j++) {
            sc_data[i][j] = 0;
        }
        sc_accomplishment[i] = 0;
    }
}

static void error_frequencies_table_reset_callback(GtkWidget *mi, void *dummy)
{
    initialise_sc_data();
    error_frequencies_table_expose(NULL, NULL, NULL);
}

static void error_frequencies_table_set_task_callback(GtkWidget *button, void *task)
{
    if (GTK_TOGGLE_BUTTON(button)->active) {
        sc_task.base = (BaseTaskType) task;
        sc_task.initial_state.bowl_closed = pars.sugar_closed;
        sc_task.initial_state.mug_contains_coffee = pars.mug_with_coffee; 
        sc_task.initial_state.mug_contains_tea = pars.mug_with_tea;
        sc_task.initial_state.mug_contains_cream = pars.mug_with_cream;
        sc_task.initial_state.mug_contains_sugar = pars.mug_with_sugar; 
        error_frequencies_table_reset_callback(NULL, NULL);
    }
}

static void set_damage_callback(GtkWidget *cb, void *dummy)
{
    sc_task.damage = (DamageType) (1 + gtk_combo_box_get_active(GTK_COMBO_BOX(cb)));
    sc_task.initial_state.bowl_closed = pars.sugar_closed;
    sc_task.initial_state.mug_contains_coffee = pars.mug_with_coffee; 
    sc_task.initial_state.mug_contains_tea = pars.mug_with_tea;
    sc_task.initial_state.mug_contains_cream = pars.mug_with_cream;
    sc_task.initial_state.mug_contains_sugar = pars.mug_with_sugar; 
    error_frequencies_table_reset_callback(NULL, NULL);
}

static void error_frequencies_table_step_callback(GtkWidget *mi, void *count)
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
        else if (sc_task.damage == DAMAGE_CH_WEIGHT_NOISE) {
            for (i = 0; i < sc_dd[sc_task.damage-1].num_levels; i++) {
                Network *tmp_net = network_copy(xg.net);
                sc_run_and_score_ch_weight_noise(tmp_net, &sc_task, i);
                network_tell_destroy(tmp_net);
            }
	}
        else if (sc_task.damage == DAMAGE_CH_WEIGHT_LESION) {
            for (i = 0; i < sc_dd[sc_task.damage-1].num_levels; i++) {
                Network *tmp_net = network_copy(xg.net);
                sc_run_and_score_ch_weight_lesion(tmp_net, &sc_task, i);
                network_tell_destroy(tmp_net);
            }
	}
        else if (sc_task.damage == DAMAGE_IH_WEIGHT_NOISE) {
            for (i = 0; i < sc_dd[sc_task.damage-1].num_levels; i++) {
                Network *tmp_net = network_copy(xg.net);
                sc_run_and_score_ih_weight_noise(tmp_net, &sc_task, i);
                network_tell_destroy(tmp_net);
            }
	}
        else if (sc_task.damage == DAMAGE_IH_WEIGHT_LESION) {
            for (i = 0; i < sc_dd[sc_task.damage-1].num_levels; i++) {
                Network *tmp_net = network_copy(xg.net);
                sc_run_and_score_ih_weight_lesion(tmp_net, &sc_task, i);
                network_tell_destroy(tmp_net);
            }
	}
        sc_count++;
        error_frequencies_table_expose(NULL, NULL, NULL);
    }
}

/******************************************************************************/

void create_bp_error_frequencies_viewer(GtkWidget *vbox)
{
    GtkWidget *page = gtk_vbox_new(FALSE, 0);
    GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
    GtkWidget *toolbar, *tmp;

    initialise_sc_data();

    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_TEXT);
    gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(hbox), toolbar, FALSE, FALSE, 0);
    gtk_widget_show(toolbar);

    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Reset", NULL, NULL, NULL, G_CALLBACK(error_frequencies_table_reset_callback), NULL);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "1", NULL, NULL, NULL, G_CALLBACK(error_frequencies_table_step_callback), (void *)1);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "10", NULL, NULL, NULL, G_CALLBACK(error_frequencies_table_step_callback), (void *)10);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "100", NULL, NULL, NULL, G_CALLBACK(error_frequencies_table_step_callback), (void *)100);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "500", NULL, NULL, NULL, G_CALLBACK(error_frequencies_table_step_callback), (void *)500);

    /*--- A vertical separator: ---*/
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

    /*--- A vertical separator: ---*/
    tmp = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 5);
    gtk_widget_show(tmp);

    /**** Widgets for setting the task: ****/

    tmp = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    tmp = gtk_radio_button_new_with_label(NULL, "Coffee");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), (sc_task.base == TASK_COFFEE));
    g_signal_connect(G_OBJECT(tmp), "toggled", G_CALLBACK(error_frequencies_table_set_task_callback), (void *)TASK_COFFEE);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 3);
    gtk_widget_show(tmp);

    tmp = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(tmp)), "Tea");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), (sc_task.base == TASK_TEA));
    g_signal_connect(G_OBJECT(tmp), "toggled", G_CALLBACK(error_frequencies_table_set_task_callback), (void *)TASK_TEA);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 3);
    gtk_widget_show(tmp);

    /*--- Padding: ---*/
    tmp = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 5);
    gtk_widget_show(tmp);

    /*--- Take a snapshot of the canvas: ---*/
    tmp = gtk_button_new_with_label("Save");
    g_signal_connect(G_OBJECT(tmp), "clicked", G_CALLBACK(error_frequencies_table_save_callback), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    /**** The subtask chart: ****/

    viewer_widget = gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(viewer_widget), "expose_event", G_CALLBACK(error_frequencies_table_expose), NULL);
    g_signal_connect(G_OBJECT(viewer_widget), "configure_event", G_CALLBACK(gtkx_configure_surface_callback), &viewer_surface);
    g_signal_connect(G_OBJECT(viewer_widget), "destroy", G_CALLBACK(gtkx_destroy_surface_callback), &viewer_surface);
    gtk_widget_set_events(viewer_widget, GDK_EXPOSURE_MASK);
    gtk_box_pack_start(GTK_BOX(page), viewer_widget, TRUE, TRUE, 0);
    gtk_widget_show(viewer_widget);

    gtk_container_add(GTK_CONTAINER(vbox), page);
    gtk_widget_show(page);
}

/******************************************************************************/
