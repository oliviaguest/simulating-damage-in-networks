
#include "xframe.h"
#include "lib_cairox_2_0.h"
#include "xcs_sequences.h"

#define NOISE_CATEGORIES 11
#define MAX_STEPS 100

extern double vector_maximum_activation(double *vector_out, int width);
static TaskType cs_sim1_task = {TASK_COFFEE, DAMAGE_NONE, {FALSE, FALSE, FALSE, FALSE, FALSE}};
static double cs_sim1_noise[NOISE_CATEGORIES] = {0.00, 0.05, 0.10, 0.15, 0.20, 0.25, 0.30, 0.35, 0.40, 0.45, 0.50};

extern GList *analyse_actions_with_acs2(TaskType *task, ACS2 *results);
extern void analyse_actions_code_free_error_list(GList *errors);
extern void xgraph_set_error_scores();
extern void initialise_widgets();
static double test_threshold = 0.0;

/******************************************************************************/

static void load_weight_file(int l)
{
    char file[64], buffer[128];
    FILE *fp;
    int j;

    /* Set this by hand for each weight set: */

    g_snprintf(file, 64, "DATA_SIMULATION_1/weights_0%d.srn", l);

    if ((fp = fopen(file, "r")) == NULL) {
        g_snprintf(buffer, 128, "ERROR: Cannot read %s ... weights not restored", file);
    }
    else if ((j = network_restore_weights(fp, xg.net)) > 0) {
        g_snprintf(buffer, 128, "ERROR: Weight file format error %d ... weights not restored", j);
        fclose(fp);
    }
    else {
        gtk_label_set_text(GTK_LABEL(xg.weight_history_label), file);
        g_snprintf(buffer, 128, "Weights successfully restored from %s", file);

        /* Initialise the graph and various viewers: */
        xgraph_set_error_scores();
        initialise_widgets();

        fclose(fp);
    }
    fprintf(stdout, "%s\n", buffer);
}

/******************************************************************************/

static Boolean record_exists(GtkListStore *store, char *title, GtkTreeIter *iter)
{
    if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), iter)) {
        return(FALSE);
    }
    else {
        char *label;
        do {
            gtk_tree_model_get(GTK_TREE_MODEL(store), iter, 0, &label, -1);
            if (strcmp(title, label) == 0) {
                return(TRUE);
            }
            free(label);
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(store), iter));
        return(FALSE);
    }
}

static void cs_sim1_increment_score(GtkListStore *store, int i, char *title)
{
    GtkTreeIter iter;

    if (record_exists(store, title, &iter)) {
        int k, t;

        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, i+1, &k, NOISE_CATEGORIES+1, &t, -1);
        gtk_list_store_set(store, &iter, 0, title, i+1, k+1, NOISE_CATEGORIES+1, t+1, -1);
    }
    else {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, title, i+1, 1, NOISE_CATEGORIES+1, 1, -1);
    }
}

static void cs_sim1_run_and_score(GtkListStore *store, TaskType *task, int i)
{
    double *vector_in = (double *)malloc(IN_WIDTH * sizeof(double));
    double *vector_out = (double *)malloc(OUT_WIDTH * sizeof(double));
    ActionType  this[MAX_STEPS];
    ACS2        results;
    int         count = 0;
    GList      *errors = NULL, *tmp;

    world_initialise(task);
    network_tell_randomise_hidden_units(xg.net);
    do {
	world_set_network_input_vector(vector_in);
	network_tell_input(xg.net, vector_in);
	network_tell_propagate2(xg.net);
	network_ask_output(xg.net, vector_out);
        if (vector_maximum_activation(vector_out, OUT_WIDTH) > test_threshold) {
            this[count] = world_get_network_output_action(NULL, vector_out);
            world_perform_action(this[count]);
        }
        else {
	    this[count] = ACTION_NONE;
        }
	/* Now add the noise to the hidden units: */
	network_inject_noise(xg.net, cs_sim1_noise[i]*cs_sim1_noise[i]);
    } while ((this[count] != ACTION_SAY_DONE) && (++count < MAX_STEPS));

    free(vector_in);
    free(vector_out);

    /* Now score the actions: */

    errors = analyse_actions_with_acs2(task, &results);
    for (tmp = errors; tmp != NULL; tmp = tmp->next) {
        cs_sim1_increment_score(store, i, tmp->data);
    }
    analyse_actions_code_free_error_list(errors);

    gtkx_flush_events();
}

