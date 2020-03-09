/*******************************************************************************

    File:       xhub_patterns.c
    Contents:   Routines to perform and display a range of pattern-set analyses
    Created:    08/11/16
    Author:     Rick Cooper
    Copyright (c) 2016 Richard P. Cooper

    Public procedures:
        void GtkWidget *hub_pattern_set_widgets(GtkWidget *page, XGlobals *xg)

TO DO:
* Tidy up (e.g., Category names should not be local to this file)
* Get rid of NUM_PATTERNS symbol

*******************************************************************************/
/******** Include files: ******************************************************/

#include "xhub.h"
#include "lib_maths.h"
#include "lib_string.h"
#include "lib_cairox.h"
#include "lib_cairoxg_2_2.h"
#include "lib_dendrogram_1_0.h"

#define NUM_PATTERNS 48

#undef SIMILARITY

typedef enum similarity_type {
    SIMILARITY_COSINE, SIMILARITY_CORRELATION
} SimilarityType;

/******************************************************************************/

static int               hub_pattern_mode = 0;
static Boolean           hub_pattern_colour = TRUE;

static double            hub_pattern_distance_matrix[NUM_PATTERNS*NUM_PATTERNS];
static double            hub_pattern_similarity_matrix[NUM_PATTERNS*NUM_PATTERNS];

static MetricType        hub_pattern_metric = METRIC_EUCLIDEAN;
static SimilarityType    hub_pattern_similarity = SIMILARITY_CORRELATION;
static LinkageType       hub_pattern_linkage = LT_MAX;
static int               hub_pattern_matrix_show = 0;

static GtkWidget        *hub_pattern_viewer_widget = NULL;
static cairo_surface_t  *hub_pattern_viewer_surface = NULL;
static GtkWidget        *hub_pattern_controls[10];

static char *print_file_prefix[4] = 
    {"features", "norms", "similarity", "dendrograms"};

static char *category_name[6] = 
    {"Birds", "Mammals", "Fruits", "Tools", "Vehicles", "HH Objects"};

static char *metric_name[2] = 
    {"Euclidean", "Jaccard"};

static char *similarity_name[2] = 
    {"Cosine", "Correlation"};

static char *linkage_name[3] =
    {"Mean", "Maximum", "Minimum"};

/*----------------------------------------------------------------------------*/

static double hub_distance(int n, double *vector1, double *vector2)
{
    if (hub_pattern_metric == METRIC_JACCARD) {
        return(jaccard_distance(n, vector1, vector2));
    }
    else if (hub_pattern_metric == METRIC_EUCLIDEAN) {
        return(euclidean_distance(n, vector1, vector2));
    }
    else {
        return(0.0);
    } 
}

/*----------------------------------------------------------------------------*/

static void hub_pattern_viewer_repaint_vectors(cairo_t *cr, PangoLayout *layout, XGlobals *xg)
{
    CairoxTextParameters tp;
    char buffer[128];
    PatternList *pattern = NULL;
    int count = 0;

    // Draw each pattern:

    cairox_centre_text(cr, 100, 20, layout, "Name");
    cairox_centre_text(cr, 416, 20, layout, "Verbal");
    cairox_centre_text(cr, 780, 20, layout, "Visual");
    count = 0;

    for (pattern = xg->pattern_set; pattern != NULL; pattern = pattern->next) {
        count++;
        cairox_vector_draw(cr, 100, 20+count*14, hub_pattern_colour, NUM_NAME, pattern->name_features);
        cairox_vector_draw(cr, 416, 20+count*14, hub_pattern_colour, NUM_VERBAL, pattern->verbal_features);
        cairox_vector_draw(cr, 780, 20+count*14, hub_pattern_colour, NUM_VISUAL, pattern->visual_features);

        cairox_text_parameters_set(&tp, 920, 20+count*14, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_CENTER, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, pattern->name);
        cairox_text_parameters_set(&tp, 980, 20+count*14, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_CENTER, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, category_name[pattern->category]);
    }
    // Write total count of patterns: 
    g_snprintf(buffer, 128, "Pattern Set %s [%d pattern(s)]", xg->pattern_set_name, count);
    cairox_text_parameters_set(&tp, 10, 40+count*14, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_CENTER, 0.0);
    cairox_paint_pango_text(cr, &tp, layout, buffer);
}

/*----------------------------------------------------------------------------*/

static void hub_category_similarity(PatternList *p, int c, double *mean, double *se)
{
    PatternList *p1, *p2;
    int n = 0, i;
    double sum = 0.0;
    double ssq = 0.0;
    double d = 0.0;
    double vector1[NUM_IO], vector2[NUM_IO];

    for (p1 = p; p1 != NULL; p1 = p1->next) {
        if (p1->category == c) {
            for (p2 = p; p2 != NULL; p2 = p2->next) {
                if ((p2->category == c) && (p1 != p2)) {
                    for (i = 0; i < NUM_NAME; i++) {
                        vector1[i] = p1->name_features[i];
                        vector2[i] = p2->name_features[i];
                    }
                    for (i = 0; i < NUM_VERBAL; i++) {
                        vector1[NUM_NAME + i] = p1->verbal_features[i];
                        vector2[NUM_NAME + i] = p2->verbal_features[i];
                    }
                    for (i = 0; i < NUM_VISUAL; i++) {
                        vector1[NUM_NAME + NUM_VERBAL + i] = p1->visual_features[i];
                        vector2[NUM_NAME + NUM_VERBAL + i] = p2->visual_features[i];
                    }

                    if (hub_pattern_similarity == SIMILARITY_COSINE) {
                        // Similarity as measured by cosine:
                        d = vector_cosine(NUM_IO, vector1, vector2);
                    }
                    else if (hub_pattern_similarity == SIMILARITY_CORRELATION) { 
                        // Similarity as measured by correlation:
                        d = vector_correlation(NUM_IO, vector1, vector2);
                    }
                    d = hub_distance(NUM_IO, vector1, vector2);
                    sum += d;
                    ssq += d*d;
                    n++;
                }
            }
        }
    }

    *mean = (n > 0) ? (sum / (double) n) : 0.0;
    *se =  (n > 1) ? sqrt((ssq - (sum*sum / (double) n))/((double) n-1)) / sqrt(n) : 0.0;
}

