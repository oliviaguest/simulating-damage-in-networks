
#include "xframe.h"
#include "lib_cairox_2_0.h"
#include "xcs_sequences.h"

extern void initialise_state(TaskType *task);
extern int categorise_action_sequence(ActionType *sequence);
extern void xgraph_set_error_scores();
extern void initialise_widgets();

/******************************************************************************/

static GtkWidget *viewer_widget = NULL;
static cairo_surface_t *viewer_surface = NULL;

/* The results matrix has 4 dimensions: instruction, weights, sequence */
static int cs_sim2_results[3][10][NUM_VARIANTS];

/******************************************************************************/


Boolean sim2_load_weights_from_disk(int net_count)
{
    char file[64], buffer[128];
    FILE *fp;
    Boolean success;
    int j;

    /* Set this by hand for each weight set: */

    g_snprintf(file, 64, "Weights/srn_20000_%02d.wgt", net_count+1);

    if ((fp = fopen(file, "r")) == NULL) {
        g_snprintf(buffer, 128, "ERROR: Cannot read %s ... weights not restored", file);
        success = FALSE;
    }
    else if ((j = network_restore_weights(fp, xg.net)) > 0) {
        g_snprintf(buffer, 128, "ERROR: Weight file format error %d ... weights not restored", j);
        fclose(fp);
        success = FALSE;
    }
    else {
        gtk_label_set_text(GTK_LABEL(xg.weight_history_label), file);
        g_snprintf(buffer, 128, "Weights successfully restored from %s", file);

        /* Initialise the graph and various viewers: */
        //        xgraph_set_error_scores();
        //        initialise_widgets();

        fclose(fp);
        success = TRUE;
    }
    fprintf(stdout, "%s\n", buffer);
    return(success);
}

/******************************************************************************/

static double mean_result(int j, int i)
{
    double sum = 0;
    int l = 0;

    for (l = 0; l < 10; l++) {
        sum = sum + cs_sim2_results[j][l][i];
    }
    return(sum / (double) l);
}

/*----------------------------------------------------------------------------*/

