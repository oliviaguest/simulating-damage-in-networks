/*******************************************************************************

    File:       x_patterns.c
    Contents:   Routines to perform and display a range of pattern-set analyses
    Created:    08/11/16
    Author:     Rick Cooper
    Copyright (c) 2016 Richard P. Cooper

    Public procedures:
        void GtkWidget *tyler_patterns_create_widgets(GtkWidget *page, XGlobals *xg)

TO DO:
* Tidy up (e.g., Category names should not be local to this file)
* Get rid of NUM_PATTERNS symbol

*******************************************************************************/
/******** Include files: ******************************************************/

#include "xframe.h"
#include "lib_maths.h"
#include "lib_string.h"
#include "lib_cairox.h"
#include "lib_cairoxg_2_2.h"
#include "lib_dendrogram_1_0.h"

#define NUM_PATTERNS 16

#define CAT_MAX 4

static char *training_set_name = "Tyler";
typedef enum similarity_type {
    SIMILARITY_COSINE, SIMILARITY_CORRELATION
} SimilarityType;

/******************************************************************************/

static int               tyler_pattern_mode = 0;
static Boolean           tyler_pattern_colour = TRUE;

static double            tyler_pattern_distance_matrix[NUM_PATTERNS*NUM_PATTERNS];
static double            tyler_pattern_similarity_matrix[NUM_PATTERNS*NUM_PATTERNS];

static MetricType        tyler_pattern_metric = METRIC_EUCLIDEAN;
static SimilarityType    tyler_pattern_similarity = SIMILARITY_COSINE;
static LinkageType       tyler_pattern_linkage = LT_MAX;

static GtkWidget        *tyler_pattern_viewer_widget = NULL;
static cairo_surface_t  *tyler_pattern_viewer_surface = NULL;
static GtkWidget        *tyler_pattern_controls[8];

static char *print_file_prefix[4] = 
    {"features", "norms", "similarity", "dendrograms"};

static char *category_name[CAT_MAX] = 
    {"Bird", "Mammal", "Tool", "Vehicle"};

static char *metric_name[2] = 
    {"Euclidean", "Jaccard"};

static char *similarity_name[2] = 
    {"Cosine", "Correlation"};

static char *linkage_name[3] =
    {"Mean", "Maximum", "Minimum"};

/*----------------------------------------------------------------------------*/

static int pattern_get_category(PatternList *pattern)
{
    if (pattern_is_animal1(pattern)) {
        return(0);
    }
    else if (pattern_is_animal2(pattern)) {
        return(1);
    }
    else if (pattern_is_artifact1(pattern)) {
        return(2);
    }
    else if (pattern_is_artifact2(pattern)) {
        return(3);
    }
    else {
        // Should not happen:
        return(0);
    }
}

/*----------------------------------------------------------------------------*/

static double hub_distance(int n, double *vector1, double *vector2)
{
    if (tyler_pattern_metric == METRIC_JACCARD) {
        return(jaccard_distance(n, vector1, vector2));
    }
    else if (tyler_pattern_metric == METRIC_EUCLIDEAN) {
        return(euclidean_distance(n, vector1, vector2));
    }
    else {
        return(0.0);
    } 
}

/*----------------------------------------------------------------------------*/

static void tyler_pattern_viewer_repaint_vectors(cairo_t *cr, PangoLayout *layout, XGlobals *xg)
{
    CairoxTextParameters tp;
    char buffer[128];
    PatternList *pattern = NULL;
    int count = 0;

    // Draw each pattern:

    cairox_centre_text(cr, 100, 20, layout, "Features");
    count = 0;

    for (pattern = xg->training_set; pattern != NULL; pattern = pattern->next) {
        count++;
        cairox_vector_draw(cr, 100, 20+count*14, tyler_pattern_colour, IO_WIDTH, pattern->vector_in);

        cairox_text_parameters_set(&tp, 420, 20+count*14, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_CENTER, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, pattern->name);
        cairox_text_parameters_set(&tp, 480, 20+count*14, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_CENTER, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, category_name[pattern_get_category(pattern)]);
    }
    // Write total count of patterns: 
    g_snprintf(buffer, 128, "Pattern Set: %s [%d pattern(s)]", training_set_name, count);
    cairox_text_parameters_set(&tp, 10, 40+count*14, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_CENTER, 0.0);
    cairox_paint_pango_text(cr, &tp, layout, buffer);
}