/*----------------------------------------------------------------------------*/

static Boolean pattern_is_in_domain(PatternList *p, int d)
{
    if (d == 0) {
        return(pattern_is_animal(p));
    }
    else if (d == 1) {
        return(pattern_is_artifact(p));
    }
    else {
        return(FALSE);
    }
}


static void hub_domain_similarity(PatternList *p, int dom, double *mean, double *se)
{
    PatternList *p1, *p2;
    int n = 0, i;
    double sum = 0.0;
    double ssq = 0.0;
    double d = 0.0;
    double vector1[NUM_IO], vector2[NUM_IO];

    for (p1 = p; p1 != NULL; p1 = p1->next) {
        if (pattern_is_in_domain(p1, dom)) {
            for (p2 = p; p2 != NULL; p2 = p2->next) {
                if (pattern_is_in_domain(p2, dom) && (p1 != p2)) {
                    for (i = 0; i < NUM_NAME; i++) {
                        vector1[i] = p1->name_features[i];
                        vector2[i] = p2->name_features[i];
                    }
                    for (i = 0; i < NUM_VERBAL; i++) {
                        vector1[NUM_NAME + i] = p1->verbal_features[i];
                        vector2[NUM_NAME + i] = p2->verbal_features[i];
                    }
                    for (i = 0; i < NUM_VISUAL; i++) {
                        vector1[NUM_NAME + NUM_VERBAL + i] = p1->visual_features[i];
                        vector2[NUM_NAME + NUM_VERBAL + i] = p2->visual_features[i];
                    }

                    if (hub_pattern_similarity == SIMILARITY_COSINE) {
                        // Similarity as measured by cosine:
                        d = vector_cosine(NUM_IO, vector1, vector2);
                    }
                    else if (hub_pattern_similarity == SIMILARITY_CORRELATION) { 
                        // Similarity as measured by correlation:
                        d = vector_correlation(NUM_IO, vector1, vector2);
                    }
                    d = hub_distance(NUM_IO, vector1, vector2);
                    sum += d;
                    ssq += d*d;
                    n++;
                }
            }
        }
    }

    *mean = (n > 0) ? (sum / (double) n) : 0.0;
    *se =  (n > 1) ? sqrt((ssq - (sum*sum / (double) n))/((double) n-1)) / sqrt(n) : 0.0;
}

/*----------------------------------------------------------------------------*/