/*****************************************************************************/

static void increment_label_count(GtkListStore *store)
{
    GtkWidget *tmp = g_object_get_data(G_OBJECT(store), "label");
    long count = (long) g_object_get_data(G_OBJECT(store), "count");
    char buffer[40];

    count++;

    if (count == 1) {
        g_snprintf(buffer, 40, "Data from 1 trial of 10 networks");
    }
    else {
        g_snprintf(buffer, 40, "Data from %ld trials of 10 networks", count);
    }

    g_object_set_data(G_OBJECT(store), "count", (void *)count);
    gtk_label_set_text(GTK_LABEL(tmp), buffer);
}

static void initialise_sim1_data(GtkListStore *store)
{
    GtkWidget *tmp = g_object_get_data(G_OBJECT(store), "label");

    gtk_list_store_clear(store);
    g_object_set_data(G_OBJECT(store), "count", 0);
    gtk_label_set_text(GTK_LABEL(tmp), "Data from zero trials");
}

static void error_frequencies_table_reset_callback(GtkWidget *mi, void *dummy)
{
    initialise_sim1_data((GtkListStore *)g_object_get_data(G_OBJECT(mi), "store"));
}

static void error_frequencies_table_export_callback(GtkWidget *mi, void *dummy)
{
    GtkListStore *store = (GtkListStore *)g_object_get_data(G_OBJECT(mi), "store");
    long count = (long) g_object_get_data(G_OBJECT(store), "count");
    BaseTaskType task = (BaseTaskType) g_object_get_data(G_OBJECT(mi), "task");
    GtkTreeIter iter;
    char buffer[64];
    char *label;
    int i, value;
    FILE *fp;

    g_snprintf(buffer, 64, "DATA_SIMULATION_1/results_%s.dat", task_name[task]);

    if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter)) {
        gtkx_warn(xg.frame, "Export aborted: Error frequency table is empty\n");
    }
    else if ((fp = fopen(buffer, "w")) == NULL) {
        gtkx_warn(xg.frame, "Export aborted: Cannot open export file\n");
    }
    else {
        fprintf(fp, "Error frequencies for %ld trial(s) of %d networks on the %s task\n\n", count, 10, (task == TASK_TEA ? "tea" : "coffee"));

        fprintf(fp, "%s", "Error");
        for (i = 0; i < NOISE_CATEGORIES; i++) {
            fprintf(fp, "\t%5.3f", cs_sim1_noise[i]);
        }
        fprintf(fp, "\t%s\n", "Total");

        do {
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &label, -1);
            fprintf(fp, "%s", label);
            for (i = 0; i < NOISE_CATEGORIES; i++) {
                gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, i+1, &value, -1);
                fprintf(fp, "\t%d", value);
            }
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, NOISE_CATEGORIES+1, &value, -1);
            fprintf(fp, "\t%d\n", value);
            free(label);
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter));
        fclose(fp);
        gtkx_warn(xg.frame, "Export succeeded\n");
    }
}

static void error_frequencies_table_step(GtkListStore *store, long count)
{
    int i, l;

    while (count-- > 0) {
        for (l = 0; l < 10; l++) {
            load_weight_file(l);
            for (i = 0; i < NOISE_CATEGORIES; i++) {
                cs_sim1_run_and_score(store, &cs_sim1_task, i);
            }
        }
        increment_label_count(store);
    }
}

static void error_frequencies_table_step_callback(GtkWidget *mi, void *count)
{
    GtkListStore *store;

    store = (GtkListStore *)g_object_get_data(G_OBJECT(mi), "store");
    cs_sim1_task.base = (BaseTaskType) g_object_get_data(G_OBJECT(mi), "task");
    cs_sim1_task.initial_state.bowl_closed = pars.sugar_closed;
    cs_sim1_task.initial_state.mug_contains_coffee = pars.mug_with_coffee;
    cs_sim1_task.initial_state.mug_contains_tea = pars.mug_with_tea;
    cs_sim1_task.initial_state.mug_contains_cream = pars.mug_with_cream;
    cs_sim1_task.initial_state.mug_contains_sugar = pars.mug_with_sugar;

    error_frequencies_table_step(store, (long) count);
}