/*----------------------------------------------------------------------------*/

static void tyler_pattern_viewer_repaint_norms(cairo_t *cr, PangoLayout *layout, XGlobals *xg)
{
    int width  = tyler_pattern_viewer_widget->allocation.width;
    int height = tyler_pattern_viewer_widget->allocation.height;
    char *cat_labels[CAT_MAX+2] = {"", "Birds", "Mammals", "Tools", "Vehicles", ""};
    char *dom_labels[4] = {"", "Animals", "Artefacts", ""};
    GraphStruct *gd;
    char buffer[128];
    PatternList *p;
    double sum_cat[CAT_MAX];
    double ssq_cat[CAT_MAX];
    int count_cat[CAT_MAX];
    double sum_dom[2];
    double ssq_dom[2];
    int count_dom[2];
    double vector_zero[IO_WIDTH];
    double gr_min, gr_max;
    int i;

    if (tyler_pattern_metric == METRIC_EUCLIDEAN) {
        gr_min = 0; gr_max = 3;
    }
    else if (tyler_pattern_metric == METRIC_JACCARD) {
        gr_min = 0; gr_max = 2;
    }
    else { // Should not happen!
        gr_min = 0; gr_max = 1;
    }

    if ((gd = graph_create(1)) != NULL) {
        CairoxFontProperties *fp;

        // First build and draw the category graph:

        graph_set_extent(gd, 0, 0, width, height*0.5);
        graph_set_margins(gd, 50, 10, 20, 40);
        fp = font_properties_create("Sans", 18, PANGO_STYLE_NORMAL, PANGO_WEIGHT_NORMAL, "black");
        g_snprintf(buffer, 128, "%s Length of Pattern Vector by Category (%s)", metric_name[tyler_pattern_metric], training_set_name);
        graph_set_title_font_properties(gd, fp);
        graph_set_title(gd, buffer);
        graph_set_axis_properties(gd, GTK_ORIENTATION_HORIZONTAL, -1, CAT_MAX, CAT_MAX+2, "%s", "Categories");
        graph_set_axis_tick_marks(gd, GTK_ORIENTATION_HORIZONTAL, CAT_MAX+2, cat_labels);
        graph_set_axis_properties(gd, GTK_ORIENTATION_VERTICAL, gr_min, gr_max, 6, "%3.1f", "Vector Length");
        graph_set_dataset_properties(gd, 0, NULL, 0.5, 0.5, 0.5, 25, LS_SOLID, MARK_NONE);
        gd->dataset[0].points = CAT_MAX;
        font_properties_set_size(fp, 14);
        graph_set_legend_properties(gd, FALSE, 0.0, 0.0, NULL);
        graph_set_legend_font_properties(gd, fp);
        graph_set_axis_font_properties(gd, GTK_ORIENTATION_HORIZONTAL, fp);
        graph_set_axis_font_properties(gd, GTK_ORIENTATION_VERTICAL, fp);
        font_properties_destroy(fp);

        for (i = 0; i < CAT_MAX; i++) {
            sum_cat[i] = 0.0;
            ssq_cat[i] = 0.0;
            count_cat[i] = 0;
        }

        for (i = 0; i < 2; i++) {
            sum_dom[i] = 0.0;
            ssq_dom[i] = 0.0;
            count_dom[i] = 0;
        }

        for (i = 0; i < IO_WIDTH; i++) {
            vector_zero[i] = 0.0;
        }

        for (p = xg->training_set; p != NULL; p = p->next) {
            double d;

            d = hub_distance(IO_WIDTH, p->vector_in, vector_zero);

            sum_cat[(int) pattern_get_category(p)] += d;
            ssq_cat[(int) pattern_get_category(p)] += d*d;
            count_cat[(int) pattern_get_category(p)]++;

            //  Should we consider only unique animal / artifact names???
            if (pattern_is_animal(p)) {
                sum_dom[0] += d;
                ssq_dom[0] += d*d;
                count_dom[0]++;
            }
            if (pattern_is_artifact(p)) {
                sum_dom[1] += d;
                ssq_dom[1] += d*d;
                count_dom[1]++;
            }
        }

        for (i = 0; i < CAT_MAX; i++) {
            gd->dataset[0].x[i] = i;
            gd->dataset[0].y[i] = (count_cat[i] > 0) ? (sum_cat[i] / (double) count_cat[i]) : 0.0;
            gd->dataset[0].se[i] = (count_cat[i] > 1) ? sqrt((ssq_cat[i] - (sum_cat[i]*sum_cat[i] / (double) (count_cat[i])))/((double) count_cat[i]-1)) / sqrt(count_cat[i]) : 0.0;
            //            fprintf(stdout, "%12s: %6.4f\n", category_name[i], (ssq_cat[i] / (double) count_cat[i]));
        }
        cairox_draw_graph(cr, gd, tyler_pattern_colour);

        // Now use the same gd to build and draw the domain graph:
        graph_set_extent(gd, 0, height*0.5, width, height*0.5);
        graph_set_margins(gd, 50, 10, 20, 40);
        g_snprintf(buffer, 128, "%s Length of Pattern Vector by Domain (%s)", metric_name[tyler_pattern_metric], training_set_name);
        graph_set_title(gd, buffer);
        graph_set_axis_properties(gd, GTK_ORIENTATION_HORIZONTAL, -1, 2, 4, "%s", "Domains");
        graph_set_axis_tick_marks(gd, GTK_ORIENTATION_HORIZONTAL, 4, dom_labels);
        graph_set_axis_properties(gd, GTK_ORIENTATION_VERTICAL, gr_min, gr_max, 6, "%3.1f", "Vector Length");
        graph_set_dataset_properties(gd, 0, NULL, 0.5, 0.5, 0.5, 50, LS_SOLID, MARK_NONE);
        gd->dataset[0].points = 2;

        for (i = 0; i < 2; i++) {
            gd->dataset[0].x[i] = i;
            gd->dataset[0].y[i] = (count_dom[i] > 0) ? (sum_dom[i] / (double) count_dom[i]) : 0.0;
            gd->dataset[0].se[i] = (count_dom[i] > 1) ? sqrt((ssq_dom[i] - (sum_dom[i]*sum_dom[i] / (double) (count_dom[i])))/((double) count_dom[i]-1)) / sqrt(count_dom[i]) : 0.0;
        }
        cairox_draw_graph(cr, gd, tyler_pattern_colour);

        graph_destroy(gd);
    }
}

