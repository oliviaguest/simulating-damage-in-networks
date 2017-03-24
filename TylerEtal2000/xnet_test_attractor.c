#include "xframe.h"

#include "lib_maths.h"
#include "lib_cairox.h"

// Number of types of damaged available from the damage menu:
#define DT_MAX 9

// Maximum number of test patterns ... should really be 2^IO_WIDTH
// which is 16 x 1024 x 1024, or 16777216
#define TEST_MAX 1024000
// Setting to 16777216 will probably exceed available memory ...
// Perhaps we should record to avoid this as it could be avoided

static char *dt_name[DT_MAX] = {
    "None",
    "20% Lesion",
    "1.0 Noise",
    "40% Lesion",
    "2.0 Noise",
    "60% Lesion",
    "3.0 Noise",
    "80% Lesion",
    "4.0 Noise"
};

static int damage_type = 0;

static GtkWidget *net_viewer_widget = NULL;
static cairo_surface_t *net_viewer_surface = NULL;

static Boolean aborted = FALSE;
static Network *reference_net = NULL;

typedef struct attractor_data {
    double vector_in[IO_WIDTH];
    double vector_hidden[HIDDEN_WIDTH];
    double vector_out[IO_WIDTH];
    int settling_time;
} AttractorData;

static AttractorData results[TEST_MAX];
static int result_count = 0;
static int result_num_inputs = 0;
static int result_num_attractors = 0;
static int result_num_outputs = 0;

/* Keep track of buttons so we can maintain sensitivity: */
static GtkWidget *button_tk = NULL;
static GtkWidget *button_ta = NULL;
static GtkWidget *button_ab = NULL;

/******************************************************************************/
/* Keep track of the results: ------------------------------------------------*/

static void vector_from_integer_id(double *vector, int index)
{
    int i;

    for (i = 0; i < IO_WIDTH; i++) {
        vector[i] = index % 2;
        index = index / 2;
    }
}

static Boolean novel_input(int result_count)
{
    int i = 0;
    Boolean found = FALSE;

    while ((!found) && (i < result_count-1)) {
        // Assume that two vectors are equal if their differenceis less than
        // some threshold.
        found = (vector_sum_square_difference(IO_WIDTH, results[result_count-1].vector_in, results[i].vector_in) < NET_SETTLING_THRESHOLD);
        i++;
    }
    return(!found);
}

static Boolean novel_hidden(int result_count)
{
    int i = 0;
    Boolean found = FALSE;

    while ((!found) && (i < result_count-1)) {
        // Assume that two vectors are equal if their differenceis less than
        // some threshold.
        found = (vector_sum_square_difference(HIDDEN_WIDTH, results[result_count-1].vector_hidden, results[i].vector_hidden) < NET_SETTLING_THRESHOLD);
        i++;
    }
    return(!found);
}

static Boolean novel_output(int result_count)
{
    int i = 0;
    Boolean found = FALSE;

    while ((!found) && (i < result_count-1)) {
        // Assume that two vectors are equal if their differenceis less than
        // some threshold.
        found = (vector_sum_square_difference(IO_WIDTH, results[result_count-1].vector_out, results[i].vector_out) < NET_SETTLING_THRESHOLD);
        i++;
    }
    return(!found);
}

static void result_record_input(int result_count, double *vector_in)
{
    int i;

    if (result_count <= TEST_MAX) {
        for (i = 0; i < IO_WIDTH; i++) {
            results[result_count-1].vector_in[i] = vector_in[i];
        }
        for (i = 0; i < HIDDEN_WIDTH; i++) {
            results[result_count-1].vector_hidden[i] = 0.0;
        }
        for (i = 0; i < IO_WIDTH; i++) {
            results[result_count-1].vector_out[i] = 0.0;
        }
        results[result_count-1].settling_time = 0;

        if (novel_input(result_count)) {
            result_num_inputs++;
        }
    }
}