static void hub_pattern_viewer_repaint_norms(cairo_t *cr, PangoLayout *layout, XGlobals *xg, int width, int height)
{
    char *cat_labels[2*CAT_MAX+1] = {"", "Birds", "", "Mammals", "", "Fruits", "", "Tools", "", "Vehicles", "", "HH Objects", ""};
    char *dom_labels[5] = {"", "Animals", "", "Artefacts", ""};
    GraphStruct *gd;
    char buffer[128];
    PatternList *p;
    double sum_cat[CAT_MAX];
    double ssq_cat[CAT_MAX];
    int count_cat[CAT_MAX];
    double sum_dom[2];
    double ssq_dom[2];
    int count_dom[2];
    double vector_zero[NUM_IO];
    double gr_min, gr_max;
    int i;

    if (hub_pattern_metric == METRIC_EUCLIDEAN) {
        gr_min = 3; gr_max = 8;
    }
    else if (hub_pattern_metric == METRIC_JACCARD) {
        gr_min = 0; gr_max = 2;
    }
    else { // Should not happen!
        gr_min = 0; gr_max = 1;
    }

    if ((gd = graph_create(1)) != NULL) {
        CairoxFontProperties *fp;

        // First build and draw the category length graph:

        graph_set_extent(gd, 0, 0, width*0.5, height*0.5);
        graph_set_margins(gd, 50, 10, 25, 40);
        fp = font_properties_create("Sans", 18, PANGO_STYLE_NORMAL, PANGO_WEIGHT_NORMAL, "black");
        g_snprintf(buffer, 128, "Mean Pattern Vector Length (%s)", xg->pattern_set_name);
        graph_set_title_font_properties(gd, fp);
        graph_set_title(gd, buffer);
        graph_set_axis_properties(gd, GTK_ORIENTATION_HORIZONTAL, -0.5, CAT_MAX-0.5, 2*CAT_MAX+1, "%s", "Categories");
        graph_set_axis_tick_marks(gd, GTK_ORIENTATION_HORIZONTAL, 2*CAT_MAX+1, cat_labels);

        g_snprintf(buffer, 128, "%s Length", metric_name[hub_pattern_metric]);
        graph_set_axis_properties(gd, GTK_ORIENTATION_VERTICAL, gr_min, gr_max, 6, "%3.1f", buffer);
        graph_set_dataset_properties(gd, 0, NULL, 0.5, 0.5, 0.5, 1.0, 25, LS_SOLID, MARK_NONE);
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

        for (i = 0; i < NUM_IO; i++) {
            vector_zero[i] = 0.0;
        }

        for (p = xg->pattern_set; p != NULL; p = p->next) {
            double vector_io[NUM_IO];
            double d;

            for (i = 0; i < NUM_NAME; i++) {
                vector_io[i] = p->name_features[i];
            }
            for (i = 0; i < NUM_VERBAL; i++) {
                vector_io[NUM_NAME + i] = p->verbal_features[i];
            }
            for (i = 0; i < NUM_VISUAL; i++) {
                vector_io[NUM_NAME + NUM_VERBAL + i] = p->visual_features[i];
            }

            d = hub_distance(NUM_IO, vector_io, vector_zero);

            sum_cat[(int) p->category] += d;
            ssq_cat[(int) p->category] += d*d;
            count_cat[(int) p->category]++;

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
        cairox_draw_graph(cr, gd, hub_pattern_colour);

        // Second build and draw the category similarity graph:

        graph_set_extent(gd, width*0.5, 0, width*0.5, height*0.5);
        graph_set_margins(gd, 50, 10, 25, 40);
        fp = font_properties_create("Sans", 18, PANGO_STYLE_NORMAL, PANGO_WEIGHT_NORMAL, "black");
        g_snprintf(buffer, 128, "Within-Category Pattern Similarity (%s)", xg->pattern_set_name);
        graph_set_title_font_properties(gd, fp);
        graph_set_title(gd, buffer);
        graph_set_axis_properties(gd, GTK_ORIENTATION_HORIZONTAL, -0.5, CAT_MAX-0.5, 2*CAT_MAX+1, "%s", "Categories");
        graph_set_axis_tick_marks(gd, GTK_ORIENTATION_HORIZONTAL, 2*CAT_MAX+1, cat_labels);
#ifdef SIMILAR
        graph_set_axis_properties(gd, GTK_ORIENTATION_VERTICAL, 0.0, 1.0, 6, "%3.1f", similarity_name[hub_pattern_similarity]);
#else
        g_snprintf(buffer, 128, "%s distance", metric_name[hub_pattern_metric]);
        graph_set_axis_properties(gd, GTK_ORIENTATION_VERTICAL, 3.0, 8.0, 6, "%3.1f", buffer);
#endif

        graph_set_dataset_properties(gd, 0, NULL, 0.5, 0.5, 0.5, 1.0, 25, LS_SOLID, MARK_NONE);
        gd->dataset[0].points = CAT_MAX;
        font_properties_set_size(fp, 14);
        graph_set_legend_properties(gd, FALSE, 0.0, 0.0, NULL);
        graph_set_legend_font_properties(gd, fp);
        graph_set_axis_font_properties(gd, GTK_ORIENTATION_HORIZONTAL, fp);
        graph_set_axis_font_properties(gd, GTK_ORIENTATION_VERTICAL, fp);
        font_properties_destroy(fp);

        for (i = 0; i < CAT_MAX; i++) {
            double mean, se;
            hub_category_similarity(xg->pattern_set, i, &mean, &se);
            gd->dataset[0].x[i] = i;
            gd->dataset[0].y[i] = mean;
            gd->dataset[0].se[i] = se;
        }
        cairox_draw_graph(cr, gd, hub_pattern_colour);

        // Now use the same gd to build and draw the domain graph:
        graph_set_extent(gd, 0, height*0.5, width*0.5, height*0.5);
        graph_set_margins(gd, 50, 10, 25, 40);
        g_snprintf(buffer, 128, "Mean Pattern Vector Length (%s)", xg->pattern_set_name);
        graph_set_title(gd, buffer);
        graph_set_axis_properties(gd, GTK_ORIENTATION_HORIZONTAL, -0.5, 1.5, 5, "%s", "Domains");
        graph_set_axis_tick_marks(gd, GTK_ORIENTATION_HORIZONTAL, 5, dom_labels);
        g_snprintf(buffer, 128, "%s Length", metric_name[hub_pattern_metric]);
        graph_set_axis_properties(gd, GTK_ORIENTATION_VERTICAL, gr_min, gr_max, 6, "%3.1f", buffer);
        graph_set_dataset_properties(gd, 0, NULL, 0.5, 0.5, 0.5, 1.0, 50, LS_SOLID, MARK_NONE);
        gd->dataset[0].points = 2;

        for (i = 0; i < 2; i++) {
            gd->dataset[0].x[i] = i;
            gd->dataset[0].y[i] = (count_dom[i] > 0) ? (sum_dom[i] / (double) count_dom[i]) : 0.0;
            gd->dataset[0].se[i] = (count_dom[i] > 1) ? sqrt((ssq_dom[i] - (sum_dom[i]*sum_dom[i] / (double) (count_dom[i])))/((double) count_dom[i]-1)) / sqrt(count_dom[i]) : 0.0;
        }
        cairox_draw_graph(cr, gd, hub_pattern_colour);

        // And finally the RH domain graph:
        graph_set_extent(gd, width*0.5, height*0.5, width*0.5, height*0.5);
        graph_set_margins(gd, 50, 10, 25, 40);
        g_snprintf(buffer, 128, "Within-Domain Pattern Similarity (%s)", xg->pattern_set_name);
        graph_set_title(gd, buffer);
        graph_set_axis_properties(gd, GTK_ORIENTATION_HORIZONTAL, -0.5, 1.5, 5, "%s", "Domains");
        graph_set_axis_tick_marks(gd, GTK_ORIENTATION_HORIZONTAL, 5, dom_labels);
#ifdef SIMILAR
        graph_set_axis_properties(gd, GTK_ORIENTATION_VERTICAL, 0.0, 1.0, 6, "%3.1f", similarity_name[hub_pattern_similarity]);
#else
        g_snprintf(buffer, 128, "%s distance", metric_name[hub_pattern_metric]);
        graph_set_axis_properties(gd, GTK_ORIENTATION_VERTICAL, 3.0, 8.0, 6, "%3.1f", buffer);
#endif
        graph_set_dataset_properties(gd, 0, NULL, 0.5, 0.5, 0.5, 1.0, 50, LS_SOLID, MARK_NONE);
        gd->dataset[0].points = 2;

        for (i = 0; i < 2; i++) {
            double mean, se;
            hub_domain_similarity(xg->pattern_set, i, &mean, &se);
            gd->dataset[0].x[i] = i;
            gd->dataset[0].y[i] = mean;
            gd->dataset[0].se[i] = se;
        }
        cairox_draw_graph(cr, gd, hub_pattern_colour);

        graph_destroy(gd);
    }
}

static double range_round_up(double y)
{
    return(pow(10, ceil(log(y) / log(10))));
}

static void paint_matrix_category_labels(cairo_t *cr, PangoLayout *layout, double x, double y, double dx, double dy)
{
    // Labels below the matrix: WARNING - THESE ASSUME THAT THE PATTERN FILE DEFINES PATTERNS IN THE SAME ORDER AS category_name[i]
    CairoxTextParameters tp;
    CairoxLineParameters lp;
    int i;

    y = y+NUM_PATTERNS*dy;

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairox_line_parameters_set(&lp, 1.0, LS_SOLID, FALSE);

    pangox_layout_set_font_size(layout, 12);
    for (i = 0; i < CAT_MAX; i++) {
        cairox_text_parameters_set(&tp, x+dx*(8*i+4), y+4, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_TOP, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, category_name[i]);

        if (i > 0) {
            cairox_paint_line(cr, &lp, x+dx*(8*i), y, x+dx*(8*i), y+5);
        }

    }

    // And labels to the left, with the same assumption
    for (i = 0; i < CAT_MAX; i++) {
        cairox_text_parameters_set(&tp, x-3, y-dy*(NUM_PATTERNS-8*(i+0.5)), PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 90.0);
        cairox_paint_pango_text(cr, &tp, layout, category_name[i]);

        if (i > 0) {
            cairox_paint_line(cr, &lp, x-5, y-dy*(NUM_PATTERNS-8*i), x, y-dy*(NUM_PATTERNS-8*i));
        }

    }
}

static void hub_pattern_viewer_repaint_similarity(cairo_t *cr, PangoLayout *layout, XGlobals *xg, int width, int height)
{
    CairoxTextParameters tp;
    CairoxScaleParameters ss;
    double vector_io[NUM_PATTERNS][NUM_IO];
    char buffer[64];
    double x, y, dx, dy;
    double d, max_d;
    int i, j;
    PatternList *p;

    ss.fmt = NULL;

    // Recompute similarity (inefficient to do this every time, but accurate)

    // Build the IO vectors:
    j = 0;
    for (p = xg->pattern_set; p != NULL; p = p->next) {
        for (i = 0; i < NUM_NAME; i++) {
            vector_io[j][i] = p->name_features[i];
        }
        for (i = 0; i < NUM_VERBAL; i++) {
            vector_io[j][NUM_NAME + i] = p->verbal_features[i];
        }
        for (i = 0; i < NUM_VISUAL; i++) {
            vector_io[j][NUM_NAME + NUM_VERBAL + i] = p->visual_features[i];
        }
        j++;
    }

    // Now calculate similarity:
    max_d = 0;
    for (i = 0; i < NUM_PATTERNS; i++) {
        for (j = 0; j < NUM_PATTERNS; j++) {
            // Similarity as measured by preferred distance metric
            d = hub_distance(NUM_IO, vector_io[j], vector_io[i]);
            hub_pattern_distance_matrix[j*NUM_PATTERNS+i] = d; 
            max_d = MAX(max_d, d);

            if (hub_pattern_similarity == SIMILARITY_COSINE) {
                // Similarity as measured by cosine:
                d = vector_cosine(NUM_IO, vector_io[j], vector_io[i]);
            }
            else if (hub_pattern_similarity == SIMILARITY_CORRELATION) { 
                // Similarity as measured by correlation:
                d = vector_correlation(NUM_IO, vector_io[j], vector_io[i]);
            }
            else {
                // Should not happen!
                d = 0;
            }
            // Rescale to 0/1 flor the pixel map
            hub_pattern_similarity_matrix[j*NUM_PATTERNS+i] = 1.0 - (d + 1) / 2.0; 
        }
    }

    max_d = range_round_up(max_d);

    // Normalise distance by dividing by max_d: 
    for (i = 0; i < NUM_PATTERNS; i++) {
        for (j = 0; j < NUM_PATTERNS; j++) {
            hub_pattern_distance_matrix[j*NUM_PATTERNS+i] = 1 - (hub_pattern_distance_matrix[j*NUM_PATTERNS+i] / max_d);
        }
    }

    // Pixel size:
    dx = 7.0;
    dy = 7.0;

    if (hub_pattern_matrix_show == 1) {
        // Paint the distance matrix:
        x = (width * 0.5) - (0.5 * NUM_PATTERNS * dx) + 20; // Add 20 because scale goes on left
        y = 25.0;

        cairox_pixel_matrix_draw(cr, x, y, dx, dy, hub_pattern_distance_matrix, NUM_PATTERNS, hub_pattern_colour);

        pangox_layout_set_font_size(layout, 16);
        cairox_text_parameters_set(&tp, x+NUM_PATTERNS*dx*0.5, y-4, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
        g_snprintf(buffer, 64, "Pattern %s Distance Matrix (%s)", metric_name[hub_pattern_metric], xg->pattern_set_name);
        cairox_paint_pango_text(cr, &tp, layout, buffer);
        // Labels below the distance matrix: WARNING - THESE ASSUME THAT THE PATTERN FILE DEFINES PATTERNS IN THE SAME ORDER AS category_name[i]
        paint_matrix_category_labels(cr, layout, x, y, dx, dy);

        cairox_scale_parameters_set(&ss, GTK_POS_LEFT, 15, NUM_PATTERNS*dy, 0.0, max_d, 5, "%4.1f");
        cairox_scale_draw(cr, layout, &ss, x-30, y, hub_pattern_colour);
    }
    else if (hub_pattern_matrix_show == 2) {
        // Paint the correlation matrix:
        x = (width * 0.5) - (0.5 * NUM_PATTERNS * dx) - 20; // Subtract 20 because scale goes on right
        y = 25.0;

        cairox_pixel_matrix_draw(cr, x, y, dx, dy, hub_pattern_similarity_matrix, NUM_PATTERNS, hub_pattern_colour);
        pangox_layout_set_font_size(layout, 16);
        cairox_text_parameters_set(&tp, x+NUM_PATTERNS*dx*0.5, y-4, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
        g_snprintf(buffer, 64, "Pattern %s Matrix (%s)", similarity_name[hub_pattern_similarity], xg->pattern_set_name);
        cairox_paint_pango_text(cr, &tp, layout, buffer);
        // Labels below the distance matrix: WARNING - THESE ASSUME THAT THE PATTERN FILE DEFINES PATTERNS IN THE SAME ORDER AS category_name[i]
        paint_matrix_category_labels(cr, layout, x, y, dx, dy);

        cairox_scale_parameters_set(&ss, GTK_POS_RIGHT, 15, NUM_PATTERNS*dy, -1.0, 1.0, 4, "%+4.1f");
        cairox_scale_draw(cr, layout, &ss, x+NUM_PATTERNS*dx+15, y, hub_pattern_colour);
    }
    else {
        // Paint distance:
        x = width * 0.5 - NUM_PATTERNS * dx - 40;
        y = 25.0;

        cairox_pixel_matrix_draw(cr, x, y, dx, dy, hub_pattern_distance_matrix, NUM_PATTERNS, hub_pattern_colour);

        pangox_layout_set_font_size(layout, 16);
        cairox_text_parameters_set(&tp, x+NUM_PATTERNS*dx*0.5, y-4, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
        g_snprintf(buffer, 64, "Pattern %s Distance Matrix (%s)", metric_name[hub_pattern_metric], xg->pattern_set_name);
        cairox_paint_pango_text(cr, &tp, layout, buffer);
        // Labels below the distance matrix: WARNING - THESE ASSUME THAT THE PATTERN FILE DEFINES PATTERNS IN THE SAME ORDER AS category_name[i]
        paint_matrix_category_labels(cr, layout, x, y, dx, dy);

        cairox_scale_parameters_set(&ss, GTK_POS_LEFT, 15, NUM_PATTERNS*dy, 0.0, max_d, 5, "%4.1f");
        cairox_scale_draw(cr, layout, &ss, x-30, y, hub_pattern_colour);

        // Paint correlation:
        x = width * 0.5 + 40;
        y = 25.0;

        cairox_pixel_matrix_draw(cr, x, y, dx, dy, hub_pattern_similarity_matrix, NUM_PATTERNS, hub_pattern_colour);
        pangox_layout_set_font_size(layout, 16);
        cairox_text_parameters_set(&tp, x+NUM_PATTERNS*dx*0.5, y-4, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
        g_snprintf(buffer, 64, "Pattern %s Matrix (%s)", similarity_name[hub_pattern_similarity], xg->pattern_set_name);
        cairox_paint_pango_text(cr, &tp, layout, buffer);
        // Labels below the distance matrix: WARNING - THESE ASSUME THAT THE PATTERN FILE DEFINES PATTERNS IN THE SAME ORDER AS category_name[i]
        paint_matrix_category_labels(cr, layout, x, y, dx, dy);

        cairox_scale_parameters_set(&ss, GTK_POS_RIGHT, 15, NUM_PATTERNS*dy, -1.0, 1.0, 4, "%+4.1f");
        cairox_scale_draw(cr, layout, &ss, x+NUM_PATTERNS*dx+15, y, hub_pattern_colour);
    }

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
            if ((list[j].vector = (double *)malloc(NUM_IO * sizeof(double))) != NULL) {
                for (i = 0; i < NUM_NAME; i++) {
                    list[j].vector[i] = p->name_features[i];
                }
                for (i = 0; i < NUM_VERBAL; i++) {
                    list[j].vector[NUM_NAME+i] = p->verbal_features[i];
                }
                for (i = 0; i < NUM_VISUAL; i++) {
                    list[j].vector[NUM_NAME+NUM_VERBAL+i] = p->visual_features[i];
                }
                list[j].name = string_copy(p->name);
                j++;
            }
        }
    }
    return(list);
}

/*----------------------------------------------------------------------------*/

static void hub_pattern_viewer_repaint_dendrogram(cairo_t *cr, PangoLayout *layout, XGlobals *xg, int width, int height)
{
    DendrogramStruct *hpd;
    CairoxTextParameters tp;
    CairoxFontProperties *fp;
    NamedVectorArray *list;
    int num_patterns;
    int x;

    if ((list = retrieve_vectors(xg->pattern_set)) != NULL) {
        num_patterns = pattern_list_length(xg->pattern_set);
        if ((hpd = dendrogram_create(list, NUM_IO, num_patterns, hub_pattern_metric, hub_pattern_linkage)) != NULL) {

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
            cairox_paint_pango_text(cr, &tp, layout, xg->pattern_set_name);

            cairox_text_parameters_set(&tp, width-150, 35, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_CENTER, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, "Metric: ");
            cairox_text_parameters_set(&tp, width-75, 35, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_CENTER, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, metric_name[hub_pattern_metric]);

            cairox_text_parameters_set(&tp, width-150, 50, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_CENTER, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, "Linkage: ");
            cairox_text_parameters_set(&tp, width-75, 50, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_CENTER, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, linkage_name[hub_pattern_linkage]);

            font_properties_destroy(fp);
            dendrogram_free(hpd);
        }
        named_vector_array_free(list, num_patterns);
    }
}

/*----------------------------------------------------------------------------*/

static Boolean hub_pattern_viewer_repaint(GtkWidget *caller, GdkEvent *event, XGlobals *xg)
{
    if (!GTK_WIDGET_MAPPED(hub_pattern_viewer_widget) || (hub_pattern_viewer_surface == NULL)) {
        return(TRUE);
    }
    else {
        PangoLayout *layout;
        int num_patterns;
        int width, height;
        cairo_t *cr;

        cr = cairo_create(hub_pattern_viewer_surface);
        layout = pango_cairo_create_layout(cr);
        pangox_layout_set_font_face(layout, "Sans");
        pangox_layout_set_font_size(layout, 12);

        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_paint(cr);

        num_patterns = pattern_list_length(xg->pattern_set);
        if (hub_pattern_mode == 0) {
            // Make sure the widget is big enough:
            gtk_widget_set_size_request(hub_pattern_viewer_widget, 1100, 60+num_patterns*14);
            gtkx_flush_events();
            hub_pattern_viewer_repaint_vectors(cr, layout, xg);
        }
        else if (hub_pattern_mode == 1) {
            gtk_widget_set_size_request(hub_pattern_viewer_widget, -1, -1);
            gtkx_flush_events();
            width  = hub_pattern_viewer_widget->allocation.width;
            height = hub_pattern_viewer_widget->allocation.height;
            hub_pattern_viewer_repaint_norms(cr, layout, xg, width, height);
        }
        else if (hub_pattern_mode == 2) {
            gtk_widget_set_size_request(hub_pattern_viewer_widget, -1, 40+num_patterns*7);
            gtkx_flush_events();
            width  = hub_pattern_viewer_widget->allocation.width;
            height = hub_pattern_viewer_widget->allocation.height;
            hub_pattern_viewer_repaint_similarity(cr, layout, xg, width, height);
        }
        else if (hub_pattern_mode == 3) {
            gtk_widget_set_size_request(hub_pattern_viewer_widget, -1, num_patterns*15+100);
            gtkx_flush_events();
            width  = hub_pattern_viewer_widget->allocation.width;
            height = hub_pattern_viewer_widget->allocation.height;
            hub_pattern_viewer_repaint_dendrogram(cr, layout, xg, width, height);
        }

        g_object_unref(layout);
        cairo_destroy(cr);

        // Now copy the surface to the window:
        if ((hub_pattern_viewer_widget->window != NULL) && ((cr = gdk_cairo_create(hub_pattern_viewer_widget->window)) != NULL)) {
            cairo_set_source_surface(cr, hub_pattern_viewer_surface, 0, 0);
            cairo_paint(cr);
            cairo_destroy(cr);
        }
    }
    return(FALSE);
}

/******************************************************************************/

static void hub_pattern_show_mode_widgets()
{
    switch (hub_pattern_mode) {
    case 0: { // Vector list
        // Show the colour selector
        gtk_widget_show(hub_pattern_controls[0]);
        gtk_widget_show(hub_pattern_controls[1]);
        // Metric:
        gtk_widget_hide(hub_pattern_controls[2]);
        gtk_widget_hide(hub_pattern_controls[3]);
        // Hide the linkage selector
        gtk_widget_hide(hub_pattern_controls[4]);
        gtk_widget_hide(hub_pattern_controls[5]);
        // Similarity
        gtk_widget_hide(hub_pattern_controls[6]);
        gtk_widget_hide(hub_pattern_controls[7]);
        // Select which similarity matrix
        gtk_widget_hide(hub_pattern_controls[8]);
        gtk_widget_hide(hub_pattern_controls[9]);
        break;
    }
    case 1: { // Norms
        // Show the colour selector
        gtk_widget_hide(hub_pattern_controls[0]);
        gtk_widget_hide(hub_pattern_controls[1]);
        // Metric:
        gtk_widget_show(hub_pattern_controls[2]);
        gtk_widget_show(hub_pattern_controls[3]);
        // Hide the linkage selector
        gtk_widget_hide(hub_pattern_controls[4]);
        gtk_widget_hide(hub_pattern_controls[5]);
        // Similarity
#ifdef SIMILAR
        gtk_widget_show(hub_pattern_controls[6]);
        gtk_widget_show(hub_pattern_controls[7]);
#else
        gtk_widget_hide(hub_pattern_controls[6]);
        gtk_widget_hide(hub_pattern_controls[7]);
#endif
        // Select which similarity matrix
        gtk_widget_hide(hub_pattern_controls[8]);
        gtk_widget_hide(hub_pattern_controls[9]);
        break;
    }
    case 2: { // Similarity
        // Show the colour selector
        gtk_widget_show(hub_pattern_controls[0]);
        gtk_widget_show(hub_pattern_controls[1]);
        // Metric:
        gtk_widget_show(hub_pattern_controls[2]);
        gtk_widget_show(hub_pattern_controls[3]);
        // Hide the linkage selector
        gtk_widget_hide(hub_pattern_controls[4]);
        gtk_widget_hide(hub_pattern_controls[5]);
        // Similarity
        gtk_widget_show(hub_pattern_controls[6]);
        gtk_widget_show(hub_pattern_controls[7]);
        // Select which similarity matrix
        gtk_widget_show(hub_pattern_controls[8]);
        gtk_widget_show(hub_pattern_controls[9]);
        break;
    }
    case 3: { // Dendrogram
        // Show the colour selector
        gtk_widget_hide(hub_pattern_controls[0]);
        gtk_widget_hide(hub_pattern_controls[1]);
        // Metric:
        gtk_widget_show(hub_pattern_controls[2]);
        gtk_widget_show(hub_pattern_controls[3]);
        // Show the linkage selector
        gtk_widget_show(hub_pattern_controls[4]);
        gtk_widget_show(hub_pattern_controls[5]);
        // Similarity
        gtk_widget_hide(hub_pattern_controls[6]);
        gtk_widget_hide(hub_pattern_controls[7]);
        // Select which similarity matrix
        gtk_widget_hide(hub_pattern_controls[8]);
        gtk_widget_hide(hub_pattern_controls[9]);
        break;
    }
    }
}

/*----------------------------------------------------------------------------*/

static void callback_select_pattern_mode(GtkWidget *combobox, XGlobals *xg)
{
    hub_pattern_mode = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox));
    hub_pattern_show_mode_widgets();
    hub_pattern_viewer_repaint(NULL, NULL, xg);
}

/*----------------------------------------------------------------------------*/

static void callback_select_metric(GtkWidget *combobox, XGlobals *xg)
{
    hub_pattern_metric = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox));
    hub_pattern_viewer_repaint(NULL, NULL, xg);
}