static double range_round_up(double y)
{
    return(pow(10, ceil(log(y) / log(10))));
}

static void tyler_pattern_viewer_repaint_similarity(cairo_t *cr, PangoLayout *layout, XGlobals *xg)
{
    CairoxTextParameters tp;
    CairoxScaleParameters ss;
    int width  = tyler_pattern_viewer_widget->allocation.width;
    double vector_io[NUM_PATTERNS][IO_WIDTH];
    char buffer[64];
    double x, y, dx, dy;
    double d, max_d;
    int i, j;
    PatternList *p;

    ss.fmt = NULL;

    // Recompute similarity (inefficient to do this every time, but accurate)

    // Build the IO vectors:
    j = 0;
    for (p = xg->training_set; p != NULL; p = p->next) {
        for (i = 0; i < IO_WIDTH; i++) {
            vector_io[j][i] = p->vector_in[i];
        }
        j++;
    }

    // Now calculate similarity:
    max_d = 0;
    for (i = 0; i < NUM_PATTERNS; i++) {
        for (j = 0; j < NUM_PATTERNS; j++) {
            // Similarity as measured by preferred distance metric
            d = hub_distance(IO_WIDTH, vector_io[j], vector_io[i]);
            tyler_pattern_distance_matrix[j*NUM_PATTERNS+i] = d; 
            max_d = MAX(max_d, d);

            if (tyler_pattern_similarity == SIMILARITY_COSINE) {
                // Similarity as measured by cosine:
                d = vector_cosine(IO_WIDTH, vector_io[j], vector_io[i]);
            }
            else if (tyler_pattern_similarity == SIMILARITY_CORRELATION) {
                // Similarity as measured by correlation:
                d = vector_correlation(IO_WIDTH, vector_io[j], vector_io[i]);
                fprintf(stdout, "Correlation %d/%d: %f\n", j, i, d);
            }
            else { 
                // Should not happen!
                d = 0;
            }
            // Rescale to 0/1 flor the pixel map
            tyler_pattern_similarity_matrix[j*NUM_PATTERNS+i] = 1.0 - ((d + 1) / 2.0); 
        }
    }

    max_d = range_round_up(max_d);

    // Normalise distance by dividing by max_d: 
    for (i = 0; i < NUM_PATTERNS; i++) {
        for (j = 0; j < NUM_PATTERNS; j++) {
            tyler_pattern_distance_matrix[j*NUM_PATTERNS+i] = 1 - (tyler_pattern_distance_matrix[j*NUM_PATTERNS+i] / max_d);
        }
    }

    // Paint the distance matrix:

    dx = 16.0;
    dy = 16.0;
    x = width * 0.5 - NUM_PATTERNS * dx - 40;
    y = 20.0;

    cairox_pixel_matrix_draw(cr, x, y, dx, dy, tyler_pattern_distance_matrix, NUM_PATTERNS, tyler_pattern_colour);

    pangox_layout_set_font_size(layout, 14);
    cairox_text_parameters_set(&tp, width*0.5-NUM_PATTERNS*dx*0.5-40, 30+NUM_PATTERNS*dy, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_TOP, 0.0);
    g_snprintf(buffer, 64, "Pattern %s Distance Matrix (%s)", metric_name[tyler_pattern_metric], training_set_name);
    cairox_paint_pango_text(cr, &tp, layout, buffer);

    cairox_scale_parameters_set(&ss, GTK_POS_LEFT, 15, NUM_PATTERNS*dy, 0.0, max_d, 5, "%4.1f");
    cairox_scale_draw(cr, layout, &ss, x-30, y, tyler_pattern_colour);

    // Paint the correlation matrix:

    dx = 16.0;
    dy = 16.0;
    x = width * 0.5 + 40;
    y = 20.0;

    cairox_pixel_matrix_draw(cr, x, y, dx, dy, tyler_pattern_similarity_matrix, NUM_PATTERNS, tyler_pattern_colour);

    pangox_layout_set_font_size(layout, 14);
    cairox_text_parameters_set(&tp, width*0.5+NUM_PATTERNS*dx*0.5+40, 30+NUM_PATTERNS*dy, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_TOP, 0.0);
    g_snprintf(buffer, 64, "Pattern %s Matrix (%s)", similarity_name[tyler_pattern_similarity], training_set_name);
    cairox_paint_pango_text(cr, &tp, layout, buffer);

    cairox_scale_parameters_set(&ss, GTK_POS_RIGHT, 15, NUM_PATTERNS*dy, -1.0, 1.0, 4, "%+4.1f");
    cairox_scale_draw(cr, layout, &ss, x+NUM_PATTERNS*dy+15, y, tyler_pattern_colour);

    g_free(ss.fmt);
}

