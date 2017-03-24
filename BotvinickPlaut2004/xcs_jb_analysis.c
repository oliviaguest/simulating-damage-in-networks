
#include "xframe.h"
#include "lib_cairox.h"
#include "xcs_sequences.h"

#define MAX_TRIALS 500

extern void initialise_state(TaskType *task);
extern int categorise_action_sequence(ActionType *sequence);

/******************************************************************************/

static GtkWidget *jb_viewer_widget = NULL;
static cairo_surface_t *jb_viewer_surface = NULL;

static int unit = 1, cycle = 1, trials = 0;
/* The hidden units on cycle 1, given an instruction and a trial: */
static double hidden_units[3][HIDDEN_WIDTH][MAX_TRIALS];
/* The resulting sequence of each trial (given each instruction): */
static int result[3][MAX_TRIALS];

static Boolean jb_viewer_expose(GtkWidget *widget, GdkEventConfigure *event, void *data);

/******************************************************************************/

static void initialise_jb_results()
{
    trials = 0;
}

/******************************************************************************/

static void record_hidden_activity(int instruct)
{
    double *vector = (double *)malloc(HIDDEN_WIDTH * sizeof(double));
    int i;

    network_ask_hidden(xg.net, vector);

    for (i = 0; i < HIDDEN_WIDTH; i++) {
        hidden_units[instruct][i][trials] = vector[i];
    }

    free(vector);
}

static int run_one_simulation(int instruct)
{
    /* Run one simulation and categorise it according to the possible targets */

    double *vector_in = (double *)malloc(IN_WIDTH * sizeof(double));
    double *vector_out = (double *)malloc(OUT_WIDTH * sizeof(double));
    int count = 0;
    ActionType this[100];

    do {
        /* Record the hidden units on specific cycle: */
        if (count == cycle) { record_hidden_activity(instruct); }

        world_set_network_input_vector(vector_in);
        network_tell_input(xg.net, vector_in);
        network_tell_propagate2(xg.net);
        network_ask_output(xg.net, vector_out);
        this[count] = world_get_network_output_action(NULL, vector_out);
        world_perform_action(this[count]);
    } while ((this[count] != ACTION_SAY_DONE) && (++count < 100));

    free(vector_in);
    free(vector_out);

    return(categorise_action_sequence(this));
}

/*----------------------------------------------------------------------------*/

static void generate_jb_results_callback(GtkWidget *mi, void *arg)
{
    int count = (long) arg;
    int tt;

    while ((trials < MAX_TRIALS) && (count-- > 0)) {
        for (tt = 0; tt < 3; tt++) {
            TaskType task = {(BaseTaskType) tt, DAMAGE_NONE, {pars.sugar_closed, FALSE, FALSE, FALSE, FALSE}};

            initialise_state(&task);
            result[tt][trials] = run_one_simulation(tt);
        }
        trials++;
    }
    jb_viewer_expose(NULL, NULL, NULL);
}

/******************************************************************************/

