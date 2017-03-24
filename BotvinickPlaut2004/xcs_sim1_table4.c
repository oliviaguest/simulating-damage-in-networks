
#include "xframe.h"
#include "lib_cairox.h"
#include "xcs_sequences.h"

extern void initialise_state(TaskType *task);

/******************************************************************************/

static GtkWidget *viewer_widget = NULL;
static cairo_surface_t *viewer_surface = NULL;

static int sim1_results[3][NUM_VARIANTS];

#include TARGET_SEQUENCES

/******************************************************************************/

char *variant_name[NUM_VARIANTS] = {
                                "Grounds - Sugar (pack) - Cream - Drink",
                                "Grounds - Sugar (bowl closed) - Cream - Drink",
                                "Grounds - Cream - Sugar (pack) - Drink",
                                "Grounds - Cream - Sugar (bowl closed) - Drink",
                                "Grounds - Sugar (bowl open) - Cream - Drink",
                                "Grounds - Cream - Sugar (bowl open) - Drink",

                                "Grounds - Sugar (pack) - Cream - 1 Sip",
                                "Grounds - Sugar (bowl closed) - Cream - 1 Sip",
                                "Grounds - Cream - Sugar (pack) - 1 Sip",
                                "Grounds - Cream - Sugar (bowl closed) - 1 Sip",
                                "Grounds - Sugar (bowl open) - Cream - 1 Sip",
                                "Grounds - Cream - Sugar (bowl open) - 1 Sip",

                                "Grounds - Sugar (pack) - Cream - Error",
                                "Grounds - Sugar (bowl closed) - Cream - Error",
                                "Grounds - Cream - Sugar (pack) - Error",
                                "Grounds - Cream - Sugar (bowl closed) - Error",
                                "Grounds - Sugar (bowl open) - Cream - Error",
                                "Grounds - Cream - Sugar (bowl open) - Error",

                                "Grounds - Sugar (pack) - Drink",
                                "Grounds - Sugar (bowl closed) - Drink",
                                "Grounds - Cream - Drink",
                                "Grounds - Drink",
                                "Grounds - Error",

                                "Teabag - Sugar (pack) - Drink",
                                "Teabag - Sugar (bowl closed) - Drink",
                                "Teabag - Sugar (bowl open) - Drink",

                                "Teabag - Sugar (pack) - Cream - Drink",
                                "Teabag - Sugar (bowl closed) - Cream - Drink",

                                "Teabag - Sugar (pack) - 1 Sip",
                                "Teabag - Sugar (bowl closed) - 1 Sip",
                                "Teabag - Sugar (bowl open) - 1 Sip",

                                "Teabag - Sugar (pack) - Error",
                                "Teabag - Sugar (bowl closed) - Error",

                                "Teabag - Error",

                                "PickUp - Sip...",
                                "Sugar (pack) - Done",
                                "Sugar (bowl closed) - Done",
                                "Other (error)"}; 


/******************************************************************************/