/*----------------------------------------------------------------------------*/

static NamedVectorArray *retrieve_vectors(PatternList *patterns)
{
    PatternList *p;
    NamedVectorArray *list;
    int i, j = 0;

    if ((list = (NamedVectorArray *)malloc(pattern_list_length(patterns) * sizeof(NamedVectorArray))) != NULL) {
        for (p = patterns; p != NULL; p = p->next) {
            if ((list[j].vector = (double *)malloc(IO_WIDTH * sizeof(double))) != NULL) {
                for (i = 0; i < IO_WIDTH; i++) {
                    list[j].vector[i] = p->vector_in[i];
                }
                list[j].name = string_copy(p->name);
                j++;
            }
        }
    }
    return(list);
}

/*----------------------------------------------------------------------------*/

static void tyler_pattern_viewer_repaint_dendrogram(cairo_t *cr, PangoLayout *layout, XGlobals *xg)
{
    DendrogramStruct *hpd;
    CairoxTextParameters tp;
    CairoxFontProperties *fp;
    NamedVectorArray *list;
    int num_patterns;
    int x;

    if ((list = retrieve_vectors(xg->training_set)) != NULL) {
        num_patterns = pattern_list_length(xg->training_set);
        if ((hpd = dendrogram_create(list, IO_WIDTH, num_patterns, tyler_pattern_metric, tyler_pattern_linkage)) != NULL) {
            int height  = tyler_pattern_viewer_widget->allocation.height;
            int width  = tyler_pattern_viewer_widget->allocation.width;

            cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
            // X coordinate to centre the dendrogram:
            x = (width - num_patterns*15) / 2;

            fp = font_properties_create("Sans", 14, PANGO_STYLE_NORMAL, PANGO_WEIGHT_NORMAL, "black");
            dendrogram_draw(cr, hpd, fp, x, 20, height);

            pangox_layout_set_font_properties(layout, fp);
            // Annotate the dendrogram with parameter settings:
            cairox_text_parameters_set(&tp, width-150, 20, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_CENTER, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, "Pattern Set: ");
            cairox_text_parameters_set(&tp, width-75, 20, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_CENTER, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, training_set_name);

            cairox_text_parameters_set(&tp, width-150, 35, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_CENTER, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, "Metric: ");
            cairox_text_parameters_set(&tp, width-75, 35, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_CENTER, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, metric_name[tyler_pattern_metric]);

            cairox_text_parameters_set(&tp, width-150, 50, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_CENTER, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, "Linkage: ");
            cairox_text_parameters_set(&tp, width-75, 50, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_CENTER, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, linkage_name[tyler_pattern_linkage]);

            font_properties_destroy(fp);
            dendrogram_free(hpd);
        }
        named_vector_array_free(list, num_patterns);
    }
}