static void result_record_intermediate_state(int result_count, Network *net)
{
    if (result_count <= TEST_MAX) {
        network_ask_hidden(net, results[result_count-1].vector_hidden);
        results[result_count-1].settling_time = net->cycles;
        network_ask_output(net, results[result_count-1].vector_out);
    }
}

static void result_record_final_state(int result_count, Network *net)
{
    if (result_count <= TEST_MAX) {
        if (novel_hidden(result_count)) {
            result_num_attractors++;
        }
        else {
            fprintf(stdout, "Attractor %d is a duplicate\n", result_count);
        }
        if (novel_output(result_count)) {
            result_num_outputs++;
        }
        else {
            fprintf(stdout, "Output %d is a duplicate\n", result_count);
        }
    }
}

/*----------------------------------------------------------------------------*/
/* Draw the results: ---------------------------------------------------------*/

static void cr_draw_vector(cairo_t *cr, double *vector, int width, int x, int y, Boolean colour)
{
    int unit_size = 12;
    int i;

    /* x and y are bottom left of the vector: */

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width(cr, 1.0);

    for (i = 0; i < width; i++) {
        double x0 = x + i * unit_size;
        double y0 = y - unit_size;

        if (colour) {
            /* The bar's colour: */
            cairox_select_colour_scale(cr, 1.0 - vector[i]);
        }
        else {
            /* The bar's greyscale: */
            cairo_set_source_rgb(cr, (1.0 - vector[i]), (1.0 - vector[i]), (1.0 - vector[i]));
        }

        /* The colour patch: */
        cairo_rectangle(cr, x0, y0, unit_size, unit_size);
        cairo_fill_preserve(cr);
        /* The outline: */
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_stroke(cr);
    }
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
}

static void attractor_write_results(cairo_t *cr, PangoLayout *layout, int x, int y, int i, XGlobals *xg)
{
    CairoxTextParameters tp;
    PatternList *p = NULL;
    char buffer[16];

    // Input vector:
    cr_draw_vector(cr, results[i].vector_in, IO_WIDTH, x, y, xg->colour);
    // Settling time:
    if (xg->net->nt == NT_RECURRENT) {
        g_snprintf(buffer, 16, "%5d", results[i].settling_time);
        cairox_text_parameters_set(&tp, x+320, y, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);
    }
    // Hidden vector:
    cr_draw_vector(cr, results[i].vector_hidden, HIDDEN_WIDTH, x+350, y, xg->colour);
    // Output vector:
    cr_draw_vector(cr, results[i].vector_out, IO_WIDTH, x+600, y, xg->colour);

    // Find the best match to the output and write it's id:

    if ((p = world_decode_vector(xg->training_set, results[i].vector_out)) != NULL) {
        g_snprintf(buffer, 16, "(%2s)", p->name);
        cairox_text_parameters_set(&tp, x+900, y, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);
    }
}

static double mean_settling_time(int i_min)
{
    int sum = 0;
    int i;

    for (i = i_min; i < result_count; i++) {
        sum += results[i].settling_time;
    }
    return(sum / (double) (result_count - i_min));
}

