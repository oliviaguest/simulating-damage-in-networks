
#include "xframe.h"
#include "lib_cairox.h"
#include "xcs_sequences.h"

extern void initialise_state(TaskType *task);
extern int categorise_action_sequence(ActionType *sequence);
extern void xgraph_set_error_scores();
extern void initialise_widgets();
extern int reload_training_data(char *file);

/******************************************************************************/

#define CRITERION 100
#define RANGE 50000

static GtkWidget *viewer_widget = NULL;
static cairo_surface_t *viewer_surface = NULL;

/* The results matrix has 3 dimensions: cycle, task, simulation (4a/4b) */
/* Values in the results matrix are # of correct attempts at criterion task (out of 100) */

static short cs_sim5_results[RANGE][2][2];
static int cs_sim5_cycle;

/******************************************************************************/

static int load_training_set(int s)
{
    char file[128];
    int l;

    g_snprintf(file, 128, "DATA_SIMULATION_5/training_sequences_%d.dat", s+1);
    l = reload_training_data(file);
    fprintf(stdout, "Read %d sequences from %s\n", l, file);
    return(l);
}

/******************************************************************************/

static Boolean cs_sim5_result_log_initialise(int simulation)
{
    char file[64];
    FILE *fp;

    g_snprintf(file, 64, "DATA_SIMULATION_5/results.%d.dat", simulation);
    if ((fp = fopen(file, "w")) != NULL) {
        fclose(fp);
    }
    return(fp == NULL);
}

static void cs_sim5_result_log_append(int simulation)
{
    char file[64];
    FILE *fp;
    int i;

    g_snprintf(file, 64, "DATA_SIMULATION_5/results.%d.dat", simulation);

    if ((fp = fopen(file, "a")) != NULL) {
        for (i = 0; i < 2; i++) {
            fprintf(fp, "%d %d %d\n", i, cs_sim5_cycle, cs_sim5_results[cs_sim5_cycle][i][simulation]);
        }
        fclose(fp);
    }
}

/******************************************************************************/