/*----------------------------------------------------------------------------*/

static Boolean tyler_pattern_viewer_repaint(GtkWidget *caller, GdkEvent *event, XGlobals *xg)
{
    if (!GTK_WIDGET_MAPPED(tyler_pattern_viewer_widget) || (tyler_pattern_viewer_surface == NULL)) {
        return(TRUE);
    }
    else {
        PangoLayout *layout;
        cairo_t *cr;

        cr = cairo_create(tyler_pattern_viewer_surface);
        layout = pango_cairo_create_layout(cr);
        pangox_layout_set_font_size(layout, 12);

        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_paint(cr);

        if (tyler_pattern_mode == 0) {
            // Make sure the widget is big enough:
            int num_patterns = pattern_list_length(xg->training_set);
            gtk_widget_set_size_request(tyler_pattern_viewer_widget, -1, 60+num_patterns*14);
            gtkx_flush_events();
            tyler_pattern_viewer_repaint_vectors(cr, layout, xg);
        }
        else if (tyler_pattern_mode == 1) {
            gtk_widget_set_size_request(tyler_pattern_viewer_widget, -1, -1);
            gtkx_flush_events();
            tyler_pattern_viewer_repaint_norms(cr, layout, xg);
        }
        else if (tyler_pattern_mode == 2) {
            gtk_widget_set_size_request(tyler_pattern_viewer_widget, -1, -1);
            gtkx_flush_events();
            tyler_pattern_viewer_repaint_similarity(cr, layout, xg);
        }
        else if (tyler_pattern_mode == 3) {
            int num_patterns = pattern_list_length(xg->training_set);
            gtk_widget_set_size_request(tyler_pattern_viewer_widget, -1, num_patterns*15);
            gtkx_flush_events();
            tyler_pattern_viewer_repaint_dendrogram(cr, layout, xg);
        }

        g_object_unref(layout);
        cairo_destroy(cr);

        // Now copy the surface to the window:
        if ((tyler_pattern_viewer_widget->window != NULL) && ((cr = gdk_cairo_create(tyler_pattern_viewer_widget->window)) != NULL)) {
            cairo_set_source_surface(cr, tyler_pattern_viewer_surface, 0, 0);
            cairo_paint(cr);
            cairo_destroy(cr);
        }
    }
    return(FALSE);
}

/******************************************************************************/