static Boolean attractor_viewer_repaint(GtkWidget *widget, GdkEvent *event, XGlobals *xg)
{
    if (!GTK_WIDGET_MAPPED(net_viewer_widget) || (net_viewer_surface == NULL)) {
        return(TRUE);
    }
    else {
        int x, y, i, i_min;
        PangoLayout *layout;
        cairo_t *cr;
        CairoxTextParameters tp;
        char buffer[128];

        cr = cairo_create(net_viewer_surface);
        layout = pango_cairo_create_layout(cr);
        pangox_layout_set_font_size(layout, 12);

        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_paint(cr);

        x = 5; y = 20;

        cairox_text_parameters_set(&tp, x, y, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, "Input");
        cairox_text_parameters_set(&tp, x+300, y, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, "Cycles");
        cairox_text_parameters_set(&tp, x+350, y, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, "Hidden (Attractor State)");
        cairox_text_parameters_set(&tp, x+600, y, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, "Output");
        y = y + 18;

        /* Display the last 25 results... */
        if (result_count > 25) {
            i_min = result_count - 25;
        }
        else {
            i_min = 0;
        }

        for (i = i_min; i < result_count; i++) {
            attractor_write_results(cr, layout, x, y, i, xg);
            y = y + 18;
        }

        g_snprintf(buffer, 128, "Distinct Inputs: %d", result_num_inputs);
        cairox_text_parameters_set(&tp, x, y, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);
        if (result_count - i_min > 0) {
            g_snprintf(buffer, 128, "%6.2f", mean_settling_time(i_min));
            cairox_text_parameters_set(&tp, x+330, y, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_BOTTOM, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, buffer);
	}
        g_snprintf(buffer, 128, "Distinct Attractors: %d", result_num_attractors);
        cairox_text_parameters_set(&tp, x+350, y, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);
        g_snprintf(buffer, 128, "Distinct Outputs: %d", result_num_outputs);
        cairox_text_parameters_set(&tp, x+600, y, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);

        g_object_unref(layout);
        cairo_destroy(cr);

        /* Now copy the surface to the window: */
        if ((net_viewer_widget->window != NULL) && ((cr = gdk_cairo_create(net_viewer_widget->window)) != NULL)) {
            cairo_set_source_surface(cr, net_viewer_surface, 0, 0);
            cairo_paint(cr);
            cairo_destroy(cr);
        }
    }
    return(FALSE);
}

/******************************************************************************/

static void dump_attractor_distance_matrix(char *filename)
{
  /* Write a matrix of distance between attractors */

  int i, j;
  FILE *fp = NULL;

  if (filename != NULL) {
    fp = fopen(filename, "w");
  }

  if (fp == NULL) {
    fp = stderr;
  }

  for (i = 0; i < result_count; i++) {
    for (j = 0; j < result_count; j++) {
      fprintf(fp, "\t%6.3f", sqrt(vector_sum_square_difference(HIDDEN_WIDTH, results[i].vector_hidden, results[j].vector_hidden)));
    }
    fprintf(fp, "\n");
  }

  if (fp != stderr) {
    fclose(fp);
  }

}


static void calculate_attractor_similarity(FILE *fp)
{
  double sim[16][16];
  double d11, d12, d13, d14, d22, d23, d24, d33, d34, d44;
  int i, j;

  if ((result_count == 16) && (fp != NULL)) {
    for (i = 0; i < 16; i++) {
      for (j = 0; j < 16; j++) {
        sim[i][j] = sqrt(vector_sum_square_difference(HIDDEN_WIDTH, results[i].vector_hidden, results[j].vector_hidden));
      }
    }

    d11 = (sim[0][1]+sim[0][2]+sim[0][3]+sim[1][2]+sim[1][3]+sim[2][3])/6.0;
    d22 = (sim[4][5]+sim[4][6]+sim[4][7]+sim[5][6]+sim[5][7]+sim[6][7])/6.0;
    d33 = (sim[8][9]+sim[8][10]+sim[8][11]+sim[9][10]+sim[9][11]+sim[10][11])/6.0;
    d44 = (sim[12][13]+sim[12][14]+sim[12][15]+sim[13][14]+sim[13][15]+sim[14][15])/6.0;

    d12 = 0;
    for (i = 0; i < 4; i++) {
      for (j = 4; j < 8; j++) {
        d12 += sim[i][j];
      }
    }
    d12 = d12 / 16.0;

    d13 = 0;
    for (i = 0; i < 4; i++) {
      for (j = 8; j < 12; j++) {
        d13 += sim[i][j];
      }
    }
    d13 = d13 / 16.0;

    d23 = 0;
    for (i = 4; i < 8; i++) {
      for (j = 8; j < 12; j++) {
        d23 += sim[i][j];
      }
    }
    d23 = d23 / 16.0;

    d14 = 0;
    for (i = 0; i < 4; i++) {
      for (j = 12; j < 16; j++) {
        d14 += sim[i][j];
      }
    }
    d14 = d14 / 16.0;

    d24 = 0;
    for (i = 4; i < 8; i++) {
      for (j = 12; j < 16; j++) {
        d24 += sim[i][j];
      }
    }
    d24 = d24 / 16.0;

    d34 = 0;
    for (i = 8; i < 12; i++) {
      for (j = 12; j < 16; j++) {
        d34 += sim[i][j];
      }
    }
    d34 = d34 / 16.0;


    fprintf(fp, "Mean within category distance: %7.4f %7.4f %7.4f %7.4f\n", d11, d22, d33, d44);

    fprintf(fp, "Mean cross category distance: %7.4f %7.4f\n", d12, d34);

    fprintf(fp, "Mean cross domain distance: %7.4f %7.4f %7.4f %7.4f\n", d13, d14, d23, d24);
  }
}