static void repaint_graph(cairo_t *cr, PangoLayout *layout, int s, int w, int h)
{
    CairoxLineParameters lp;
    CairoxTextParameters tp;
    char buffer[128];
    int x0, y0, x1, y1, i, x, y, xa, ya, t;

    x0 = 40;
    y0 = 30 + s * (h / 2);
    x1 = w - x0;
    y1 = (h * (s + 1)) / 2 - 50;

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairox_line_parameters_set(&lp, 1.0, LS_SOLID, FALSE);

    /* The horizontal lines for the box: */
    cairox_paint_line(cr, &lp, x0, y0, x1, y0);
    cairox_paint_line(cr, &lp, x0, y1, x1, y1);

    /* The vertical lines for the box: */
    cairox_paint_line(cr, &lp, x0, y0, x0, y1);
    cairox_paint_line(cr, &lp, x1, y0, x1, y1);

    /* The vertical grid lines: */

    cairox_line_parameters_set(&lp, 1.0, LS_DOTTED, FALSE);
    for (i = 0; i <= 10; i++) {
	x = x0 + (i * (x1 - x0)) / 10.0;
	g_snprintf(buffer, 8, "%d", (i * RANGE) / 10);
        cairox_text_parameters_set(&tp, x, y1+20, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);
        cairox_paint_line(cr, &lp, x, y0-2, x, y1+1);
    }

    /* The horizontal grid lines: */

    for (i = 0; i <= 5; i++) {
	y = y1 - (i * (y1 - y0)) / 5.0;
	g_snprintf(buffer, 8, "%d%%", i * 20);
        cairox_text_parameters_set(&tp, x0-5, y, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_CENTER, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);
        cairox_paint_line(cr, &lp, x0, y, x1, y);
    }

    /* The data: */
    cairo_set_dash(cr, NULL, 0, 0);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);

    for (t = 0; t < 2; t++) {
        xa = x0;
        ya = y1 - ((cs_sim5_results[0][t][s] * (y1 - y0)) / 100);
        cairo_move_to(cr, xa, ya);
        for (i = 1; i < cs_sim5_cycle; i++) {
            xa = x0 + (i * (x1 - x0)) / RANGE;
            ya = y1 - ((cs_sim5_results[i][t][s] * (y1 - y0)) / 100);
            cairo_line_to(cr, xa, ya);
        }
        cairox_set_colour_by_index(cr, 2*(t+1));
        cairo_stroke(cr);
    }

    /* The horizontal axis label: */
    g_snprintf(buffer, 128, "Epoch");
    cairox_text_parameters_set(&tp, x/2, y1+38, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
    cairox_paint_pango_text(cr, &tp, layout, buffer);

    /* The graph caption: */
    g_snprintf(buffer, 128, "Trained initially on the %s tasks", (s == 1 ? "tea" : "coffee"));
    cairox_text_parameters_set(&tp, x/2, y0-10, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
    cairox_paint_pango_text(cr, &tp, layout, buffer);
}

static Boolean cs_sim5_viewer_expose(GtkWidget *widget, GdkEvent *event, void *data)
{
    if (!GTK_WIDGET_MAPPED(viewer_widget) || (viewer_surface == NULL)) {
        return(TRUE);
    }
    else {
        int w = viewer_widget->allocation.width;
        int h = viewer_widget->allocation.height;
        PangoLayout *layout;
        cairo_t *cr;

        cr = cairo_create(viewer_surface);
        layout = pango_cairo_create_layout(cr);
        pangox_layout_set_font_size(layout, 12);

        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_paint(cr);

        repaint_graph(cr, layout, 0, w, h);
        repaint_graph(cr, layout, 1, w, h);

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

static void initialise_cs_sim5_results()
{
    int j, k, l;

    for (j = 0; j < RANGE; j++) {
        for (k = 0; k < 2; k++) {
            for (l = 0; l < 2; l++) {
                cs_sim5_results[j][k][l] = 0;
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

static int test_criterion(int c, int s)
{
    int i, j, r = 0;

    for (i = 0; i < CRITERION; i++) {
        TaskType task = {(BaseTaskType) s+1, DAMAGE_NONE, {TRUE, FALSE, FALSE, FALSE, FALSE}};

        initialise_state(&task);
        j = run_one_simulation();

        if (s == 0) {
            /* Coffee criterion: */
            if ((j == SEQ_COFFEE1) || (j == SEQ_COFFEE2) || (j == SEQ_COFFEE3) || (j == SEQ_COFFEE4)) {
                r++;
            }
	}
        else {
            /* Tea criterion: */
            if ((j == SEQ_TEA1) || (j == SEQ_TEA2)) {
                r++;
            }
        }
    }
    return(r);
}

static Boolean train_to_criterion(int simulation, int task)
{
    /* Return TRUE if both criteria are simultaneously satisfied. Otherwise   */
    /* return FALSE when the specified criterion is satisfied.                */

    do {
        cs_sim5_results[cs_sim5_cycle][0][simulation] = test_criterion(cs_sim5_cycle, 0);
        cs_sim5_results[cs_sim5_cycle][1][simulation] = test_criterion(cs_sim5_cycle, 1);

        cs_sim5_result_log_append(simulation);

        if (cs_sim5_results[cs_sim5_cycle][task][simulation] == CRITERION) {
            return(cs_sim5_results[cs_sim5_cycle][1-task][simulation] == CRITERION);
	}
	else {
            network_train(xg.net, xg.first, pars.lr, pars.ef, pars.wut, pars.penalty);
            gtkx_flush_events();
            cs_sim5_cycle++;
        }
    } while (cs_sim5_cycle < RANGE);
    return(FALSE);
}

static void generate_cs_sim5_results(int simulation)
{
    Boolean done = FALSE;
    int task;

    cs_sim5_cycle = 0;
    task = simulation;
    network_tell_initialise(xg.net);

    while ((cs_sim5_cycle < RANGE) && !done) {
        load_training_set(task);
        done = train_to_criterion(simulation, task);
        task = 1 - task;
        cs_sim5_viewer_expose(NULL, NULL, NULL);
        gtkx_flush_events();
    }
}

static void generate_cs_sim5_results_callback(GtkWidget *mi, void *dummy)
{
    if (cs_sim5_result_log_initialise(1)) {
        generate_cs_sim5_results(1);
    }
    if (cs_sim5_result_log_initialise(0)) {
        generate_cs_sim5_results(0);
    }
}

static void clear_cs_sim5_results_callback(GtkWidget *mi, void *dummy)
{
    initialise_cs_sim5_results();
    cs_sim5_viewer_expose(NULL, NULL, NULL);
}

void create_cs_sim5_graph(GtkWidget *vbox)
{
    GtkWidget *page = gtk_vbox_new(FALSE, 0);
    GtkWidget *toolbar;

    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_TEXT);
    gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(page), toolbar, FALSE, FALSE, 0);
    gtk_widget_show(toolbar);

    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Reset", NULL, NULL, NULL, G_CALLBACK(clear_cs_sim5_results_callback), NULL);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Run", NULL, NULL, NULL, G_CALLBACK(generate_cs_sim5_results_callback), NULL);

    viewer_widget = gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(viewer_widget), "expose_event", G_CALLBACK(cs_sim5_viewer_expose), NULL);
    g_signal_connect(G_OBJECT(viewer_widget), "configure_event", G_CALLBACK(gtkx_configure_surface_callback), &viewer_surface);
    g_signal_connect(G_OBJECT(viewer_widget), "destroy", G_CALLBACK(gtkx_destroy_surface_callback), &viewer_surface);
    gtk_widget_set_events(viewer_widget, GDK_EXPOSURE_MASK);
    gtk_box_pack_start(GTK_BOX(page), viewer_widget, TRUE, TRUE, 0);
    gtk_widget_show(viewer_widget);

    gtk_container_add(GTK_CONTAINER(vbox), page);
    gtk_widget_show(page);

    initialise_cs_sim5_results();
}

/******************************************************************************/