static void tyler_pattern_show_mode_widgets()
{
    if (tyler_pattern_mode == 0) { // Vector list
        // Hide metric selector
        gtk_widget_hide(tyler_pattern_controls[2]);
        gtk_widget_hide(tyler_pattern_controls[3]);
    }
    else {
        gtk_widget_show(tyler_pattern_controls[2]);
        gtk_widget_show(tyler_pattern_controls[3]);
    }

    if (tyler_pattern_mode == 2) { // Similarity matrices
        gtk_widget_show(tyler_pattern_controls[6]);
        gtk_widget_show(tyler_pattern_controls[7]);
    }
    else {
        gtk_widget_hide(tyler_pattern_controls[6]);
        gtk_widget_hide(tyler_pattern_controls[7]);
    }

    if (tyler_pattern_mode == 3) { // Dendrogram
        // Hide the colour selector
        gtk_widget_hide(tyler_pattern_controls[0]);
        gtk_widget_hide(tyler_pattern_controls[1]);
        // Show the linkage selector
        gtk_widget_show(tyler_pattern_controls[4]);
        gtk_widget_show(tyler_pattern_controls[5]);
    }
    else {
        // Show the colour selector
        gtk_widget_show(tyler_pattern_controls[0]);
        gtk_widget_show(tyler_pattern_controls[1]);
        // Hide the linkage selector
        gtk_widget_hide(tyler_pattern_controls[4]);
        gtk_widget_hide(tyler_pattern_controls[5]);
    }
}

/*----------------------------------------------------------------------------*/

static void callback_select_pattern_mode(GtkWidget *combobox, XGlobals *xg)
{
    tyler_pattern_mode = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox));
    tyler_pattern_show_mode_widgets();
    tyler_pattern_viewer_repaint(NULL, NULL, xg);
}

/*----------------------------------------------------------------------------*/

static void callback_select_metric(GtkWidget *combobox, XGlobals *xg)
{
    tyler_pattern_metric = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox));
    tyler_pattern_viewer_repaint(NULL, NULL, xg);
}

/*----------------------------------------------------------------------------*/

static void callback_select_similarity(GtkWidget *combobox, XGlobals *xg)
{
    tyler_pattern_similarity  = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox));
    tyler_pattern_viewer_repaint(NULL, NULL, xg);
}

/*----------------------------------------------------------------------------*/

static void callback_select_linkage(GtkWidget *combobox, XGlobals *xg)
{
    tyler_pattern_linkage = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox));
    tyler_pattern_viewer_repaint(NULL, NULL, xg);
}

/*----------------------------------------------------------------------------*/

static void callback_toggle_colour(GtkWidget *caller, XGlobals *xg)
{
    tyler_pattern_colour = (Boolean) (GTK_TOGGLE_BUTTON(caller)->active);
    tyler_pattern_viewer_repaint(NULL, NULL, xg);
}

/*----------------------------------------------------------------------------*/

static void xhub_patterns_print_callback(GtkWidget *caller, XGlobals *xg)
{
    cairo_surface_t *surface = NULL;
    cairo_t *cr = NULL;
    PangoLayout *layout;
    char filename[128], prefix[64];
    int width  = tyler_pattern_viewer_widget->allocation.width;
    int height = tyler_pattern_viewer_widget->allocation.height;

    g_snprintf(prefix, 64, "pattern_%s_%s", training_set_name, print_file_prefix[tyler_pattern_mode]);

    /* PDF version: */
    g_snprintf(filename, 128, "FIGURES/%s.pdf", prefix);
    surface = cairo_pdf_surface_create(filename, width, height);
    cr = cairo_create(surface);
    layout = pango_cairo_create_layout(cr);

    if (tyler_pattern_mode == 0) {
        tyler_pattern_viewer_repaint_vectors(cr, layout, xg);
    }
    else if (tyler_pattern_mode == 1) {
        tyler_pattern_viewer_repaint_norms(cr, layout, xg);
    }
    else if (tyler_pattern_mode == 2) {
        tyler_pattern_viewer_repaint_similarity(cr, layout, xg);
    }
    else if (tyler_pattern_mode == 3) {
        tyler_pattern_viewer_repaint_dendrogram(cr, layout, xg);
    }
    g_object_unref(layout);
    cairo_destroy(cr);

    /* Save the PNG while we're at it: */
    g_snprintf(filename, 128, "FIGURES/%s.png", prefix);
    cairo_surface_write_to_png(surface, filename);
    /* And destroy the surface */
    cairo_surface_destroy(surface);
}

/*----------------------------------------------------------------------------*/

