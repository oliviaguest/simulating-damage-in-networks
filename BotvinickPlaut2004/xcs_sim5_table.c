
#include "xframe.h"
#include "lib_cairox_2_0.h"
#include "xcs_sequences.h"

extern void initialise_state(TaskType *task);
extern int categorise_action_sequence(ActionType *sequence);
extern void xgraph_set_error_scores();
extern void initialise_widgets();
extern int reload_training_data(char *file);

/******************************************************************************/

static GtkWidget *viewer_widget = NULL;
static cairo_surface_t *viewer_surface = NULL;

/* The results matrix has 3 dimensions: simulation (5a/5b), trial, criterion (1 - 6) */
/* Values in the results matrix are epochs to criterion. */
static int cs_sim5_results[2][5][6];

#define CRITERION 100

/******************************************************************************/

static double mean_result(int j, int i)
{
    double sum = 0;
    int l = 0;

    for (l = 0; l < 5; l++) {
        sum = sum + cs_sim5_results[j][l][i];
    }
    return(sum / (double) l);
}

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

static void repaint_table(cairo_t *cr, PangoLayout *layout, int s)
{
    char buffer[128];
    CairoxLineParameters lp;
    CairoxTextParameters tp;
    int x0, y0, dx, dy, i, l;

    x0 = 40;
    y0 = 30 + s * 250;
    dx = 0; dy = 0;

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairox_line_parameters_set(&lp, 1.0, LS_SOLID, FALSE);
    for (l = 0; l < 8; l++) {
        cairox_paint_line(cr, &lp, x0, y0 + dy, x0 + 7*80, y0 + dy);
        dy = dy + 25;
    }

    for (i = 0; i < 8; i++) {
        cairox_paint_line(cr, &lp, x0 + dx, y0, x0 + dx, y0 + 7*25);
        dx = dx + 80;
    }

    dy = 20;
    for (i = 0; i < 6; i++) {
        g_snprintf(buffer, 128, "Criterion %d", i);
        cairox_text_parameters_set(&tp, x0+120+i*80, y0+dy, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);
    }

    for (l = 0; l < 5; l++) {
        dy = dy + 25;
        g_snprintf(buffer, 128, "Trial %d", l+1);
        cairox_text_parameters_set(&tp, x0+15, y0+dy, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);
        for (i = 0; i < 6; i++) {
            g_snprintf(buffer, 128, "%d", cs_sim5_results[s][l][i]);
            cairox_text_parameters_set(&tp, x0+135+i*80, y0+dy, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_BOTTOM, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, buffer);
        }
    }

    /* The row of column means: */
    dy = dy + 25;
    g_snprintf(buffer, 128, "Mean");
    cairox_text_parameters_set(&tp, x0+15, y0+dy, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
    cairox_paint_pango_text(cr, &tp, layout, buffer);
    for (i = 0; i < 6; i++) {
        g_snprintf(buffer, 128, "%5.1f", mean_result(s, i));
        cairox_text_parameters_set(&tp, x0+135+i*80, y0+dy, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);
    }

    /* The table caption: */
    g_snprintf(buffer, 128, "Epochs to successive criteria (simulation 5%s)", (s == 0 ? "a" : "b"));
    cairox_text_parameters_set(&tp, 310, y0+200, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
    cairox_paint_pango_text(cr, &tp, layout, buffer);
}

/*----------------------------------------------------------------------------*/

static Boolean cs_sim5_viewer_expose(GtkWidget *widget, GdkEvent *event, void *data)
{
    if (!GTK_WIDGET_MAPPED(viewer_widget) || (viewer_surface == NULL)) {
        return(TRUE);
    }
    else {
        PangoLayout *layout;
        cairo_t *cr;
        CairoxTextParameters tp;
        char buffer[128];

        cr = cairo_create(viewer_surface);
        layout = pango_cairo_create_layout(cr);
        pangox_layout_set_font_size(layout, 12);

        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_paint(cr);

        repaint_table(cr, layout, 0);
        repaint_table(cr, layout, 1);

        /* The current criterion (for information): */
        g_snprintf(buffer, 128, "Current criterion: %d correct trials", CRITERION);
        cairox_text_parameters_set(&tp, 310, 520, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
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

static void initialise_cs_sim5_results()
{
    int j, k, l;

    for (j = 0; j < 2; j++) {
        for (k = 0; k < 5; k++) {
            for (l = 0; l < 6; l++) {
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

static Boolean test_criterion(int c, int s)
{
    int i, j;

    /* Iterate N times and succeed only if each and every iteration succeeds: */
    for (i = 0; i < CRITERION; i++) {
        TaskType task = {(BaseTaskType) s+1, DAMAGE_NONE, {TRUE, FALSE, FALSE, FALSE, FALSE}};

        initialise_state(&task);
        j = run_one_simulation();

        if (s == 0) {
            /* Coffee criterion: */
            if ((j != SEQ_COFFEE1) && (j != SEQ_COFFEE2) && (j != SEQ_COFFEE3) && (j != SEQ_COFFEE4)) {
                return(FALSE);
            }
	}
        else {
            /* Tea criterion: */
            if ((j != SEQ_TEA1) && (j != SEQ_TEA2)) {
                return(FALSE);
            }
        }
    }
    return(TRUE);
}

static int train_to_criterion(int cycle, int task)
{
    while ((cycle < 50000) && (!test_criterion(cycle, task))) {
        network_train(xg.net, xg.first, pars.lr, pars.ef, pars.wut, pars.penalty);
        gtkx_flush_events();
        cycle++;
    }
    return(cycle);
}

static void generate_cs_sim5_results(int simulation, int trial)
{
    int n, cycle, task;

    cycle = 0;
    network_tell_initialise(xg.net);
    task = simulation;

    for (n = 0; n < 6; n++) {
        load_training_set(task);
        cycle = train_to_criterion(cycle, task);
        cs_sim5_results[simulation][trial][n] = cycle;
        task = 1 - task;
        cs_sim5_viewer_expose(NULL, NULL, NULL);
        gtkx_flush_events();
    }
}

static void generate_cs_sim5_results_callback(GtkWidget *mi, void *dummy)
{
    /* Here we iterate over five trials for sim 4a and sim 4b :*/

    int trial;

    for (trial = 0; trial < 5; trial++) {
        generate_cs_sim5_results(1, trial);
        generate_cs_sim5_results(0, trial);
    }
}

static void clear_cs_sim5_results_callback(GtkWidget *mi, void *dummy)
{
    initialise_cs_sim5_results();
    cs_sim5_viewer_expose(NULL, NULL, NULL);
}

void create_cs_sim5_table(GtkWidget *vbox)
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