/*----------------------------------------------------------------------------*/

static void callback_test_known(GtkWidget *button, XGlobals *xg)
{
    double *vector_in = (double *)malloc(IO_WIDTH * sizeof(double));
    aborted = FALSE;
    xg->pattern_num = 0;

    if (button_ab != NULL) { gtk_widget_set_sensitive(button_ab, TRUE); }
    if (button_ta != NULL) { gtk_widget_set_sensitive(button_ta, FALSE); }
    if (button_tk != NULL) { gtk_widget_set_sensitive(button_tk, FALSE); }

    result_count = 0;
    result_num_inputs = 0;
    result_num_attractors = 0;
    result_num_outputs = 0;

    while ((!aborted) && (xg->pattern_num < pattern_list_length(xg->training_set))) {
        xg->pattern_num++;
        result_count++;

        world_set_network_input_vector(vector_in, xg->training_set, xg->pattern_num);
        network_tell_input(xg->net, vector_in);
        result_record_input(result_count, vector_in);
        network_tell_randomise_hidden(xg->net);
        if (xg->net->nt == NT_RECURRENT) {
            do {
                network_tell_propagate(xg->net);
                result_record_intermediate_state(result_count, xg->net);
                attractor_viewer_repaint(NULL, NULL, xg);
                gtkx_flush_events();
// Insert a delay if we want to see things settle:
//                 g_usleep(200000);
            } while ((!xg->net->settled) && (xg->net->cycles < 100));
        }
        else {
            network_tell_propagate(xg->net);
            result_record_intermediate_state(result_count, xg->net);
        }
        result_record_final_state(result_count, xg->net);
        attractor_viewer_repaint(NULL, NULL, xg);
    }

    dump_attractor_distance_matrix("Attractor matrix.tab");
    calculate_attractor_similarity(stdout);

    free(vector_in);
    if (button_ab != NULL) { gtk_widget_set_sensitive(button_ab, FALSE); }
    if (button_ta != NULL) { gtk_widget_set_sensitive(button_ta, TRUE); }
    if (button_tk != NULL) { gtk_widget_set_sensitive(button_tk, TRUE); }
}