void tyler_patterns_create_widgets(GtkWidget *page, XGlobals *xg)
{
    GtkWidget *scrolled_win, *hbox, *tmp;

    /* Top row of selector widgets: */

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    /* Widgets for setting display type: */
    tmp = gtk_label_new("Show:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    tmp = gtk_combo_box_new_text();
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Vectors");
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Vector Norms");
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Vector Similarity");
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Dendrogram");
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), tyler_pattern_mode);
    g_signal_connect(G_OBJECT(tmp), "changed", G_CALLBACK(callback_select_pattern_mode), xg);
    gtk_widget_show(tmp);

    /*--- Filler: ---*/
    tmp = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 0);
    gtk_widget_show(tmp);

    /* Widgets for setting the metric: */
    /*--- The label: ---*/
    tmp = gtk_label_new("  Metric: ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
    tyler_pattern_controls[2] = tmp;
    /*--- The combobox selector: ---*/
    tmp = gtk_combo_box_new_text();
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), metric_name[0]);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), metric_name[1]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), tyler_pattern_metric);
    g_signal_connect(G_OBJECT(tmp), "changed", G_CALLBACK(callback_select_metric), xg);
    gtk_widget_show(tmp);
    tyler_pattern_controls[3] = tmp;

    /* Widgets for setting cosine/correlation similarity: */
    /*--- The label: ---*/
    tmp = gtk_label_new("  Similarity: ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
    tyler_pattern_controls[6] = tmp;
    /*--- The combobox selector: ---*/
    tmp = gtk_combo_box_new_text();
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), similarity_name[0]);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), similarity_name[1]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), tyler_pattern_similarity);
    g_signal_connect(G_OBJECT(tmp), "changed", G_CALLBACK(callback_select_similarity), xg);
    gtk_widget_show(tmp);
    tyler_pattern_controls[7] = tmp;

    /* Widgets for setting the linkage function: */
    /*--- The label: ---*/
    tmp = gtk_label_new("  Linkage: ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
    tyler_pattern_controls[4] = tmp;
    /*--- The combobox selector: ---*/
    tmp = gtk_combo_box_new_text();
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), linkage_name[0]);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), linkage_name[1]);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), linkage_name[2]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), tyler_pattern_linkage);
    g_signal_connect(G_OBJECT(tmp), "changed", G_CALLBACK(callback_select_linkage), xg);
    gtk_widget_show(tmp);
    tyler_pattern_controls[5] = tmp;

    /*--- Filler: ---*/
    tmp = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 0);
    gtk_widget_show(tmp);

    /* Widgets for setting colour/greyscale: */
    /*--- The label: ---*/
    tmp = gtk_label_new("  Colour: ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
    tyler_pattern_controls[0] = tmp;
    /*--- The checkbox: ---*/
    tmp = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), tyler_pattern_colour);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(tmp), "toggled", G_CALLBACK(callback_toggle_colour), xg);
    gtk_widget_show(tmp);
    tyler_pattern_controls[1] = tmp;

    /*--- Spacer: ---*/
    tmp = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    /*--- And a print button: ---*/
    gtkx_stock_image_button_create(hbox, GTK_STOCK_PRINT, G_CALLBACK(xhub_patterns_print_callback), xg);

    /* The rest of the page - a scrolling window: */

    scrolled_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(page), scrolled_win, TRUE, TRUE, 0);
    gtk_widget_show(scrolled_win);

    // The drawing area:
    tyler_pattern_viewer_widget = gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(tyler_pattern_viewer_widget), "expose_event", G_CALLBACK(tyler_pattern_viewer_repaint), xg);
    g_signal_connect(G_OBJECT(tyler_pattern_viewer_widget), "configure_event", G_CALLBACK(gtkx_configure_surface_callback), &tyler_pattern_viewer_surface);
    g_signal_connect(G_OBJECT(tyler_pattern_viewer_widget), "destroy", G_CALLBACK(gtkx_destroy_surface_callback), &tyler_pattern_viewer_surface);
    gtk_widget_set_events(tyler_pattern_viewer_widget, GDK_EXPOSURE_MASK);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_win), tyler_pattern_viewer_widget);
    gtk_widget_show(tyler_pattern_viewer_widget);

    // Set widgets for the current mode: 
    tyler_pattern_show_mode_widgets();
    gtk_widget_show(page);
}

/******************************************************************************/