/******************************************************************************/

void create_cs_sim1_table(GtkWidget *vbox, BaseTaskType task)
{
    GtkWidget *page = gtk_vbox_new(FALSE, 0);
    GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
    GtkWidget *toolbar, *tmp, *list_view;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkListStore *store;

    int i;

    store = gtk_list_store_new(13, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT);

    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_TEXT);
    gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(hbox), toolbar, FALSE, FALSE, 0);
    gtk_widget_show(toolbar);

    tmp = gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Reset", NULL, NULL, NULL, G_CALLBACK(error_frequencies_table_reset_callback), NULL);
    g_object_set_data(G_OBJECT(tmp), "store", store);
    g_object_set_data(G_OBJECT(tmp), "task", (void *)task);
    tmp = gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "1", NULL, NULL, NULL, G_CALLBACK(error_frequencies_table_step_callback), (void *)1);
    g_object_set_data(G_OBJECT(tmp), "store", store);
    g_object_set_data(G_OBJECT(tmp), "task", (void *)task);
    tmp = gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "10", NULL, NULL, NULL, G_CALLBACK(error_frequencies_table_step_callback), (void *)10);
    g_object_set_data(G_OBJECT(tmp), "store", store);
    g_object_set_data(G_OBJECT(tmp), "task", (void *)task);
    tmp = gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "50", NULL, NULL, NULL, G_CALLBACK(error_frequencies_table_step_callback), (void *)50);
    g_object_set_data(G_OBJECT(tmp), "store", store);
    g_object_set_data(G_OBJECT(tmp), "task", (void *)task);
    tmp = gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "100", NULL, NULL, NULL, G_CALLBACK(error_frequencies_table_step_callback), (void *)100);
    g_object_set_data(G_OBJECT(tmp), "store", store);
    g_object_set_data(G_OBJECT(tmp), "task", (void *)task);
    tmp = gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "500", NULL, NULL, NULL, G_CALLBACK(error_frequencies_table_step_callback), (void *)500);
    g_object_set_data(G_OBJECT(tmp), "store", store);
    g_object_set_data(G_OBJECT(tmp), "task", (void *)task);
    tmp = gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Export", NULL, NULL, NULL, G_CALLBACK(error_frequencies_table_export_callback), NULL);
    g_object_set_data(G_OBJECT(tmp), "store", store);
    g_object_set_data(G_OBJECT(tmp), "task", (void *)task);

    /**** The subtask list: ****/

    tmp = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tmp), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(page), tmp, TRUE, TRUE, 0);
    gtk_widget_show(tmp);

    list_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(list_view), TRUE);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(list_view), TRUE);
    gtk_container_add(GTK_CONTAINER(tmp), list_view);
    gtk_widget_show(list_view);
    g_object_unref(store);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Error Type", renderer, "text", 0, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_sort_column_id(column, 0);
    gtk_tree_view_append_column(GTK_TREE_VIEW(list_view), column);

    for (i = 0; i < NOISE_CATEGORIES; i++) {
        char buffer[16];
        g_snprintf(buffer, 16, "%5.3f", cs_sim1_noise[i]);
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes(buffer, renderer, "text", i+1, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_column_set_sort_column_id(column, i+1);
        gtk_tree_view_append_column(GTK_TREE_VIEW(list_view), column);
    }

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Total", renderer, "text", NOISE_CATEGORIES+1, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_sort_column_id(column, NOISE_CATEGORIES+1);
    gtk_tree_view_append_column(GTK_TREE_VIEW(list_view), column);

    gtk_container_add(GTK_CONTAINER(vbox), page);
    gtk_widget_show(page);

    tmp = gtk_label_new("My label");
    gtk_box_pack_start(GTK_BOX(page), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);

    g_object_set_data(G_OBJECT(store), "label", tmp);

    initialise_sim1_data(store);
}

/******************************************************************************/