/*----------------------------------------------------------------------------*/
static void callback_select_matrix_show(GtkWidget *combobox, XGlobals *xg)
{
    hub_pattern_matrix_show = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox));
    hub_pattern_viewer_repaint(NULL, NULL, xg);
}

/*----------------------------------------------------------------------------*/

static void callback_select_similarity(GtkWidget *combobox, XGlobals *xg)
{
    hub_pattern_similarity  = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox));
    hub_pattern_viewer_repaint(NULL, NULL, xg);
}

/*----------------------------------------------------------------------------*/

static void callback_select_linkage(GtkWidget *combobox, XGlobals *xg)
{
    hub_pattern_linkage = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox));
    hub_pattern_viewer_repaint(NULL, NULL, xg);
}

/*----------------------------------------------------------------------------*/

static void callback_toggle_colour(GtkWidget *caller, XGlobals *xg)
{
    hub_pattern_colour = (Boolean) (GTK_TOGGLE_BUTTON(caller)->active);
    hub_pattern_viewer_repaint(NULL, NULL, xg);
}

/*----------------------------------------------------------------------------*/

static void xhub_patterns_print_callback(GtkWidget *caller, XGlobals *xg)
{
    cairo_surface_t *surface = NULL;
    cairo_t *cr = NULL;
    PangoLayout *layout;
    char filename[128], prefix[64];
    int num_patterns = pattern_list_length(xg->pattern_set);
    int width = 0, height = 0;

    if (hub_pattern_mode == 0) {
        width = 1100;
        height = 60+num_patterns*14;
    }
    else if (hub_pattern_mode == 1) {
        width = 800;
        height = 600;
    }
    else if (hub_pattern_mode == 2) {
        height = 50+num_patterns*7;
        if (hub_pattern_matrix_show == 0) {
            width = height*2+100;
        }
        else {
            width = height+30;
        }
    }
    else if (hub_pattern_mode == 3) {
        width = num_patterns*15+100;
        height = 600;
    }

    g_snprintf(prefix, 64, "%s_%s", xg->pattern_set_name, print_file_prefix[hub_pattern_mode]);

    /* PDF version: */
    g_snprintf(filename, 128, "%s/%s.pdf", PRINT_FOLDER, prefix);
    surface = cairo_pdf_surface_create(filename, width, height);
    cr = cairo_create(surface);
    layout = pango_cairo_create_layout(cr);
    pangox_layout_set_font_face(layout, "Sans");
    pangox_layout_set_font_size(layout, 12);

    if (hub_pattern_mode == 0) {
        hub_pattern_viewer_repaint_vectors(cr, layout, xg);
    }
    else if (hub_pattern_mode == 1) {
        hub_pattern_viewer_repaint_norms(cr, layout, xg, width, height);
    }
    else if (hub_pattern_mode == 2) {
        hub_pattern_viewer_repaint_similarity(cr, layout, xg, width, height);
    }
    else if (hub_pattern_mode == 3) {
        hub_pattern_viewer_repaint_dendrogram(cr, layout, xg, width, height);
    }
    g_object_unref(layout);
    cairo_destroy(cr);

    /* Save the PNG while we're at it: */
    g_snprintf(filename, 128, "%s/%s.png", PRINT_FOLDER, prefix);
    cairo_surface_write_to_png(surface, filename);
    /* And destroy the surface */
    cairo_surface_destroy(surface);
}