static void callback_test_all(GtkWidget *button, XGlobals *xg)
{
    double *vector_in = (double *)malloc(IO_WIDTH * sizeof(double));

    aborted = FALSE;

    if (button_ab != NULL) { gtk_widget_set_sensitive(button_ab, TRUE); }
    if (button_ta != NULL) { gtk_widget_set_sensitive(button_ta, FALSE); }
    if (button_tk != NULL) { gtk_widget_set_sensitive(button_tk, FALSE); }

    result_count = 0;
    result_num_inputs = 0;
    result_num_attractors = 0;
    result_num_outputs = 0;

    while ((!aborted) && (result_count < TEST_MAX)) {
        result_count++;

        vector_from_integer_id(vector_in, result_count);
        network_tell_input(xg->net, vector_in);
        result_record_input(result_count, vector_in);
        network_tell_randomise_hidden(xg->net);
        if (xg->net->nt == NT_RECURRENT) {
            do {
                network_tell_propagate(xg->net);
                result_record_intermediate_state(result_count, xg->net);
                attractor_viewer_repaint(NULL, NULL, xg);
                gtkx_flush_events();
// Insert a delay if we want to see things settle:
//                 g_usleep(200000);
            } while ((!xg->net->settled) && (xg->net->cycles < 100));
        }
        else {
            network_tell_propagate(xg->net);
            result_record_intermediate_state(result_count, xg->net);
        }
        result_record_final_state(result_count, xg->net);
        attractor_viewer_repaint(NULL, NULL, xg);
    }
    free(vector_in);

    if (button_ab != NULL) { gtk_widget_set_sensitive(button_ab, FALSE); }
    if (button_ta != NULL) { gtk_widget_set_sensitive(button_ta, TRUE); }
    if (button_tk != NULL) { gtk_widget_set_sensitive(button_tk, TRUE); }
}

static void callback_test_abort(GtkWidget *button, XGlobals *xg)
{
    aborted = TRUE;
    if (button_ab != NULL) { gtk_widget_set_sensitive(button_ab, FALSE); }
    if (button_ta != NULL) { gtk_widget_set_sensitive(button_ta, TRUE); }
    if (button_tk != NULL) { gtk_widget_set_sensitive(button_tk, TRUE); }
}

/*----------------------------------------------------------------------------*/

static void callback_damage_set(GtkWidget *combo_box, XGlobals *xg)
{
    int dt = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_box));

    /* Keep a copy of the undamaged net: */
    if (damage_type == 0) {
        if (reference_net != NULL) {
            network_destroy(reference_net);
        }
        reference_net = network_copy(xg->net);
    }

    if (damage_type != dt) {
        /* Damage type has changed ... Restore the good network (unless we already have it) */
        if (damage_type != 0) {
            network_destroy(xg->net);
            xg->net = network_copy(reference_net);
        }
        /* Inflict the required damage: */
        switch (dt) {
            case 1: { //     20% Lesion
                network_lesion_weights(xg->net, 0.2);
                break;
            }
            case 3: { //     40% Lesion
                network_lesion_weights(xg->net, 0.4);
                break;
            }
            case 5: { //     60% Lesion
                network_lesion_weights(xg->net, 0.6);
                break;
            }
            case 7: { //     80% Lesion
                network_lesion_weights(xg->net, 0.8);
                break;
            }
            case 2: { //     1.0 Noise
                network_perturb_weights(xg->net, 1.0);
                break;
            }
            case 4: { //     2.0 Noise
                network_perturb_weights(xg->net, 2.0);
                break;
            }
            case 6: { //     3.0 Noise
                network_perturb_weights(xg->net, 3.0);
                break;
            }
            case 8: { //     4.0 Noise
                network_perturb_weights(xg->net, 4.0);
                break;
            }
            default: {
                /* Nothing to do - no damage */
            }
        }

        /* And update our record of the damage type: */
        damage_type = dt;
    }
}

static void callback_destroy_reference_network(GtkWidget *widget, void *dummy)
{
    if (reference_net != NULL) {
        network_destroy(reference_net);
        reference_net = NULL;
    }
}

/*----------------------------------------------------------------------------*/

static void callback_print_attractors(GtkWidget *button, XGlobals *xg)
{
// TO DO
fprintf(stdout, "Can't save to PDF yet\n");
}

static void callback_toggle_colour(GtkWidget *button, XGlobals *xg)
{
    xg->colour = (Boolean) (GTK_TOGGLE_BUTTON(button)->active);
    attractor_viewer_repaint(NULL, NULL, xg);
}

/******************************************************************************/