static void set_colour(cairo_t *cr, int i)
{
    switch (i) {
        case SEQ_COFFEE1	 : { cairo_set_source_rgb(cr, 1.0, 0.0, 0.0); break; }
	case SEQ_COFFEE2	 : { cairo_set_source_rgb(cr, 0.7, 0.0, 0.3); break; }
	case SEQ_COFFEE3	 : { cairo_set_source_rgb(cr, 0.4, 0.0, 0.6); break; }
	case SEQ_COFFEE4	 : { cairo_set_source_rgb(cr, 0.1, 0.0, 0.9); break; }
	case SEQ_COFFEE5	 : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
	case SEQ_COFFEE6	 : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }

	case SEQ_COFFEE1_1SIP	 : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
	case SEQ_COFFEE2_1SIP	 : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
	case SEQ_COFFEE3_1SIP	 : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
	case SEQ_COFFEE4_1SIP	 : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
	case SEQ_COFFEE5_1SIP	 : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
	case SEQ_COFFEE6_1SIP	 : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }

	case SEQ_ERROR_COFFEE1	 : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
	case SEQ_ERROR_COFFEE2	 : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
	case SEQ_ERROR_COFFEE3	 : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
	case SEQ_ERROR_COFFEE4	 : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
	case SEQ_ERROR_COFFEE5	 : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
	case SEQ_ERROR_COFFEE6	 : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }

	case SEQ_ERROR_COFFEE_S1 : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
	case SEQ_ERROR_COFFEE_S2 : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
	case SEQ_ERROR_COFFEE_C  : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
        case SEQ_ERROR_COFFEE_D  : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
        case SEQ_ERROR_COFFEE    : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }

        case SEQ_TEA1            : { cairo_set_source_rgb(cr, 0.0, 1.0, 0.0); break; }
        case SEQ_TEA2            : { cairo_set_source_rgb(cr, 0.0, 1.0, 1.0); break; }
        case SEQ_TEA3            : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }

        case SEQ_TEA1_CREAM      : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
        case SEQ_TEA2_CREAM      : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }

        case SEQ_TEA1_1SIP       : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
        case SEQ_TEA2_1SIP       : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
        case SEQ_TEA3_1SIP       : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
        case SEQ_ERROR_TEA1      : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
        case SEQ_ERROR_TEA2      : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
        case SEQ_ERROR_TEA       : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }

        case SEQ_PICKUP_SIP      : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
        case SEQ_SUGAR_PACK      : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
        case SEQ_SUGAR_BOWL      : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
        case SEQ_ERROR           : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }

        default	                 : { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
    }
}

static void draw_unit_activity(cairo_t *cr, int instruct, int width, int height)
{
    int x0, y0, t;
    char buffer[32];
    CairoxTextParameters tp;
    CairoxLineParameters lp;
    PangoLayout *layout;

    x0 = 20;
    y0 = (instruct + 1) * height;

    cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
    cairo_rectangle(cr, x0, y0-(height-20), width-x0, height-24);
    cairo_fill(cr);

    layout = pango_cairo_create_layout(cr);
    pangox_layout_set_font_size(layout, 12);
    g_snprintf(buffer, 32, "Instruction: %s", task_name[instruct]);
    cairox_text_parameters_set(&tp, x0, y0-(height-18), PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0);
    cairox_paint_pango_text(cr, &tp, layout, buffer);

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairox_line_parameters_set(&lp, 1.0, LS_SOLID, FALSE);
    cairox_paint_line(cr, &lp, x0, y0-4, x0, y0-(height-20));
    cairox_paint_line(cr, &lp, x0, y0-4, x0+2*trials, y0-4);

    for (t = 0; t < trials; t++) {
        /* Set colour to reflect sequence type: */
        set_colour(cr, result[instruct][t]);
        cairo_rectangle(cr, x0 + 2 * t - 1.5, y0-1.5-4-((height-20) * hidden_units[instruct][unit-1][t])+1.5, 3, 3);
        cairo_fill(cr);
    }
    g_object_unref(layout);
}

static Boolean jb_viewer_expose(GtkWidget *widget, GdkEventConfigure *event, void *data)
{
    if (!GTK_WIDGET_MAPPED(jb_viewer_widget) || (jb_viewer_surface == NULL)) {
        return(TRUE);
    }
    else {
        int height = jb_viewer_widget->allocation.height;
        int width  = jb_viewer_widget->allocation.width;
        PangoLayout *layout;
        cairo_t *cr;
        int i;

        cr = cairo_create(jb_viewer_surface);
        layout = pango_cairo_create_layout(cr);
        pangox_layout_set_font_size(layout, 12);
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_paint(cr);

        for (i = TASK_NONE; i < TASK_MAX; i++) {
            draw_unit_activity(cr, i, width, height/TASK_MAX);
        }

        /* Now copy the surface to the window: */
        if ((jb_viewer_widget->window != NULL) && ((cr = gdk_cairo_create(jb_viewer_widget->window)) != NULL)) {
            cairo_set_source_surface(cr, jb_viewer_surface, 0, 0);
            cairo_paint(cr);
            cairo_destroy(cr);
        }
        return(FALSE);
    }
}