static void load_weight_file(int l)
{
    char file[64], buffer[128];
    FILE *fp;
    int j;

    /* Set this by hand for each weight set: */

    g_snprintf(file, 64, "DATA_SIMULATION_2/weights_2_%d.dat", l);

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

static Boolean cs_sim2_viewer_expose(GtkWidget *widg, GdkEvent *event, void *data)
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
        int y = 0, i, j, w;

        cr = cairo_create(viewer_surface);
        layout = pango_cairo_create_layout(cr);
        pangox_layout_set_font_size(layout, 12);

        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_paint(cr);
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);

        g_snprintf(buffer, 128, "INSTRUCTION:"); y = y + 15;
        cairox_text_parameters_set(&tp, 10, y, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_line_parameters_set(&lp, 1.0, LS_SOLID, FALSE);
        cairox_paint_pango_text(cr, &tp, layout, buffer);
        cairox_paint_line(cr, &lp, 5, y+2, 255, y+2);

        y = y + 5;
        cairox_line_parameters_set(&lp, 1.0, LS_DOTTED, FALSE);
        for (i = 0; i < NUM_VARIANTS; i++) {
            g_snprintf(buffer, 128, "%s:", variant_name[i]); y = y + 15;
            cairox_text_parameters_set(&tp, 10, y, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, buffer);
            if ((i == 22) || (i == 5) || (i == 33) || (i == 25)) {
                cairox_paint_line(cr, &lp, 5, y+2, 575, y+2);
            }
        }
        y = y + 2;
        cairox_line_parameters_set(&lp, 1.0, LS_SOLID, FALSE);
        cairox_paint_line(cr, &lp, 5, y, 255, y);
        g_snprintf(buffer, 128, "TOTAL:"); y = y + 15;
        cairox_text_parameters_set(&tp, 10, y, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);
        cairox_line_parameters_set(&lp, 1.0, LS_SOLID, FALSE);

        for (j = 0; j < 3; j++) {
            double count = 0.0;

            y = 15;
            g_snprintf(buffer, 128, "%s", task_name[j]);
            w = pangox_layout_get_string_width(layout, buffer);
            cairox_text_parameters_set(&tp, 340+100*j, y, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, buffer);
            cairox_paint_line(cr, &lp, 340+100*j-w/2, y+2, 340+100*j+w/2, y+2);

            y = y + 5;
            for (i = 0; i < NUM_VARIANTS; i++) {
                double mean = mean_result(j, i);
                g_snprintf(buffer, 128, "%4.1f", mean); y = y + 15;
                cairox_text_parameters_set(&tp, 355+100*j, y, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_BOTTOM, 0.0);
                cairox_paint_pango_text(cr, &tp, layout, buffer);
                count += mean;
	    }

            y = y + 2;
            cairox_paint_line(cr, &lp, 325+100*j, y, 355+100*j, y);
            g_snprintf(buffer, 128, "%5.1f", count); y = y + 15;
            cairox_text_parameters_set(&tp, 355+100*j, y, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_BOTTOM, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, buffer);
        }

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

static void initialise_cs_sim2_results()
{
    int i, j, l;
    for (j = 0; j < 3; j++) {
        for (l = 0; l < 10; l++) {
            for (i = 0; i < NUM_VARIANTS; i++) {
                cs_sim2_results[j][l][i] = 0;
            }
        }
    }
}

static int run_one_simulation()
{
    /* Run one simulation and categorise it according to the possible targets */

    double *vector_in = (double *)malloc(IN_WIDTH * sizeof(double));
    double *vector_out = (double *)malloc(OUT_WIDTH * sizeof(double));
    int count = 0;
    ActionType this[100];

    do {
        world_set_network_input_vector(vector_in);
        network_tell_input(xg.net, vector_in);
        network_tell_propagate2(xg.net);
        network_ask_output(xg.net, vector_out);
        this[count] = world_get_network_output_action(NULL, vector_out);
        world_perform_action(this[count]);
    } while ((this[count] != ACTION_SAY_DONE) && (count++ < 100));

    free(vector_in);
    free(vector_out);

    return(categorise_action_sequence(this));
}

static void generate_results_for_n_runs(int l, long count)
{
    int tt, i, s;

    for (i = 0; i < count; i++) {
        for (tt = 0; tt < 3; tt++) {
            TaskType task = {(BaseTaskType) tt, DAMAGE_NONE, {pars.sugar_closed, FALSE, FALSE, FALSE, FALSE}};

            initialise_state(&task);
            s = run_one_simulation();
            cs_sim2_results[tt][l][s]++;
        }
        cs_sim2_viewer_expose(NULL, NULL, NULL);
    }
}

static void generate_cs_sim2_results_callback(GtkWidget *mi, void *count)
{
    /* Here we iterate over five weight files and for each of the    */
    /* states of the sugar bowl.                                     */

    int l = 0;

    for (l = 0; l < 10; l++) {
        load_weight_file(l);
        generate_results_for_n_runs(l, (long) count);
    }
}

static void clear_cs_sim2_results_callback(GtkWidget *mi, void *dummy)
{
    initialise_cs_sim2_results();
    if (viewer_widget != NULL) {
        cs_sim2_viewer_expose(NULL, NULL, NULL);
    }
}

void create_cs_sim2_table(GtkWidget *vbox)
{
    GtkWidget *page = gtk_vbox_new(FALSE, 0);
    GtkWidget *toolbar, *hbox;

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_TEXT);
    gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(hbox), toolbar, FALSE, FALSE, 0);
    gtk_widget_show(toolbar);

    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Reset", NULL, NULL, NULL, G_CALLBACK(clear_cs_sim2_results_callback), NULL);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "10 Trials", NULL, NULL, NULL, G_CALLBACK(generate_cs_sim2_results_callback), (void *)10);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "100 Trials", NULL, NULL, NULL, G_CALLBACK(generate_cs_sim2_results_callback), (void *)100);

    /* The viewer: */

    viewer_widget = gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(viewer_widget), "expose_event", G_CALLBACK(cs_sim2_viewer_expose), NULL);
    g_signal_connect(G_OBJECT(viewer_widget), "configure_event", G_CALLBACK(gtkx_configure_surface_callback), &viewer_surface);
    g_signal_connect(G_OBJECT(viewer_widget), "destroy", G_CALLBACK(gtkx_destroy_surface_callback), &viewer_surface);
    gtk_widget_set_events(viewer_widget, GDK_EXPOSURE_MASK);
    gtk_box_pack_start(GTK_BOX(page), viewer_widget, TRUE, TRUE, 0);
    gtk_widget_show(viewer_widget);

    gtk_container_add(GTK_CONTAINER(vbox), page);
    gtk_widget_show(page);

    initialise_cs_sim2_results();
}

/******************************************************************************/