Boolean sim1_table4_expose(GtkWidget *widget, GdkEvent *event, void *data)
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
        int y = 18, i, j, w;

        cr = cairo_create(viewer_surface);
        layout = pango_cairo_create_layout(cr);
        pangox_layout_set_font_size(layout, 12);

        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_paint(cr);

        y = y + 2;
        cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
        cairox_line_parameters_set(&lp, 1.0, LS_DOTTED, FALSE);
        for (i = 0; i < NUM_VARIANTS; i++) {
            g_snprintf(buffer, 128, "%s:", variant_name[i]); y = y + 15;
            cairox_text_parameters_set(&tp,  10, y, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, buffer);
            if ((i == 22) || (i == 5) || (i == 33) || (i == 25)) {
                cairox_paint_line(cr, &lp, 5, y+2, 545, y+2);
            }
        }
        y = y + 2;
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairox_line_parameters_set(&lp, 1.0, LS_SOLID, FALSE);
        cairox_paint_line(cr, &lp, 5, y, 255, y);
        g_snprintf(buffer, 128, "TOTAL:"); y = y + 15;
        cairox_text_parameters_set(&tp,  10, y, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);

        cairox_line_parameters_set(&lp, 1.0, LS_SOLID, FALSE);
        for (j = 0; j < 3; j++) {
            int count = 0;
            y = 18;
            g_snprintf(buffer, 128, "%s", task_name[j]);

            cairox_text_parameters_set(&tp,  340+100*j, y, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, buffer);
            y = y + 2;
            w = pangox_layout_get_string_width(layout, buffer);
            cairox_paint_line(cr, &lp, 340+100*j-w/2, y, 340+100*j+w/2, y);
            for (i = 0; i < NUM_VARIANTS; i++) {
                g_snprintf(buffer, 128, "%d", sim1_results[j][i]); y = y + 15;
                cairox_text_parameters_set(&tp, 345+100*j, y, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_BOTTOM, 0.0);
                cairox_paint_pango_text(cr, &tp, layout, buffer);
                count += sim1_results[j][i];
            }
            y = y + 2;
            cairox_paint_line(cr, &lp, 320+100*j, y, 347+100*j, y);
            g_snprintf(buffer, 128, "%d", count); y = y + 15;
            cairox_text_parameters_set(&tp,  345+100*j, y, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_BOTTOM, 0.0);
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

static void initialise_sim1_results()
{
    int i, j;
    for (j = 0; j < 3; j++) {
        for (i = 0; i < NUM_VARIANTS; i++) {
            sim1_results[j][i] = 0;
        }
    }
}

static Boolean sequence_compare(ActionType *known, ActionType *observed)
{
    int i = 0;

    while (known[i] == observed[i]) {
        if (known[i] == ACTION_SAY_DONE) {
            return(TRUE);
        }
        else {
            i++;
        }
    }
    return(FALSE);
}

static Boolean subsequence_compare(ActionType *known, ActionType *observed)
{
    int i = 0;

    while ((known[i] != ACTION_SAY_DONE) && (known[i] == observed[i])) {
        i++;
    }
    return(known[i] == ACTION_SAY_DONE);
}

int categorise_action_sequence(ActionType *sequence)
{
    /* Answers:
        0: Ground -> Sugar (pack) -> Cream -> Drink
        1: Ground -> Sugar (bowl) -> Cream -> Drink
        2: Ground -> Cream -> Sugar (pack) -> Drink
        3: Ground -> Cream -> Sugar (bowl) -> Drink
        4: Teabag -> Sugar (pack) -> Drink
        5: Teabag -> Sugar (bowl) -> Drink
        6: Error
    */
   
    if (sequence_compare(sequence_coffee1, sequence)) {
        return(SEQ_COFFEE1);
    }
    else if (sequence_compare(sequence_coffee2, sequence)) {
        return(SEQ_COFFEE2);
    }
    else if (sequence_compare(sequence_coffee3, sequence)) {
        return(SEQ_COFFEE3);
    }
    else if (sequence_compare(sequence_coffee4, sequence)) {
        return(SEQ_COFFEE4);
    }
    else if (sequence_compare(sequence_coffee5, sequence)) {
        return(SEQ_COFFEE5);
    }
    else if (sequence_compare(sequence_coffee6, sequence)) {
        return(SEQ_COFFEE6);
    }
    else if (subsequence_compare(sequence_coffee1_1sip, sequence)) {
        return(SEQ_COFFEE1_1SIP);
    }
    else if (subsequence_compare(sequence_coffee2_1sip, sequence)) {
        return(SEQ_COFFEE2_1SIP);
    }
    else if (subsequence_compare(sequence_coffee3_1sip, sequence)) {
        return(SEQ_COFFEE3_1SIP);
    }
    else if (subsequence_compare(sequence_coffee4_1sip, sequence)) {
        return(SEQ_COFFEE4_1SIP);
    }
    else if (subsequence_compare(sequence_coffee5_1sip, sequence)) {
        return(SEQ_COFFEE5_1SIP);
    }
    else if (subsequence_compare(sequence_coffee6_1sip, sequence)) {
        return(SEQ_COFFEE6_1SIP);
    }
    else if (subsequence_compare(sequence_coffee1_error, sequence)) {
        return(SEQ_ERROR_COFFEE1);
    }
    else if (subsequence_compare(sequence_coffee2_error, sequence)) {
        return(SEQ_ERROR_COFFEE2);
    }
    else if (subsequence_compare(sequence_coffee3_error, sequence)) {
        return(SEQ_ERROR_COFFEE3);
    }
    else if (subsequence_compare(sequence_coffee4_error, sequence)) {
        return(SEQ_ERROR_COFFEE4);
    }
    else if (subsequence_compare(sequence_coffee5_error, sequence)) {
        return(SEQ_ERROR_COFFEE5);
    }
    else if (subsequence_compare(sequence_coffee6_error, sequence)) {
        return(SEQ_ERROR_COFFEE6);
    }
    else if (sequence_compare(sequence_coffee_s1_drink, sequence)) {
        return(SEQ_ERROR_COFFEE_S1);
    }
    else if (sequence_compare(sequence_coffee_s2_drink, sequence)) {
        return(SEQ_ERROR_COFFEE_S2);
    }
    else if (sequence_compare(sequence_coffee_c_drink, sequence)) {
        return(SEQ_ERROR_COFFEE_C);
    }
    else if (sequence_compare(sequence_coffee_drink, sequence)) {
        return(SEQ_ERROR_COFFEE_D);
    }
    else if (subsequence_compare(sequence_coffee_start, sequence)) {
        return(SEQ_ERROR_COFFEE);
    }
    else if (sequence_compare(sequence_tea1, sequence)) {
        return(SEQ_TEA1);
    }
    else if (sequence_compare(sequence_tea2, sequence)) {
        return(SEQ_TEA2);
    }
    else if (sequence_compare(sequence_tea3, sequence)) {
        return(SEQ_TEA3);
    }
    else if (subsequence_compare(sequence_tea1_1sip, sequence)) {
        return(SEQ_TEA1_1SIP);
    }
    else if (subsequence_compare(sequence_tea2_1sip, sequence)) {
        return(SEQ_TEA2_1SIP);
    }
    else if (subsequence_compare(sequence_tea3_1sip, sequence)) {
        return(SEQ_TEA3_1SIP);
    }
    else if (subsequence_compare(sequence_tea1_with_cream, sequence)) {
        return(SEQ_TEA1_CREAM);
    }
    else if (subsequence_compare(sequence_tea2_with_cream, sequence)) {
        return(SEQ_TEA2_CREAM);
    }
    else if (subsequence_compare(sequence_tea1_error, sequence)) {
        return(SEQ_ERROR_TEA1);
    }
    else if (subsequence_compare(sequence_tea2_error, sequence)) {
        return(SEQ_ERROR_TEA2);
    }
    else if (subsequence_compare(sequence_tea_start, sequence)) {
        return(SEQ_ERROR_TEA);
    }
    else if (subsequence_compare(sequence_pickup_sip, sequence)) {
        return(SEQ_PICKUP_SIP);
    }
    else if (sequence_compare(sequence_sugar_pack, sequence)) {
        return(SEQ_SUGAR_PACK);
    }
    else if (sequence_compare(sequence_sugar_bowl, sequence)) {
        return(SEQ_SUGAR_BOWL);
    }
    else {
        return(SEQ_ERROR);
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

static void generate_sim1_results_callback(GtkWidget *mi, void *count)
{
    int tt, i, s;

    for (i = 0; i < (long) count; i++) {
        for (tt = 0; tt < 3; tt++) {
            TaskType task = {(BaseTaskType) tt, DAMAGE_NONE, {pars.sugar_closed, FALSE, FALSE, FALSE, FALSE}};

            initialise_state(&task);
            s = run_one_simulation();
            sim1_results[tt][s]++;
            sim1_table4_expose(viewer_widget, NULL, NULL);
        }
    }
}

static void clear_sim1_results_callback(GtkWidget *mi, void *dummy)
{
    initialise_sim1_results();
    sim1_table4_expose(viewer_widget, NULL, NULL);
}

void create_sim1_viewer(GtkWidget *vbox)
{
    GtkWidget *page = gtk_vbox_new(FALSE, 0);
    GtkWidget *toolbar, *scroll_win;

    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_TEXT);
    gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(page), toolbar, FALSE, FALSE, 0);
    gtk_widget_show(toolbar);

    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Reset", NULL, NULL, NULL, G_CALLBACK(clear_sim1_results_callback), NULL);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "10 Trials", NULL, NULL, NULL, G_CALLBACK(generate_sim1_results_callback), (void *)10);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "50 Trials", NULL, NULL, NULL, G_CALLBACK(generate_sim1_results_callback), (void *)50);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "100 Trials", NULL, NULL, NULL, G_CALLBACK(generate_sim1_results_callback), (void *)100);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "500 Trials", NULL, NULL, NULL, G_CALLBACK(generate_sim1_results_callback), (void *)500);

    viewer_widget = gtk_drawing_area_new();
    gtk_widget_set_size_request(viewer_widget, -1, 24 + 15 * (NUM_VARIANTS + 1));
    g_signal_connect(G_OBJECT(viewer_widget), "expose_event", G_CALLBACK(sim1_table4_expose), NULL);
    g_signal_connect(G_OBJECT(viewer_widget), "configure_event", G_CALLBACK(gtkx_configure_surface_callback), &viewer_surface);
    g_signal_connect(G_OBJECT(viewer_widget), "destroy", G_CALLBACK(gtkx_destroy_surface_callback), &viewer_surface);
    gtk_widget_set_events(viewer_widget, GDK_EXPOSURE_MASK);

    scroll_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll_win), viewer_widget);
    gtk_widget_show(scroll_win);

    gtk_box_pack_start(GTK_BOX(page), scroll_win, TRUE, TRUE, 0);
    gtk_widget_show(viewer_widget);

    gtk_container_add(GTK_CONTAINER(vbox), page);
    gtk_widget_show(page);

    initialise_sim1_results();
}

/******************************************************************************/