/******************************************************************************/

static void clear_jb_results_callback(GtkWidget *mi, void *dummy)
{
    initialise_jb_results();
    jb_viewer_expose(NULL, NULL, NULL);
}

/******************************************************************************/

static void callback_spin_unit(GtkWidget *widget, GtkSpinButton *spin)
{
    unit = gtk_spin_button_get_value_as_int(spin);
    jb_viewer_expose(NULL, NULL, NULL);
}

static void callback_spin_cycle(GtkWidget *widget, GtkSpinButton *spin)
{
    cycle = gtk_spin_button_get_value_as_int(spin);
    initialise_jb_results();
    jb_viewer_expose(NULL, NULL, NULL);
}

GtkWidget *create_jb_analysis_page()
{
    GtkWidget *toolbar, *label, *spin, *hbox, *page;
    GtkAdjustment *adj;

    page = gtk_vbox_new(FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_TEXT);
    gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(hbox), toolbar, FALSE, FALSE, 0);
    gtk_widget_show(toolbar);

    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Reset", NULL, NULL, NULL, G_CALLBACK(clear_jb_results_callback), NULL);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "20 Trials", NULL, NULL, NULL, G_CALLBACK(generate_jb_results_callback), (void *)20);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "100 Trials", NULL, NULL, NULL, G_CALLBACK(generate_jb_results_callback), (void *)100);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "500 Trials", NULL, NULL, NULL, G_CALLBACK(generate_jb_results_callback), (void *)500);

    adj = (GtkAdjustment *)gtk_adjustment_new(unit, 1, HIDDEN_WIDTH, 1.0, 1.0, 0);
    spin = gtk_spin_button_new(adj, 1, 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin), TRUE);
    g_signal_connect(G_OBJECT(adj), "value_changed", G_CALLBACK(callback_spin_unit), spin);
    gtk_box_pack_end(GTK_BOX(hbox), spin, FALSE, FALSE, 0);
    gtk_widget_show(spin);

    label = gtk_label_new("Hidden unit: ");
    gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_widget_show(label);

    label = gtk_label_new("  ");
    gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_widget_show(label);

    adj = (GtkAdjustment *)gtk_adjustment_new(cycle, 0, 40, 1.0, 1.0, 0);
    spin = gtk_spin_button_new(adj, 1, 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin), TRUE);
    g_signal_connect(G_OBJECT(adj), "value_changed", G_CALLBACK(callback_spin_cycle), spin);
    gtk_box_pack_end(GTK_BOX(hbox), spin, FALSE, FALSE, 0);
    gtk_widget_show(spin);

    label = gtk_label_new("Cycle: ");
    gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_widget_show(label);

    /* The viewer: */

    jb_viewer_widget = gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(jb_viewer_widget), "expose_event", G_CALLBACK(jb_viewer_expose), NULL);
    g_signal_connect(G_OBJECT(jb_viewer_widget), "configure_event", G_CALLBACK(gtkx_configure_surface_callback), &jb_viewer_surface);
    g_signal_connect(G_OBJECT(jb_viewer_widget), "destroy", G_CALLBACK(gtkx_destroy_surface_callback), &jb_viewer_surface);
    gtk_widget_set_events(jb_viewer_widget, GDK_EXPOSURE_MASK);
    gtk_box_pack_start(GTK_BOX(page), jb_viewer_widget, TRUE, TRUE, 0);
    gtk_widget_show(jb_viewer_widget);

    return(page);
}

/******************************************************************************/