/*----------------------------------------------------------------------------*/

void hub_pattern_set_widgets(GtkWidget *page, XGlobals *xg)
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
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Bar Charts");
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Vector Similarity");
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Dendrogram");
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), hub_pattern_mode);
    g_signal_connect(G_OBJECT(tmp), "changed", G_CALLBACK(callback_select_pattern_mode), xg);
    gtk_widget_show(tmp);

    /*--- Filler: ---*/
    tmp = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 0);
    gtk_widget_show(tmp);

    /* Widgets for selecting which matrices to show: */
    /*--- The label: ---*/
    tmp = gtk_label_new("  Show: ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
    hub_pattern_controls[8] = tmp;
    /*--- The combobox selector: ---*/
    tmp = gtk_combo_box_new_text();
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Both");
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Distance");
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Similarity");
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), hub_pattern_matrix_show);
    g_signal_connect(G_OBJECT(tmp), "changed", G_CALLBACK(callback_select_matrix_show), xg);
    gtk_widget_show(tmp);
    hub_pattern_controls[9] = tmp;

    /* Widgets for setting the metric: */
    /*--- The label: ---*/
    tmp = gtk_label_new("  Metric: ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
    hub_pattern_controls[2] = tmp;
    /*--- The combobox selector: ---*/
    tmp = gtk_combo_box_new_text();
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), metric_name[0]);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), metric_name[1]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), hub_pattern_metric);
    g_signal_connect(G_OBJECT(tmp), "changed", G_CALLBACK(callback_select_metric), xg);
    gtk_widget_show(tmp);
    hub_pattern_controls[3] = tmp;

    /* Widgets for setting cosine/correlation similarity: */
    /*--- The label: ---*/
    tmp = gtk_label_new("  Similarity: ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
    hub_pattern_controls[6] = tmp;
    /*--- The combobox selector: ---*/
    tmp = gtk_combo_box_new_text();
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), similarity_name[0]);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), similarity_name[1]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), hub_pattern_similarity);
    g_signal_connect(G_OBJECT(tmp), "changed", G_CALLBACK(callback_select_similarity), xg);
    gtk_widget_show(tmp);
    hub_pattern_controls[7] = tmp;

    /* Widgets for setting the linkage function: */
    /*--- The label: ---*/
    tmp = gtk_label_new("  Linkage: ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
    hub_pattern_controls[4] = tmp;
    /*--- The combobox selector: ---*/
    tmp = gtk_combo_box_new_text();
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), linkage_name[0]);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), linkage_name[1]);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), linkage_name[2]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), hub_pattern_linkage);
    g_signal_connect(G_OBJECT(tmp), "changed", G_CALLBACK(callback_select_linkage), xg);
    gtk_widget_show(tmp);
    hub_pattern_controls[5] = tmp;

    /*--- Filler: ---*/
    tmp = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 0);
    gtk_widget_show(tmp);

    /* Widgets for setting colour/greyscale: */
    /*--- The label: ---*/
    tmp = gtk_label_new("  Colour: ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
    hub_pattern_controls[0] = tmp;
    /*--- The checkbox: ---*/
    tmp = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), hub_pattern_colour);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(tmp), "toggled", G_CALLBACK(callback_toggle_colour), xg);
    gtk_widget_show(tmp);
    hub_pattern_controls[1] = tmp;

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
    hub_pattern_viewer_widget = gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(hub_pattern_viewer_widget), "expose_event", G_CALLBACK(hub_pattern_viewer_repaint), xg);
    g_signal_connect(G_OBJECT(hub_pattern_viewer_widget), "configure_event", G_CALLBACK(gtkx_configure_surface_callback), &hub_pattern_viewer_surface);
    g_signal_connect(G_OBJECT(hub_pattern_viewer_widget), "destroy", G_CALLBACK(gtkx_destroy_surface_callback), &hub_pattern_viewer_surface);
    gtk_widget_set_events(hub_pattern_viewer_widget, GDK_EXPOSURE_MASK);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_win), hub_pattern_viewer_widget);
    gtk_widget_show(hub_pattern_viewer_widget);

    // Set widgets for the current mode: 
    hub_pattern_show_mode_widgets();
    gtk_widget_show(page);
}

/******************************************************************************/