void test_attractor_create_widgets(GtkWidget *vbox, XGlobals *xg)
{
    /* The buttons for testing attractors: */

    GtkWidget *tmp, *hbox, *toolbar;
    int dt;

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    /*--- Run the attractor test: ---*/
    button_tk = gtk_button_new_with_label(" Test Known ");
    g_signal_connect(G_OBJECT(button_tk), "clicked", GTK_SIGNAL_FUNC(callback_test_known), xg);
    gtk_box_pack_start(GTK_BOX(hbox), button_tk, FALSE, FALSE, 5);
    gtk_widget_set_sensitive(button_tk, TRUE);
    gtk_widget_show(button_tk);

    button_ta = gtk_button_new_with_label(" Test All ");
    g_signal_connect(G_OBJECT(button_ta), "clicked", GTK_SIGNAL_FUNC(callback_test_all), xg);
    gtk_box_pack_start(GTK_BOX(hbox), button_ta, FALSE, FALSE, 5);
    gtk_widget_set_sensitive(button_ta, TRUE);
    gtk_widget_show(button_ta);

    button_ab = gtk_button_new_with_label(" Abort ");
    g_signal_connect(G_OBJECT(button_ab), "clicked", GTK_SIGNAL_FUNC(callback_test_abort), xg);
    gtk_box_pack_start(GTK_BOX(hbox), button_ab, FALSE, FALSE, 5);
    gtk_widget_set_sensitive(button_ab, FALSE);
    gtk_widget_show(button_ab);

    /*--- Filler: ---*/
    tmp = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 5);
    gtk_widget_show(tmp);

    /*--- Damage the network: ---*/
    /*--- The label: ---*/
    tmp = gtk_label_new("Damage: ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
    /*--- The menu of damage options: ---*/
    tmp = gtk_combo_box_new_text();
    for (dt = 0; dt < DT_MAX; dt++) {
        gtk_combo_box_insert_text(GTK_COMBO_BOX(tmp), dt, dt_name[dt]);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), damage_type);
    g_signal_connect(G_OBJECT(tmp), "changed", G_CALLBACK(callback_damage_set), xg);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);

    /*--- Filler: ---*/
    tmp = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 5);
    gtk_widget_show(tmp);

    /* Widgets for setting colour/greyscale: */
    /*--- The label: ---*/
    tmp = gtk_label_new("  Colour: ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
    /*--- The checkbox: ---*/
    tmp = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), xg->colour);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(tmp), "toggled", GTK_SIGNAL_FUNC(callback_toggle_colour), xg);
    gtk_widget_show(tmp);

    /*--- Filler: ---*/
    tmp = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 5);
    gtk_widget_show(tmp);

    /*--- Toolbar with print button: ---*/
    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
    gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(hbox), toolbar, TRUE, TRUE, 0);
    gtk_widget_show(toolbar);

    gtkx_toolbar_insert_space(GTK_TOOLBAR(toolbar), TRUE, 0);
    gtkx_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_PRINT, NULL, NULL, G_CALLBACK(callback_print_attractors), xg, 1);

    /* Horizontal separator and the rest of the page: */

    tmp = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);

    net_viewer_widget = gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(net_viewer_widget), "expose_event", G_CALLBACK(attractor_viewer_repaint), xg);
    g_signal_connect(G_OBJECT(net_viewer_widget), "configure_event", G_CALLBACK(gtkx_configure_surface_callback), &net_viewer_surface);
    g_signal_connect(G_OBJECT(net_viewer_widget), "destroy", G_CALLBACK(gtkx_destroy_surface_callback), &net_viewer_surface);
    g_signal_connect(G_OBJECT(net_viewer_widget), "destroy", G_CALLBACK(callback_destroy_reference_network), NULL);
    gtk_widget_set_events(net_viewer_widget, GDK_EXPOSURE_MASK);
    gtk_box_pack_start(GTK_BOX(vbox), net_viewer_widget, TRUE, TRUE, 0);
    gtk_widget_show(net_viewer_widget);
}

/******************************************************************************/

