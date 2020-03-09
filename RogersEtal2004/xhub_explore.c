/*******************************************************************************

    File:       xhub_explore.c
    Contents:   Widgets to support exploration of the network's behaviour
    Author:     Rick Cooper
    Created:    03/11/16
    Copyright (c) 2016 Richard P. Cooper

    Explore the network's behaviour, e.g., with naming on each pattern from the
    current pattern set, or by looking at the "depth" of attracors. The
    funtions are set up to test picture naming. To test anything else clamping
    should be adjusted to present and clamp the correct input prior to settling.

    Public procedures:
        void hub_explore_create_widgets(GtkWidget *page, XGlobals *xg)
        void hub_explore_initialise_network(XGlobals *xg)

*******************************************************************************/
/******** Include files: ******************************************************/

#include "xhub.h"
#include <string.h>
#include "lib_maths.h"
#include "lib_string.h"
#include "lib_cairox.h"
#include "lib_cairoxt_2_0.h"
#include "lib_cairoxg_2_2.h"
#include "lib_dendrogram_1_0.h"

// It would be good to get rid of this!
#define NUM_PATTERNS 48

// This is the number of virtual participants - used in the attractor
// density analysis
#define INDIVIDUALS 20

#define DOM_MAX 2

typedef enum explore_mode_type {
    EXPLORE_SETTLE_NAME_FROM_VISUAL, EXPLORE_TABULATE_FROM_VISUAL,
    EXPLORE_TABULATE_FROM_VERBAL, EXPLORE_TABULATE_FROM_NAME,
    EXPLORE_TABULATE_FROM_ALL, EXPLORE_WEIGHT_DISTRIBUTION,
    EXPLORE_ATTRACTOR_BAR_CHARTS, EXPLORE_ATTRACTOR_SIMILARITY,
    EXPLORE_ATTRACTOR_DENSITY, EXPLORE_ATTRACTOR_DENDROGRAMS
} ExploreModeType;

/* Local variables (globally accessible in this file): -----------------------*/
typedef enum similarity_type {
    SIMILARITY_COSINE, SIMILARITY_CORRELATION
} SimilarityType;

static GtkWidget        *explore_viewer_widget = NULL;
static cairo_surface_t  *explore_viewer_surface = NULL;
static Network          *explore_network = NULL;
static double            explore_disconnection_severity = 0.0;
static double            explore_noise_sd = 0.0;
// FIX ME: ADD INTERFACE FACILITIES TO ADJUST THE FOLLOWING TWO:
static double            explore_ablation_severity = 0.0;
static double            explore_weight_scaling = 1.0;
static Boolean           explore_slow = TRUE;
static Boolean           explore_colour = TRUE;
static ExploreModeType   explore_mode = EXPLORE_SETTLE_NAME_FROM_VISUAL;
static double            explore_attractor_similarity_distance[NUM_PATTERNS*NUM_PATTERNS];
static double            explore_attractor_similarity_correlation[NUM_PATTERNS*NUM_PATTERNS];
static double            explore_attractor_similarity_cosine[NUM_PATTERNS*NUM_PATTERNS];
static double            explore_attractor_density_by_category[INDIVIDUALS][CAT_MAX];
static double            explore_attractor_density_by_domain[INDIVIDUALS][DOM_MAX];

static GtkWidget        *explore_button[8];
static GtkWidget        *explore_widget[11];

static TableStruct      *explore_table_name_all = NULL;
static GraphStruct      *explore_graph_categories[2] = {NULL, NULL};
static GraphStruct      *explore_graph_domains[2] = {NULL, NULL};
static GraphStruct      *explore_weight_histogram[3] = {NULL, NULL, NULL};
static DendrogramStruct *explore_dendrogram = NULL;

static MetricType        explore_metric = METRIC_JACCARD;
static LinkageType       explore_linkage = LT_MEAN;
static SimilarityType    explore_similarity = SIMILARITY_CORRELATION;

static char *print_file_prefix[10] = 
    {"settled_network", "settle_from_visual_table", "settle_from_verbal_table",
     "settle_from_name_table", "settle_from_all_table", "weight_distribution",
     "attractor_length", "attractor_similarity", "attractor_density", 
     "attractor_dendrogram"};

static char *category_name[CAT_MAX] = 
    {"Birds", "Mammals", "Fruits", "Tools", "Vehicles", "HH Objects"};

static char *domain_name[DOM_MAX] = 
    {"Animals", "Artifacts"};

#undef SIMILAR

#ifdef SIMILAR
static char *similarity_name[2] = 
    {"Cosine", "Correlation"};
#endif

static char *metric_name[2] = 
    {"Euclidean", "Jaccard"};

static char *linkage_name[3] =
    {"Mean", "Maximum", "Minimum"};

#define MAX_BINS 21

typedef struct weight_statistics {
    int pos_count, neg_count, zero_count;
    double min, max, mean;
    double bin_x[MAX_BINS], bin_y[MAX_BINS];
} WeightStatistics;

/******************************************************************************/

// Initialise attractor similarity to -1 to indicate this is not known
static void explore_initialise_attractor_similarity()
{
    int i, j;

    for (i = 0; i < NUM_PATTERNS; i++) {
        for (j = 0; j < NUM_PATTERNS; j++) {
            explore_attractor_similarity_distance[j*NUM_PATTERNS+i] = -1.0;
            explore_attractor_similarity_correlation[j*NUM_PATTERNS+i] = -1.0;
            explore_attractor_similarity_cosine[j*NUM_PATTERNS+i] = -1.0;
        }
    }
}

/******************************************************************************/

static NamedVectorArray *retrieve_attractor_vectors(PatternList *patterns)
{
    PatternList *p;
    NamedVectorArray *list = NULL;
    ClampType clamp;
    int j = 0;
    int width;

    if (explore_network != NULL) {
        if ((list = (NamedVectorArray *)malloc(pattern_list_length(patterns) * sizeof(NamedVectorArray))) != NULL) {

            // How many hidden units?
            width = explore_network->hidden_width;

            for (p = patterns; p != NULL; p = p->next) {
                if ((list[j].vector = (double *)malloc(width * sizeof(double))) != NULL) {

                    // Run the network with the pattern to get the attractor, then
                    /* Set up the clamp: */
                    clamp_set_clamp_visual(&clamp, 0, 3 * explore_network->params.ticks, p);
                    /* Initialise units to random values etc: */
                    network_initialise(explore_network);
                    do {
                        network_tell_recirculate_input(explore_network, &clamp);
                        network_tell_propagate(explore_network);
                    } while (!network_is_settled(explore_network));

                    network_ask_hidden(explore_network, list[j].vector);
                    list[j].name = string_copy(p->name);
                    j++;
                }
            }
        }
    }
    return(list);
}

/*----------------------------------------------------------------------------*/

static void regenerate_attractor_dendrogram(XGlobals *xg)
{
    NamedVectorArray *list;

    if (explore_network == NULL) {
        hub_explore_initialise_network(xg);
    }

    if (explore_dendrogram != NULL) {
        dendrogram_free(explore_dendrogram);
    }

    if ((list = retrieve_attractor_vectors(xg->pattern_set)) != NULL) {
        // How many hidden units?
        int width = explore_network->hidden_width;
        explore_dendrogram = dendrogram_create(list, width, pattern_list_length(xg->pattern_set), explore_metric, explore_linkage);
    }
    named_vector_array_free(list, pattern_list_length(xg->pattern_set));
}

/******************************************************************************/

static void explore_viewer_repaint_naming(cairo_t *cr, XGlobals *xg)
{
    PangoLayout *layout;
    CairoxTextParameters tp;
    char buffer[128];
    int width = explore_viewer_widget->allocation.width;

    layout = pango_cairo_create_layout(cr);
    pangox_layout_set_font_size(layout, 12);

    // Draw the input pattern, if set:
    if (xg->current_pattern == NULL) {
        cairox_text_parameters_set(&tp, width*0.5, 20, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_CENTER, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, "No input pattern selected");
    }
    else {
        g_snprintf(buffer, 128, "Input Pattern is: %s", xg->current_pattern->name);
        cairox_text_parameters_set(&tp, 10, 10, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_CENTER, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);

        cairox_vector_draw(cr, width*0.10, 30, explore_colour, NUM_NAME, xg->current_pattern->name_features);
        cairox_centre_text(cr, width*0.10, 40, layout, "Name");

        cairox_vector_draw(cr, width*0.45, 30, explore_colour, NUM_VERBAL, xg->current_pattern->verbal_features);
        cairox_centre_text(cr, width*0.45, 40, layout, "Verbal");

        cairox_vector_draw(cr, width*0.85, 30, explore_colour, NUM_VISUAL, xg->current_pattern->visual_features);
        cairox_centre_text(cr, width*0.85, 40, layout, "Visual");
    }

    if (explore_network != NULL) {
        double vector_io[NUM_IO];
        double vector_hid[NUM_SEMANTIC];
        PatternList *best;

        if (explore_network->cycles == 0) {
            g_snprintf(buffer, 128, "Current state of network (unsettled):");
        }
        else {
            g_snprintf(buffer, 128, "Current state of network (cycle %d; %s):", explore_network->cycles, (explore_network->settled ? "settled" : "settling"));
        }
        cairox_text_parameters_set(&tp, 10, 100, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_CENTER, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);

        // Draw the network input pools:
        network_ask_input(explore_network, vector_io);
        cairox_vector_draw(cr, width*0.10, 120, explore_colour, NUM_NAME, &vector_io[0]);
        cairox_vector_draw(cr, width*0.45, 120, explore_colour, NUM_VERBAL, &vector_io[NUM_NAME]);
        cairox_vector_draw(cr, width*0.85, 120, explore_colour, NUM_VISUAL, &vector_io[NUM_NAME+NUM_VERBAL]);

        // Draw the network hidden layer:
        network_ask_hidden(explore_network, vector_hid);
        cairox_vector_draw(cr, width*0.50, 160, explore_colour, NUM_SEMANTIC, vector_hid);

        // Show the length of the hidden layer representation:
        g_snprintf(buffer, 128, "Size: %f", vector_length(NUM_SEMANTIC, vector_hid));
        cairox_text_parameters_set(&tp, width, 160, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_CENTER, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);

        // Draw the network output pools:
        network_ask_output(explore_network, vector_io);
        cairox_vector_draw(cr, width*0.10, 200, explore_colour, NUM_NAME, &vector_io[0]);
        cairox_vector_draw(cr, width*0.45, 200, explore_colour, NUM_VERBAL, &vector_io[NUM_NAME]);
        cairox_vector_draw(cr, width*0.85, 200, explore_colour, NUM_VISUAL, &vector_io[NUM_NAME+NUM_VERBAL]);

        // Draw the pattern with the most active name:
        if ((best = pattern_list_get_best_name_match(xg->pattern_set, vector_io)) == NULL) {
            cairox_text_parameters_set(&tp, width*0.5, 300, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_CENTER, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, "No pattern matches by name");
        }
        else {
            g_snprintf(buffer, 128, "Best matching training pattern ny name is %s (d = %6.4f):", best->name, pattern_get_distance(best, vector_io));
            cairox_text_parameters_set(&tp, 10, 280, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_CENTER, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, buffer);

            cairox_vector_draw(cr, width*0.10, 300, explore_colour, NUM_NAME, best->name_features);
            cairox_vector_draw(cr, width*0.45, 300, explore_colour, NUM_VERBAL, best->verbal_features);
            cairox_vector_draw(cr, width*0.85, 300, explore_colour, NUM_VISUAL, best->visual_features);
        }

        // Draw the closest match from the pattern set the network's I/O:
        if ((best = pattern_list_get_best_feature_match(xg->pattern_set, vector_io)) == NULL) {
            cairox_text_parameters_set(&tp, width*0.5, 350, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_CENTER, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, "No input pattern matches");
        }
        else {
            g_snprintf(buffer, 128, "Best matching training pattern by features is %s (d = %6.4f):", best->name, pattern_get_distance(best, vector_io));
            cairox_text_parameters_set(&tp, 10, 330, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_CENTER, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, buffer);
            
            cairox_vector_draw(cr, width*0.10, 350, explore_colour, NUM_NAME, best->name_features);
            cairox_vector_draw(cr, width*0.45, 350, explore_colour, NUM_VERBAL, best->verbal_features);
            cairox_vector_draw(cr, width*0.85, 350, explore_colour, NUM_VISUAL, best->visual_features);
        }
    }

    g_object_unref(layout);
}

/*----------------------------------------------------------------------------*/

static void explore_viewer_repaint_name_table(cairo_t *cr, XGlobals *xg)
{
    int width  = explore_viewer_widget->allocation.width;
    int height = explore_viewer_widget->allocation.height;

    if (explore_table_name_all != NULL) {
        table_set_extent(explore_table_name_all, 0, 0, width, height);
        cairox_draw_table(cr, explore_table_name_all);
    }
}

/*----------------------------------------------------------------------------*/

static void explore_viewer_repaint_attractor_length(cairo_t *cr, XGlobals *xg)
{
    int width  = explore_viewer_widget->allocation.width;
    int height = explore_viewer_widget->allocation.height;
    char buffer[128];

    if (explore_graph_categories[0] != NULL) {
        graph_set_extent(explore_graph_categories[0], 0, 0, width*0.5, height*0.5);
        g_snprintf(buffer, 128, "Mean Attractor Vector Length (%s)", xg->pattern_set_name);
        graph_set_title(explore_graph_categories[0], buffer);
        cairox_draw_graph(cr, explore_graph_categories[0], explore_colour);
    }
    if (explore_graph_categories[1] != NULL) {
        graph_set_extent(explore_graph_categories[1], width*0.5, 0, width*0.5, height*0.5);
#ifdef SIMILAR
        g_snprintf(buffer, 128, "Within-Category Attractor Similarity (%s)", xg->pattern_set_name);
#else
        g_snprintf(buffer, 128, "Within-Category Attractor Distance (%s)", xg->pattern_set_name);
#endif
        graph_set_title(explore_graph_categories[1], buffer);
        cairox_draw_graph(cr, explore_graph_categories[1], explore_colour);
    }
    if (explore_graph_domains[0] != NULL) {
        graph_set_extent(explore_graph_domains[0], 0, height*0.5, width*0.5, height*0.5);
        g_snprintf(buffer, 128, "Mean Attractor Vector Length (%s)", xg->pattern_set_name);
        graph_set_title(explore_graph_domains[0], buffer);
        cairox_draw_graph(cr, explore_graph_domains[0], explore_colour);
    }
    if (explore_graph_domains[1] != NULL) {
        graph_set_extent(explore_graph_domains[1], width*0.5, height*0.5, width*0.5, height*0.5);
#ifdef SIMILAR
        g_snprintf(buffer, 128, "Within-Domain Attractor Similarity (%s)", xg->pattern_set_name);
#else
        g_snprintf(buffer, 128, "Within-Domain Attractor Distance (%s)", xg->pattern_set_name);
#endif
        graph_set_title(explore_graph_domains[1], buffer);
        cairox_draw_graph(cr, explore_graph_domains[1], explore_colour);
    }
}

/******************************************************************************/

static void generate_attractor_states(Network *my_net, PatternList *patterns, double attractor[NUM_PATTERNS][NUM_SEMANTIC])
{
    PatternList *p;
    ClampType clamp;
    int j;

    for (p = patterns, j = 0; p != NULL; p = p->next, j++) {
        /* Set up the clamp: */
        clamp_set_clamp_visual(&clamp, 0, 3 * my_net->params.ticks, p);
        /* Initialise units to random values etc: */
        network_initialise(my_net);
        do {
            network_tell_recirculate_input(my_net, &clamp);
            network_tell_propagate(my_net);
        } while (!network_is_settled(my_net));

        network_ask_hidden(my_net, attractor[j]);
    }
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

/*----------------------------------------------------------------------------*/

static void get_category_similarity(PatternList *patterns,  int c, double attractor[NUM_PATTERNS][NUM_SEMANTIC], double *mean, double *se)
{
    PatternList *p1, *p2;
    int n = 0, i, j;
    double sum = 0.0;
    double ssq = 0.0;
    double d = 0.0;

    for (p1 = patterns, i = 0; p1 != NULL; p1 = p1->next, i++) {
        if (p1->category == c) {
            for (p2 = patterns, j = 0; p2 != NULL; p2 = p2->next, j++) {
                if ((p2->category == c) && (p1 != p2)) {
#ifdef SIMILAR
                    if (explore_similarity == SIMILARITY_COSINE) {
                        // Similarity as measured by cosine:
                        d = vector_cosine(NUM_SEMANTIC, attractor[i], attractor[j]);
                    }
                    else if (explore_similarity == SIMILARITY_CORRELATION) { 
                        // Similarity as measured by correlation:
                        d = vector_correlation(NUM_SEMANTIC, attractor[i], attractor[j]);
                    }
#else
                    d = euclidean_distance(NUM_SEMANTIC, attractor[i], attractor[j]);
#endif
                    sum += d;
                    ssq += d*d;
                    n++;
                }
            }
        }
    }

    *mean = (n > 0) ? (sum / (double) n) : 0.0;
    *se = (n > 1) ? sqrt((ssq - (sum*sum / (double) (n)))/((double) n-1)) / sqrt(n) : 0.0;

}

/*----------------------------------------------------------------------------*/

static void get_domain_similarity(PatternList *patterns,  int c, double attractor[NUM_PATTERNS][NUM_SEMANTIC], double *mean, double *se)
{
    PatternList *p1, *p2;
    int n = 0;
    double sum = 0.0;
    double ssq = 0.0;
    double d = 0.0;
    int i, j;

    for (p1 = patterns, i = 0; p1 != NULL; p1 = p1->next, i++) {
        if (pattern_is_in_domain(p1, c)) {
            for (p2 = patterns, j = 0; p2 != NULL; p2 = p2->next, j++) {
                if (pattern_is_in_domain(p2, c) && (p1 != p2)) {
#ifdef SIMILAR
                    if (explore_similarity == SIMILARITY_COSINE) {
                        // Similarity as measured by cosine:
                        d = vector_cosine(NUM_SEMANTIC, attractor[i], attractor[j]);
                    }
                    else if (explore_similarity == SIMILARITY_CORRELATION) { 
                        // Similarity as measured by correlation:
                        d = vector_correlation(NUM_SEMANTIC, attractor[i], attractor[j]);
                    }
#else
                    d = euclidean_distance(NUM_SEMANTIC, attractor[i], attractor[j]);
#endif
                    sum += d;
                    ssq += d*d;
                    n++;
                }
            }
        }
    }
    *mean = (n > 0) ? (sum / (double) n) : 0.0;
    *se = (n > 1) ? sqrt((ssq - (sum*sum / (double) (n)))/((double) n-1)) / sqrt(n) : 0.0;
}

/*----------------------------------------------------------------------------*/

static void explore_attractor_density_initialise()
{
    int j, c;
    for (j = 0; j < INDIVIDUALS; j++) {
        for (c = 0; c < CAT_MAX; c++) {
            explore_attractor_density_by_category[j][c] = -1.0;
        }
        for (c = 0; c < DOM_MAX; c++) {
            explore_attractor_density_by_domain[j][c] = -1.0;
        }
    }
}

/*----------------------------------------------------------------------------*/

static void explore_attractor_density_regenerate(XGlobals *xg)
{
    char filename[128];
    double attractor[NUM_PATTERNS][NUM_SEMANTIC];
    char *damage_graph_folder_name;
    int j, c;

    // The folder is the active value of explore_widget[10], which is a
    // combobox selector:
    damage_graph_folder_name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(explore_widget[10]));

    fprintf(stdout, "%s\n", damage_graph_folder_name);

    // Load the pattern set from the selected folder
    if (pattern_file_load_from_folder(xg, damage_graph_folder_name)) {
        // For files 1 to 20: calculate the statistic and show the result
        for (j = 0; j < INDIVIDUALS; j++) {
            // Load weights:
            g_snprintf(filename, 128, "DataFiles/%s/%02d.wgt", damage_graph_folder_name, j+1);
            weight_file_read(xg, filename);

            // Collect the attractor states:
            generate_attractor_states(xg->net, xg->pattern_set, attractor);
            
            for (c = 0; c < CAT_MAX; c++) {
                double mean, se;

                get_category_similarity(xg->pattern_set, c, attractor, &mean, &se);
                explore_attractor_density_by_category[j][c] = mean;
            }

            for (c = 0; c < DOM_MAX; c++) {
                double mean, se;

                get_domain_similarity(xg->pattern_set, c, attractor, &mean, &se);
                explore_attractor_density_by_domain[j][c] = mean;
            }

            fprintf(stdout, "%f\t%f\n", explore_attractor_density_by_domain[j][0], explore_attractor_density_by_domain[j][1]);

        }
    }
    else {
        gtkx_warn(xg->frame, "Failed to locate pattern file in selected folder");
    }
    free(damage_graph_folder_name);
}

/*----------------------------------------------------------------------------*/

static void explore_viewer_repaint_attractor_density(cairo_t *cr, XGlobals *xg)
{
    char buffer[128];
    PangoLayout *layout;
    CairoxTextParameters tp;
    CairoxLineParameters lp;
    int width  = explore_viewer_widget->allocation.width;
    int j, c;

    layout = pango_cairo_create_layout(cr);
    pangox_layout_set_font_size(layout, 18);

#ifdef SIMILAR
    g_snprintf(buffer, 128, "Mean pairwise %s similarity of attractors", similarity_name[explore_similarity]);
#else
    g_snprintf(buffer, 128, "Mean pairwise (Euclidean) distance between attractors (within-category and within-domain");
#endif
    cairox_text_parameters_set(&tp, width*0.5, 20, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
    cairox_paint_pango_text(cr, &tp, layout, buffer);

    pangox_layout_set_font_size(layout, 16);

    // Number the rows:
    for (j = 0; j < INDIVIDUALS; j++) {
        g_snprintf(buffer, 128, "%2d", j+1);
        cairox_text_parameters_set(&tp, 30, j*20+60, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);
    }

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairox_line_parameters_set(&lp, 1.0, LS_SOLID, FALSE);

    // Now one column for each category:
    for (c = 0; c < CAT_MAX; c++) {
        double sum = 0;

        cairox_text_parameters_set(&tp, 80+c*90, 40, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, category_name[c]);

        for (j = 0; j < INDIVIDUALS; j++) {
            g_snprintf(buffer, 128, "%7.4f", explore_attractor_density_by_category[j][c]);
            cairox_text_parameters_set(&tp, 100 + c*90, j*20+60, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_BOTTOM, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, buffer);
            sum += explore_attractor_density_by_category[j][c];
        }

        cairox_paint_line(cr, &lp, 55+c*90, 43, 105+c*90, 43);
        cairox_paint_line(cr, &lp, 55+c*90, 20*20+43, 105+c*90, 20*20+43);

        g_snprintf(buffer, 128, "%7.4f", sum / (double) INDIVIDUALS);
        cairox_text_parameters_set(&tp, 100 + c*90, 20*20+60, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);
    }

    // And one column for each domain:
    for (c = 0; c < DOM_MAX; c++) {
        double sum = 0;

        cairox_text_parameters_set(&tp, 680+c*90, 40, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, domain_name[c]);

        for (j = 0; j < INDIVIDUALS; j++) {
            g_snprintf(buffer, 128, "%7.4f", explore_attractor_density_by_domain[j][c]);
            cairox_text_parameters_set(&tp, 700 + c*90, j*20+60, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_BOTTOM, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, buffer);
            sum += explore_attractor_density_by_domain[j][c];
        }

        cairox_paint_line(cr, &lp, 655+c*90, 43, 705+c*90, 43);
        cairox_paint_line(cr, &lp, 655+c*90, 20*20+43, 705+c*90, 20*20+43);

        g_snprintf(buffer, 128, "%7.4f", sum / (double) INDIVIDUALS);
        cairox_text_parameters_set(&tp, 700 + c*90, 20*20+60, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);
    }

    // Do a repeated measures t-test on the domain data:

    double diff, sd, t;

    diff = 0.0;
    for (j = 0; j < INDIVIDUALS; j++) {
        diff += (explore_attractor_density_by_domain[j][1] - explore_attractor_density_by_domain[j][0]);
    }

    diff = diff / (double) INDIVIDUALS;
    sd = 0.0;
    for (j = 0; j < INDIVIDUALS; j++) {
        sd += squared((explore_attractor_density_by_domain[j][1] - explore_attractor_density_by_domain[j][0]) - diff);
    }
    sd = sd / (double) (INDIVIDUALS-1);
    sd = sqrt(sd / (double) INDIVIDUALS);

    t = diff / sd;

    g_snprintf(buffer, 128, "t(%d) = %6.3f", (INDIVIDUALS-1), t);
    cairox_text_parameters_set(&tp, 730, 480, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
    cairox_paint_pango_text(cr, &tp, layout, buffer);

    g_object_unref(layout);
}

/******************************************************************************/

static void explore_viewer_repaint_attractor_dendrogram(cairo_t *cr, XGlobals *xg)
{

    if (explore_dendrogram == NULL) {
        regenerate_attractor_dendrogram(xg);
    }

    if (explore_dendrogram != NULL) {
        CairoxFontProperties *fp;
        int height  = explore_viewer_widget->allocation.height;
        int width  = explore_viewer_widget->allocation.width;

        // X coordinate to centre the dendrogram:
        int x = (width - NUM_PATTERNS*15) / 2;

        fp = font_properties_create("Sans", 14, PANGO_STYLE_NORMAL, PANGO_WEIGHT_NORMAL, "black");
        dendrogram_draw(cr, explore_dendrogram, fp, x, 20, height);
        font_properties_destroy(fp);
    }
}

/******************************************************************************/

static WeightStatistics *calculate_weight_statistics(Network *net, int k)
{
    WeightStatistics *ws;

    if ((ws = (WeightStatistics *)malloc(sizeof(WeightStatistics))) != NULL) {
        int i, j, lim_i = 0, lim_j = 0, b;
        double *weights = NULL, w;

        ws->pos_count = 0;
        ws->neg_count = 0;
        ws->zero_count = 0;
        ws->min = -1;
        ws->max = +1;
        ws->mean = 0;

        for (b = 0; b < MAX_BINS; b++) {
            ws->bin_x[b] = -2.5 + 5.0 * (b+0.5)/(double) MAX_BINS;
            ws->bin_y[b] = 0.0;
        }

        if (k == 0) { // Visible to Hidden
            weights = net->weights_ih;
            lim_i = net->in_width;
            lim_j = net->hidden_width;
        }

        if (k == 1) { // Hidden to Hidden
            weights = net->weights_hh;
            lim_i = net->hidden_width;
            lim_j = net->hidden_width;
        }

        if (k == 2) { // Hidden to Visible
            weights = net->weights_ho;
            lim_i = net->hidden_width;
            lim_j = net->out_width;
        }

        if (weights != NULL) {
            ws->min = weights[0];
            ws->max = weights[0];
            ws->mean = 0;

            for (i = 0; i < lim_i; i++) {
                for (j = 0; j < lim_j; j++) {
                    w = weights[i * lim_j + j];

                    if (w > 0) {
                        ws->pos_count++;
                    }
                    else if (w < 0) {
                        ws->neg_count++;
                    }
                    else {
                        ws->zero_count++;
                    }
                    ws->mean += w; // ws_mean is actually the sum of the weights
                    if (ws->min > w) {
                        ws->min = w;
                    }
                    if (ws->max < w) {
                        ws->max = w;
                    }

                    /* Histograph data: */
                    b = (int) ((w + 2.5) * MAX_BINS / 5.0);
                    if (b < 0) {
                        b = 0;
                    }
                    else if (b >= MAX_BINS) {
                        b = MAX_BINS-1;
                    }
                    ws->bin_y[b]++;
                }
            }
            ws->mean /= (double) (lim_i * lim_j); // divide by number of weights
            for (b = 0; b < MAX_BINS; b++) {
                ws->bin_y[b] /= (double) (lim_i * lim_j); 
            }
        }
    }
    return(ws);
}

void ftabulate_weight_statistics(FILE *fp, Network *net)
{
    if (fp != NULL) {
        WeightStatistics *ws;
        int k;

        for (k = 0; k < 3; k++) {
            if ((ws = calculate_weight_statistics(net, k)) != NULL) {
                int total = ws->neg_count + ws->zero_count + ws->pos_count;
                fprintf(fp, "%d\t%f\t%f\t%f\t%f\t%f\n", k, ws->neg_count / (double) total, ws->pos_count / (double) total, ws->min, ws->max, ws->mean);
                free(ws);
            }
        }
    }
}

static void cairo_draw_weight_statistics(cairo_t *cr, PangoLayout *layout, WeightStatistics *ws, int y, char *header)
{
    CairoxTextParameters tp;
    char buffer[128];
    int total;

    pangox_layout_set_font_size(layout, 14);

    total = ws->pos_count+ws->neg_count+ws->zero_count;

    g_snprintf(buffer, 128, "%s (N = %d):", header, total);
    cairox_text_parameters_set(&tp, 50, y, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_TOP, 0.0);
    cairox_paint_pango_text(cr, &tp, layout, buffer);

    cairox_text_parameters_set(&tp,  50, y+25, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_TOP, 0.0);
    cairox_paint_pango_text(cr, &tp, layout, "Positive:");
    cairox_text_parameters_set(&tp, 150, y+25, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_TOP, 0.0);
    g_snprintf(buffer, 128, "%5.3f", ws->pos_count / (double) total);
    cairox_paint_pango_text(cr, &tp, layout, buffer);

    cairox_text_parameters_set(&tp, 170, y+25, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_TOP, 0.0);
    cairox_paint_pango_text(cr, &tp, layout, "Negative:");
    cairox_text_parameters_set(&tp, 270, y+25, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_TOP, 0.0);
    g_snprintf(buffer, 128, "%5.3f", ws->neg_count / (double) total);
    cairox_paint_pango_text(cr, &tp, layout, buffer);

    cairox_text_parameters_set(&tp, 290, y+25, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_TOP, 0.0);
    cairox_paint_pango_text(cr, &tp, layout, "Zero:");
    cairox_text_parameters_set(&tp, 390, y+25, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_TOP, 0.0);
    g_snprintf(buffer, 128, "%5.3f", ws->zero_count / (double) total);
    cairox_paint_pango_text(cr, &tp, layout, buffer);

    cairox_text_parameters_set(&tp,  50, y+50, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_TOP, 0.0);
    cairox_paint_pango_text(cr, &tp, layout, "Min:");
    cairox_text_parameters_set(&tp, 150, y+50, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_TOP, 0.0);
    g_snprintf(buffer, 128, "%7.4f", ws->min);
    cairox_paint_pango_text(cr, &tp, layout, buffer);

    cairox_text_parameters_set(&tp, 170, y+50, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_TOP, 0.0);
    cairox_paint_pango_text(cr, &tp, layout, "Max:");
    cairox_text_parameters_set(&tp, 270, y+50, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_TOP, 0.0);
    g_snprintf(buffer, 128, "%7.4f", ws->max);
    cairox_paint_pango_text(cr, &tp, layout, buffer);

    cairox_text_parameters_set(&tp, 290, y+50, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_TOP, 0.0);
    cairox_paint_pango_text(cr, &tp, layout, "Mean:");
    cairox_text_parameters_set(&tp, 390, y+50, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_TOP, 0.0);
    g_snprintf(buffer, 128, "%7.4f", ws->mean);
    cairox_paint_pango_text(cr, &tp, layout, buffer);
}

static void explore_viewer_repaint_weight_distribution(cairo_t *cr, XGlobals *xg)
{
    PangoLayout *layout;
    CairoxTextParameters tp;
    WeightStatistics *ws;
    char *headings[3] = {"Visible to Hidden", "Hidden to Hidden", "Hidden to Visible"};
    char buffer[64];
    int width = explore_viewer_widget->allocation.width;
    int k, b;

    layout = pango_cairo_create_layout(cr);
    pangox_layout_set_font_size(layout, 16);

    g_snprintf(buffer, 64, "Weight Distribution (excluding bias weights)");
    cairox_text_parameters_set(&tp, 50, 20, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_TOP, 0.0);
    cairox_paint_pango_text(cr, &tp, layout, buffer);

    for (k = 0; k < 3; k++) {
        if ((ws = calculate_weight_statistics(explore_network, k)) != NULL) {
            cairo_draw_weight_statistics(cr, layout, ws, 50 + k*175, headings[k]);
            if (explore_weight_histogram[k] != NULL) {
                /* Set the graph position: */
                graph_set_extent(explore_weight_histogram[k], 400, k*175, width-420, 175);
                /* Load the histogram data: */
                for (b = 0; b < MAX_BINS; b++) {
                    explore_weight_histogram[k]->dataset[0].x[b] = ws->bin_x[b];
                    explore_weight_histogram[k]->dataset[0].y[b] = ws->bin_y[b];
                    explore_weight_histogram[k]->dataset[0].se[b] = 0.0;
                }
                explore_weight_histogram[k]->dataset[0].points = MAX_BINS;
                /* Draw the graph: */
                cairox_draw_graph(cr, explore_weight_histogram[k], explore_colour);
            }
            free(ws);
        }
    }

    g_object_unref(layout);
}

/******************************************************************************/

static void explore_viewer_repaint_attractor_similarity_matrix(cairo_t *cr, XGlobals *xg)
{
    PangoLayout *layout;
    CairoxScaleParameters ss = {};
    CairoxTextParameters tp;
    int width  = explore_viewer_widget->allocation.width;
    double x, y, dx, dy;
    char buffer[64];

    layout = pango_cairo_create_layout(cr);
    pangox_layout_set_font_size(layout, 14);
    ss.fmt = NULL;

    // Distance
    dx = 6.0; dy = 6.0;
    x = width * 0.5 - NUM_PATTERNS * dx - 40; y = 20.0;

    cairox_pixel_matrix_draw(cr, x, y, dx, dy, explore_attractor_similarity_distance, NUM_PATTERNS, explore_colour);

    pangox_layout_set_font_size(layout, 14);
    g_snprintf(buffer, 64, "Attractor  %s Distance Matrix (%s)", metric_name[explore_metric], xg->pattern_set_name);
    cairox_text_parameters_set(&tp, width*0.5-NUM_PATTERNS*dx*0.5-40, 30+NUM_PATTERNS*dy, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_TOP, 0.0);
    cairox_paint_pango_text(cr, &tp, layout, buffer);
    if (explore_metric == METRIC_JACCARD) {
        cairox_scale_parameters_set(&ss, GTK_POS_LEFT, 15, NUM_PATTERNS*dy, 0.0, 1.0, 5, "%4.1f");
    }
    else if (explore_metric == METRIC_EUCLIDEAN) {
        cairox_scale_parameters_set(&ss, GTK_POS_LEFT, 15, NUM_PATTERNS*dy, 0.0, 8.0, 5, "%4.1f");
    }
    cairox_scale_draw(cr, layout, &ss, x-30, y, explore_colour);

    // Correlation

    x = width * 0.5 + 40; y = 20.0;

    if (explore_similarity == SIMILARITY_COSINE) {
        cairox_pixel_matrix_draw(cr, x, y, dx, dy, explore_attractor_similarity_cosine, NUM_PATTERNS, explore_colour);
    }
    else {
        cairox_pixel_matrix_draw(cr, x, y, dx, dy, explore_attractor_similarity_correlation, NUM_PATTERNS, explore_colour);
    }

    pangox_layout_set_font_size(layout, 14);
    if (explore_similarity == SIMILARITY_COSINE) {
        g_snprintf(buffer, 64, "Attractor Cosine Matrix (%s)", xg->pattern_set_name);
    }
    else {
        g_snprintf(buffer, 64, "Attractor Correlation  Matrix (%s)", xg->pattern_set_name);
    }
    cairox_text_parameters_set(&tp, width*0.5+NUM_PATTERNS*dx*0.5+40, 30+NUM_PATTERNS*dy, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_TOP, 0.0);
    cairox_paint_pango_text(cr, &tp, layout, buffer);

    cairox_scale_parameters_set(&ss, GTK_POS_RIGHT, 15, NUM_PATTERNS*dy, -1.0, 1.0, 4, "%+4.1f");
    cairox_scale_draw(cr, layout, &ss, x+NUM_PATTERNS*dy+15, y, explore_colour);

    g_free(ss.fmt);
    g_object_unref(layout);
}

/******************************************************************************/

static Boolean explore_viewer_repaint(GtkWidget *caller, GdkEvent *event, XGlobals *xg)
{

    if (!GTK_WIDGET_MAPPED(explore_viewer_widget) || (explore_viewer_surface == NULL)) {
        return(TRUE);
    }
    else {
        cairo_t *cr;

        cr = cairo_create(explore_viewer_surface);

        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_paint(cr);

        if (explore_mode == EXPLORE_SETTLE_NAME_FROM_VISUAL) {
            gtk_widget_set_size_request(explore_viewer_widget, -1, -1);
            gtkx_flush_events();
            explore_viewer_repaint_naming(cr, xg);
        }
        else if (explore_mode == EXPLORE_TABULATE_FROM_VISUAL) {
            gtk_widget_set_size_request(explore_viewer_widget, -1, 65+NUM_PATTERNS*20);
            gtkx_flush_events();
            explore_viewer_repaint_name_table(cr, xg);
        }
        else if (explore_mode == EXPLORE_TABULATE_FROM_VERBAL) {
            gtk_widget_set_size_request(explore_viewer_widget, -1, 65+NUM_PATTERNS*20);
            gtkx_flush_events();
            explore_viewer_repaint_name_table(cr, xg);
        }
        else if (explore_mode == EXPLORE_TABULATE_FROM_NAME) {
            gtk_widget_set_size_request(explore_viewer_widget, -1, 65+NUM_PATTERNS*20);
            gtkx_flush_events();
            explore_viewer_repaint_name_table(cr, xg);
        }
        else if (explore_mode == EXPLORE_TABULATE_FROM_ALL) {
            gtk_widget_set_size_request(explore_viewer_widget, -1, 65+NUM_PATTERNS*20);
            gtkx_flush_events();
            explore_viewer_repaint_name_table(cr, xg);
        }
        else if (explore_mode == EXPLORE_WEIGHT_DISTRIBUTION) {
            gtk_widget_set_size_request(explore_viewer_widget, -1, -1);
            gtkx_flush_events();
            explore_viewer_repaint_weight_distribution(cr, xg);
        }
        else if (explore_mode == EXPLORE_ATTRACTOR_BAR_CHARTS) {
            gtk_widget_set_size_request(explore_viewer_widget, -1, -1);
            gtkx_flush_events();
            explore_viewer_repaint_attractor_length(cr, xg);
        }
        else if (explore_mode == EXPLORE_ATTRACTOR_SIMILARITY) {
            gtk_widget_set_size_request(explore_viewer_widget, -1, -1);
            gtkx_flush_events();
            explore_viewer_repaint_attractor_similarity_matrix(cr, xg);
        }
        else if (explore_mode == EXPLORE_ATTRACTOR_DENSITY) {
            explore_viewer_repaint_attractor_density(cr, xg);
        }
        else if (explore_mode == EXPLORE_ATTRACTOR_DENDROGRAMS) {
            gtk_widget_set_size_request(explore_viewer_widget, -1, NUM_PATTERNS*15);
            gtkx_flush_events();
            explore_viewer_repaint_attractor_dendrogram(cr, xg);
        }
        cairo_destroy(cr);

        /* Now copy the surface to the window: */
        if ((explore_viewer_widget->window != NULL) && ((cr = gdk_cairo_create(explore_viewer_widget->window)) != NULL)) {
            cairo_set_source_surface(cr, explore_viewer_surface, 0, 0);
            cairo_paint(cr);
            cairo_destroy(cr);
        }
    }
    return(FALSE);
}

/******************************************************************************/

static void explore_mode_activate_buttons()
{
    switch (explore_mode) {
    case EXPLORE_SETTLE_NAME_FROM_VISUAL: {
        gtk_widget_show(explore_button[0]);
        gtk_widget_show(explore_button[1]);
        gtk_widget_show(explore_button[2]);
        gtk_widget_hide(explore_button[3]);
        gtk_widget_hide(explore_button[4]);
        gtk_widget_hide(explore_button[5]);
        gtk_widget_hide(explore_button[6]);
        gtk_widget_hide(explore_button[7]);
        // Turn the "Slow" controls on:
        gtk_widget_show(explore_widget[0]);
        gtk_widget_show(explore_widget[1]);
        // Turn the dendrogram controls off:
        gtk_widget_hide(explore_widget[2]);
        gtk_widget_hide(explore_widget[3]);
        gtk_widget_hide(explore_widget[4]);
        gtk_widget_hide(explore_widget[5]);
        gtk_widget_hide(explore_widget[6]);
        gtk_widget_show(explore_widget[7]); // Colour label
        gtk_widget_show(explore_widget[8]); // Colour checkbox
        gtk_widget_show(explore_widget[9]); // Density folder label
        gtk_widget_show(explore_widget[10]); // Density folder selector
        gtk_widget_hide(explore_widget[9]); // Density folder label
        gtk_widget_hide(explore_widget[10]); // Density folder selector
        break;
    }
    case EXPLORE_TABULATE_FROM_VISUAL:
    case EXPLORE_TABULATE_FROM_VERBAL:
    case EXPLORE_TABULATE_FROM_NAME:
    case EXPLORE_TABULATE_FROM_ALL: {
        gtk_widget_hide(explore_button[0]);
        gtk_widget_hide(explore_button[1]);
        gtk_widget_hide(explore_button[2]);
        gtk_widget_show(explore_button[3]);
        gtk_widget_hide(explore_button[4]);
        gtk_widget_hide(explore_button[5]);
        gtk_widget_hide(explore_button[6]);
        gtk_widget_hide(explore_button[7]);
        // Turn the "Slow" controls off:
        gtk_widget_hide(explore_widget[0]);
        gtk_widget_hide(explore_widget[1]);
        // Turn the dendrogram controls off:
        gtk_widget_hide(explore_widget[2]);
        gtk_widget_hide(explore_widget[3]);
        gtk_widget_hide(explore_widget[4]);
        gtk_widget_hide(explore_widget[5]);
        gtk_widget_hide(explore_widget[6]);
        gtk_widget_show(explore_widget[7]); // Colour label
        gtk_widget_show(explore_widget[8]); // Colour checkbox
        gtk_widget_hide(explore_widget[9]); // Density folder label
        gtk_widget_hide(explore_widget[10]); // Density folder selector
        break;
    }
    case EXPLORE_WEIGHT_DISTRIBUTION: {
        gtk_widget_hide(explore_button[0]); // First pattern
        gtk_widget_hide(explore_button[1]); // Step forward through patterns
        gtk_widget_hide(explore_button[2]); // Execute / Settle
        gtk_widget_hide(explore_button[3]); // Refresh test name all
        gtk_widget_hide(explore_button[4]); // Refresh test attractors
        gtk_widget_hide(explore_button[5]); // Refresh test attractor sim mat
        gtk_widget_hide(explore_button[6]); // Refresh attractor density
        gtk_widget_hide(explore_button[7]); // Refresh attractor dendrogram
        gtk_widget_hide(explore_widget[0]); // "Slow" label
        gtk_widget_hide(explore_widget[1]); // "Slow" checkbox
        gtk_widget_hide(explore_widget[2]); // Dendrogram metric label
        gtk_widget_hide(explore_widget[3]); // Dendrogram metric selector
        gtk_widget_hide(explore_widget[4]); // Dendrogram linkage label
        gtk_widget_hide(explore_widget[5]); // Dendrogram linkage selector
        gtk_widget_show(explore_widget[6]); // Filler
        gtk_widget_show(explore_widget[7]); // Colour label
        gtk_widget_show(explore_widget[8]); // Colour checkbox
        gtk_widget_hide(explore_widget[9]); // Density folder label
        gtk_widget_hide(explore_widget[10]); // Density folder selector
        break;
    }
    case EXPLORE_ATTRACTOR_BAR_CHARTS: {
        gtk_widget_hide(explore_button[0]);
        gtk_widget_hide(explore_button[1]);
        gtk_widget_hide(explore_button[2]);
        gtk_widget_hide(explore_button[3]);
        gtk_widget_show(explore_button[4]);
        gtk_widget_hide(explore_button[5]);
        gtk_widget_hide(explore_button[6]);
        gtk_widget_hide(explore_button[7]);
        // Turn the "Slow" controls off:
        gtk_widget_hide(explore_widget[0]);
        gtk_widget_hide(explore_widget[1]);
        // Turn the dendrogram controls off:
        gtk_widget_hide(explore_widget[2]);
        gtk_widget_hide(explore_widget[3]);
        gtk_widget_hide(explore_widget[4]);
        gtk_widget_hide(explore_widget[5]);
        gtk_widget_hide(explore_widget[6]); // Filler
        gtk_widget_hide(explore_widget[7]); // Colour label
        gtk_widget_hide(explore_widget[8]); // Colour checkbox
        gtk_widget_hide(explore_widget[9]); // Density folder label
        gtk_widget_hide(explore_widget[10]); // Density folder selector
        break;
    }
    case EXPLORE_ATTRACTOR_SIMILARITY: {
        gtk_widget_hide(explore_button[0]);
        gtk_widget_hide(explore_button[1]);
        gtk_widget_hide(explore_button[2]);
        gtk_widget_hide(explore_button[3]);
        gtk_widget_hide(explore_button[4]);
        gtk_widget_show(explore_button[5]);
        gtk_widget_hide(explore_button[6]);
        gtk_widget_hide(explore_button[7]);
        gtk_widget_hide(explore_widget[0]); // Slow label
        gtk_widget_hide(explore_widget[1]); // Slow checkbutton
        gtk_widget_show(explore_widget[2]); // Metric label
        gtk_widget_show(explore_widget[3]); // Metric selector
        gtk_widget_hide(explore_widget[4]); // Linkage label
        gtk_widget_hide(explore_widget[5]); // Linkage selector
        gtk_widget_show(explore_widget[6]); // Filler
        gtk_widget_show(explore_widget[7]); // Colour label
        gtk_widget_show(explore_widget[8]); // Colour checkbox
        gtk_widget_hide(explore_widget[9]); // Density folder label
        gtk_widget_hide(explore_widget[10]); // Density folder selector
        break;
    }
    case EXPLORE_ATTRACTOR_DENSITY: {
        gtk_widget_hide(explore_button[0]);
        gtk_widget_hide(explore_button[1]);
        gtk_widget_hide(explore_button[2]);
        gtk_widget_hide(explore_button[3]);
        gtk_widget_hide(explore_button[4]);
        gtk_widget_hide(explore_button[5]);
        gtk_widget_show(explore_button[6]);
        gtk_widget_hide(explore_button[7]);
        gtk_widget_hide(explore_widget[0]); // Slow label
        gtk_widget_hide(explore_widget[1]); // Slow checkbutton
        gtk_widget_hide(explore_widget[2]); // Metric label
        gtk_widget_hide(explore_widget[3]); // Metric selector
        gtk_widget_hide(explore_widget[4]); // Linkage label
        gtk_widget_hide(explore_widget[5]); // Linkage selector
        gtk_widget_hide(explore_widget[6]); // Filler
        gtk_widget_hide(explore_widget[7]); // Colour label
        gtk_widget_hide(explore_widget[8]); // Colour checkbox
        gtk_widget_show(explore_widget[9]); // Density folder label
        gtk_widget_show(explore_widget[10]); // Density folder selector
        break;
    }
    case EXPLORE_ATTRACTOR_DENDROGRAMS: {
        gtk_widget_hide(explore_button[0]);
        gtk_widget_hide(explore_button[1]);
        gtk_widget_hide(explore_button[2]);
        gtk_widget_hide(explore_button[3]);
        gtk_widget_hide(explore_button[4]);
        gtk_widget_hide(explore_button[5]);
        gtk_widget_hide(explore_button[6]);
        gtk_widget_show(explore_button[7]);
        gtk_widget_hide(explore_widget[0]); // Slow label
        gtk_widget_hide(explore_widget[1]); // Slow checkbutton
        gtk_widget_show(explore_widget[2]);
        gtk_widget_show(explore_widget[3]);
        gtk_widget_show(explore_widget[4]);
        gtk_widget_show(explore_widget[5]);
        gtk_widget_show(explore_widget[6]);
        gtk_widget_hide(explore_widget[7]); // Colour label
        gtk_widget_hide(explore_widget[8]); // Colour checkbox
        gtk_widget_hide(explore_widget[9]); // Density folder label
        gtk_widget_hide(explore_widget[10]); // Density folder selector
        break;
    }
    }
}

/*----------------------------------------------------------------------------*/

static void callback_select_explore_mode(GtkWidget *combobox, XGlobals *xg)
{
    explore_mode = (ExploreModeType) gtk_combo_box_get_active(GTK_COMBO_BOX(combobox));
    explore_mode_activate_buttons();
    explore_viewer_repaint(NULL, NULL, xg);
}

/*----------------------------------------------------------------------------*/

static void test_name_all_callback(GtkWidget *caller, XGlobals *xg)
{
    PatternList *p, *best;
    ClampType clamp;
    double max_me, me;
    double mean_d, d, mean_set;
    GdkColor colour_red;
    GdkColor colour_black;
    int j = 0, a;
    char buffer[64];

    table_set_dimensions(explore_table_name_all,  pattern_list_length(xg->pattern_set) + 1, 5);

    if (explore_network == NULL) {
        hub_explore_initialise_network(xg);
    }

    // Initialise
    gdk_color_parse("red", &colour_red);
    gdk_color_parse("black", &colour_black);
    max_me = 0.0;
    mean_set = 0.0;
    mean_d = 0.0;
    a = 0; // Number correct (i.e., accuracy)

    for (p = xg->pattern_set; p != NULL; p = p->next) {
        double vector_io[NUM_IO];

        /* Set up the clamp: */

        if (explore_mode == EXPLORE_TABULATE_FROM_VISUAL) {
            table_set_title(explore_table_name_all, "Error by Pattern (Naming from Visual Input)");
            clamp_set_clamp_visual(&clamp, 0, 3 * explore_network->params.ticks, p);
        }
        else if (explore_mode == EXPLORE_TABULATE_FROM_VERBAL) {
            table_set_title(explore_table_name_all, "Error by Pattern (Naming from Verbal Input)");
            clamp_set_clamp_verbal(&clamp, 0, 3 * explore_network->params.ticks, p);
        }
        else if (explore_mode == EXPLORE_TABULATE_FROM_NAME) {
            table_set_title(explore_table_name_all, "Error by Pattern (Naming from Name Input)");
            clamp_set_clamp_name(&clamp, 0, 3 * explore_network->params.ticks, p);
        }
        else if (explore_mode == EXPLORE_TABULATE_FROM_ALL) {
            table_set_title(explore_table_name_all, "Error by Pattern (Naming from All Input)");
            clamp_set_clamp_all(&clamp, 0, 3 * explore_network->params.ticks, p);
        }

        /* Initialise units to random values etc: */
        network_initialise(explore_network);
        do {
            network_tell_recirculate_input(explore_network, &clamp);
            network_tell_propagate(explore_network);
        } while (!network_is_settled(explore_network));

        network_ask_output(explore_network, vector_io);

        table_cell_set_value_from_string(explore_table_name_all, j, 0, p->name);

        table_cell_set_value_from_int(explore_table_name_all, j, 2, explore_network->cycles);
        mean_set += explore_network->cycles;
        // distance between vector_io and target
        d = pattern_get_distance(p, vector_io);
        table_cell_set_value_from_double(explore_table_name_all, j, 4, d);
        mean_d += d;

        if ((best = pattern_list_get_best_name_match(xg->pattern_set, vector_io)) != NULL) {
            me = pattern_get_max_bit_error(best, vector_io);

            table_cell_set_value_from_string(explore_table_name_all, j, 1, best->name);
            table_cell_set_value_from_double(explore_table_name_all, j, 3, me);
            max_me = MAX(max_me, me);
            if (best == p) {
                table_cell_set_properties(explore_table_name_all, j, 0, &colour_black, PANGO_STYLE_NORMAL);
                table_cell_set_properties(explore_table_name_all, j, 1, &colour_black, PANGO_STYLE_NORMAL);
                table_cell_set_properties(explore_table_name_all, j, 2, &colour_black, PANGO_STYLE_NORMAL);
                table_cell_set_properties(explore_table_name_all, j, 3, &colour_black, PANGO_STYLE_NORMAL);
                table_cell_set_properties(explore_table_name_all, j, 4, &colour_black, PANGO_STYLE_NORMAL);
                a++;
            }
            else {
                table_cell_set_properties(explore_table_name_all, j, 0, &colour_red, PANGO_STYLE_NORMAL);
                table_cell_set_properties(explore_table_name_all, j, 1, &colour_red, PANGO_STYLE_NORMAL);
                table_cell_set_properties(explore_table_name_all, j, 2, &colour_red, PANGO_STYLE_NORMAL);
                table_cell_set_properties(explore_table_name_all, j, 3, &colour_red, PANGO_STYLE_NORMAL);
                table_cell_set_properties(explore_table_name_all, j, 4, &colour_red, PANGO_STYLE_NORMAL);
            }
        }
        else {
            table_cell_set_value_from_string(explore_table_name_all, j, 1, NULL);
            table_cell_set_value_from_string(explore_table_name_all, j, 3, NULL);
            table_cell_set_properties(explore_table_name_all, j, 0, &colour_red, PANGO_STYLE_NORMAL);
            table_cell_set_properties(explore_table_name_all, j, 2, &colour_red, PANGO_STYLE_NORMAL);
            table_cell_set_properties(explore_table_name_all, j, 4, &colour_red, PANGO_STYLE_NORMAL);
        }
        j++;
    }

    if (j > 1) {
        mean_set = mean_set / (double) j;
        mean_d = mean_d / (double) j;
    }

    table_cell_set_value_from_string(explore_table_name_all, j, 0, "OVERALL:");
    g_snprintf(buffer, 64, "%d / %d", a, j);
    table_cell_set_value_from_string(explore_table_name_all, j, 1, buffer);
    table_cell_set_value_from_double(explore_table_name_all, j, 2, mean_set);
    table_cell_set_value_from_double(explore_table_name_all, j, 3, max_me);
    table_cell_set_value_from_double(explore_table_name_all, j, 4, mean_d);

    explore_viewer_repaint(NULL, NULL, xg);
}

void ftabulate_naming_accuracy(FILE *fp, XGlobals *xg)
{
    /* Try the net on every pattern, given visual input, and record the error */
    if (fp != NULL) {
        PatternList *p;
        double d, ne, me;
        ClampType clamp;
        int n = 0;

        for (p = xg->pattern_set; p != NULL; p = p->next) {
            double vector_io[NUM_IO];

            /* Set up the clamp: */

            table_set_title(explore_table_name_all, "Error by Pattern (Naming from Visual Input)");
            clamp_set_clamp_visual(&clamp, 0, 3 * explore_network->params.ticks, p);
            /* Initialise units to random values etc: */
            network_initialise(xg->net);
            do {
                network_tell_recirculate_input(xg->net, &clamp);
                network_tell_propagate(xg->net);
            } while (!network_is_settled(xg->net));

            network_ask_output(xg->net, vector_io);

            d = pattern_get_distance(p, vector_io);
            ne = pattern_get_name_distance(p, vector_io);
            me = pattern_get_max_bit_error(p, vector_io);

            if (pattern_is_animal(p)) {
                fprintf(fp, "%d 0 %d %f %f %f\n", n++, (int) p->category, d, ne, me);
            }
            if (pattern_is_artifact(p)){
                fprintf(fp, "%d 1 %d %f %f %f\n", n++, (int) p->category, d, ne, me);
            }
        }
    }
}

/*----------------------------------------------------------------------------*/

static void test_attractors_callback(GtkWidget *caller, XGlobals *xg)
{
    PatternList *p;
    double attractor[NUM_PATTERNS][NUM_SEMANTIC];
    double sum_cat[CAT_MAX];
    double ssq_cat[CAT_MAX];
    int count_cat[CAT_MAX];
    double sum_dom[2];
    double ssq_dom[2];
    int count_dom[2];
    double d;
    int i, j, c;

    for (i = 0; i < CAT_MAX; i++) {
        sum_cat[i] = 0;
        ssq_cat[i] = 0;
        count_cat[i] = 0;
    }
    for (i = 0; i < DOM_MAX; i++) {
        sum_dom[i] = 0;
        ssq_dom[i] = 0;
        count_dom[i] = 0;
    }

    if (explore_network == NULL) {
        hub_explore_initialise_network(xg);
    }

    generate_attractor_states(explore_network, xg->pattern_set, attractor);

    for (p = xg->pattern_set, j = 0; p != NULL; p = p->next, j++) {
        d = vector_length(NUM_SEMANTIC, attractor[j]);

        sum_cat[(int) p->category] += d;
        ssq_cat[(int) p->category] += d*d;
        count_cat[(int) p->category]++;

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

    if (explore_graph_categories[0] != NULL) {
        // Add the category data to the graph:
        for (i = 0; i < CAT_MAX; i++) {
            explore_graph_categories[0]->dataset[0].x[i] = i;
            explore_graph_categories[0]->dataset[0].y[i] = (count_cat[i] > 0) ? (sum_cat[i] / (double) count_cat[i]) : 0.0;
            explore_graph_categories[0]->dataset[0].se[i] = (count_cat[i] > 1) ? sqrt((ssq_cat[i] - (sum_cat[i]*sum_cat[i] / (double) (count_cat[i])))/((double) count_cat[i]-1)) / sqrt(count_cat[i]) : 0.0;
        }
        explore_graph_categories[0]->dataset[0].points = CAT_MAX;
    }

    if (explore_graph_categories[1] != NULL) {
        // Add the category similarity data to the graph:
        for (c = 0; c < CAT_MAX; c++) {
            double mean, se;

            get_category_similarity(xg->pattern_set, c, attractor, &mean, &se);
            explore_graph_categories[1]->dataset[0].x[c] = c;
            explore_graph_categories[1]->dataset[0].y[c] = mean;
            explore_graph_categories[1]->dataset[0].se[c] = se;
        }
        explore_graph_categories[1]->dataset[0].points = CAT_MAX;
    }

    if (explore_graph_domains[0] != NULL) {
        // Add the domain data to the graph:
        for (i = 0; i < 2; i++) {
            explore_graph_domains[0]->dataset[0].x[i] = i;
            explore_graph_domains[0]->dataset[0].y[i] = (count_dom[i] > 0) ? sum_dom[i] / (double) count_dom[i] : 0.0;

            explore_graph_domains[0]->dataset[0].se[i] = (count_dom[i] > 1) ? sqrt((ssq_dom[i] - (sum_dom[i]*sum_dom[i] / (double) (count_dom[i])))/((double) count_dom[i]-1)) / sqrt(count_dom[i]) : 0.0;
        }
        explore_graph_domains[0]->dataset[0].points = 2;
    }

    if (explore_graph_domains[1] != NULL) {
        // Add the domain similarity data to the graph:
        for (c = 0; c < 2; c++) {
            double mean, se;

            get_domain_similarity(xg->pattern_set, c, attractor, &mean, &se);
            explore_graph_domains[1]->dataset[0].x[c] = c;
            explore_graph_domains[1]->dataset[0].y[c] = mean;
            explore_graph_domains[1]->dataset[0].se[c] = se;
        }
        explore_graph_domains[1]->dataset[0].points = 2;
        
    }

    explore_viewer_repaint(NULL, NULL, xg);
}

/*----------------------------------------------------------------------------*/

static void test_attractor_similarity_matrix_callback(GtkWidget *caller, XGlobals *xg)
{
    double attractor[NUM_PATTERNS][NUM_SEMANTIC];
    double d, max_d;
    int i, j;

    if (explore_network == NULL) {
        hub_explore_initialise_network(xg);
    }

    // First collect the attractor states:
    generate_attractor_states(xg->net, xg->pattern_set, attractor);

    // Now calculate similarity:
    max_d = 0;
    for (i = 0; i < NUM_PATTERNS; i++) {
        for (j = 0; j < NUM_PATTERNS; j++) {
            // Similarity as measured by Euclidean distance
            if (explore_metric == METRIC_JACCARD) {
                d = jaccard_distance(NUM_SEMANTIC, attractor[j], attractor[i]);
            }
            else if (explore_metric == METRIC_EUCLIDEAN) {
                d = euclidean_distance(NUM_SEMANTIC, attractor[j], attractor[i]);
            }
            else { // Default case - should not happen!
                d = 10.0; 
            }
            explore_attractor_similarity_distance[j*NUM_PATTERNS+i] = d; 
            max_d = MAX(max_d, d);

            // Similarity as measured by correlation (rescalled);
            d = vector_correlation(NUM_SEMANTIC, attractor[j], attractor[i]);
            explore_attractor_similarity_correlation[j*NUM_PATTERNS+i] = 1 - ((d + 1) / 2.0); 

            // Similarity as measured by correlation (rescalled);
            d = vector_cosine(NUM_SEMANTIC, attractor[j], attractor[i]);
            explore_attractor_similarity_cosine[j*NUM_PATTERNS+i] = 1 - ((d + 1) / 2.0); 
        }
    }

    // Standardise/ Normalise distance: 
    if (explore_metric == METRIC_JACCARD) {
        max_d = 1.0;
    }
    else if (explore_metric == METRIC_EUCLIDEAN) {
        max_d = 8.0;
    }
    else {
        max_d = 10.0;
    }

    for (i = 0; i < NUM_PATTERNS; i++) {
        for (j = 0; j < NUM_PATTERNS; j++) {
            explore_attractor_similarity_distance[j*NUM_PATTERNS+i] = 1 - (explore_attractor_similarity_distance[j*NUM_PATTERNS+i] / max_d);
        }
    }

    // And redraw:
    explore_viewer_repaint(NULL, NULL, xg);
}

/*----------------------------------------------------------------------------*/

static void test_settle_callback(GtkWidget *caller, XGlobals *xg)
{
    if (xg->current_pattern == NULL) {
        gtkx_warn(xg->frame, "Please select an input pattern before settling the network");
    }
    else {
        ClampType clamp;

        if (explore_network == NULL) {
            hub_explore_initialise_network(xg);
        }

        /* Set up the clamp: */
        clamp_set_clamp_visual(&clamp, 0, 3 * explore_network->params.ticks, xg->current_pattern);
        /* Initialise units to random values etc: */
        network_initialise(explore_network);

        do {
            network_tell_recirculate_input(explore_network, &clamp);
            explore_viewer_repaint(NULL, NULL, xg);
            gtkx_flush_events();
            if (explore_slow) {
                g_usleep(500000);
            }
            network_tell_propagate(explore_network);
        } while (!network_is_settled(explore_network));

        explore_viewer_repaint(NULL, NULL, xg);
    }
}

/*----------------------------------------------------------------------------*/

static void refresh_attractor_density_callback(GtkWidget *caller, XGlobals *xg)
{
    explore_attractor_density_regenerate(xg);
    explore_viewer_repaint(NULL, NULL, xg);
}

/*----------------------------------------------------------------------------*/

static void refresh_attractor_dendrogram_callback(GtkWidget *caller, XGlobals *xg)
{
    regenerate_attractor_dendrogram(xg);
    explore_viewer_repaint(NULL, NULL, xg);
}

/*----------------------------------------------------------------------------*/

static void test_first_callback(GtkWidget *caller, XGlobals *xg)
{
    xg->current_pattern = xg->pattern_set;
    explore_viewer_repaint(NULL, NULL, xg);
}

/*----------------------------------------------------------------------------*/

static void test_next_callback(GtkWidget *caller, XGlobals *xg)
{
    if ((xg->current_pattern == NULL) || (xg->current_pattern->next == NULL)) {
        xg->current_pattern = xg->pattern_set;
    }
    else {
        xg->current_pattern = xg->current_pattern->next;
    }
    explore_viewer_repaint(NULL, NULL, xg);
}

/*----------------------------------------------------------------------------*/

static void damage_net_callback(GtkWidget *caller, XGlobals *xg)
{
    hub_explore_initialise_network(xg);
    /* Now apply damage: */
    if (explore_noise_sd > 0.0) {
        network_perturb_weights(explore_network, explore_noise_sd);
    }
    if (explore_disconnection_severity > 0.0) {
        network_sever_weights(explore_network, explore_disconnection_severity);
    }
    if (explore_ablation_severity > 0.0) {
        network_ablate_units(explore_network, explore_ablation_severity);
    }
    if (explore_weight_scaling < 1.0) {
        network_scale_weights(explore_network, explore_weight_scaling);
    }

    explore_viewer_repaint(NULL, NULL, xg);
}

/*----------------------------------------------------------------------------*/

static void reinstate_net_callback(GtkWidget *caller, XGlobals *xg)
{
    hub_explore_initialise_network(xg);
    explore_viewer_repaint(NULL, NULL, xg);
}

/*----------------------------------------------------------------------------*/

static void callback_spin_lesion_level(GtkWidget *sb, XGlobals *xg)
{
    explore_disconnection_severity = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(sb)) / 100.0;
}

/*----------------------------------------------------------------------------*/

static void callback_spin_noise_level(GtkWidget *sb, XGlobals *xg)
{
    explore_noise_sd  = gtk_spin_button_get_value(GTK_SPIN_BUTTON(sb));
}

/*----------------------------------------------------------------------------*/

static void callback_toggle_slow(GtkWidget *caller, XGlobals *xg)
{
    explore_slow = (Boolean) (GTK_TOGGLE_BUTTON(caller)->active);
}

/*----------------------------------------------------------------------------*/

static void callback_select_metric(GtkWidget *combobox, XGlobals *xg)
{
    explore_metric = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox));
    regenerate_attractor_dendrogram(xg);
    explore_viewer_repaint(NULL, NULL, xg);
}

/*----------------------------------------------------------------------------*/

static void callback_select_linkage(GtkWidget *combobox, XGlobals *xg)
{
    explore_linkage = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox));
    regenerate_attractor_dendrogram(xg);
    explore_viewer_repaint(NULL, NULL, xg);
}

/*----------------------------------------------------------------------------*/

static void callback_toggle_colour(GtkWidget *caller, XGlobals *xg)
{
    explore_colour = (Boolean) (GTK_TOGGLE_BUTTON(caller)->active);
    explore_viewer_repaint(NULL, NULL, xg);
}

/*----------------------------------------------------------------------------*/

static void callback_select_density_folder(GtkWidget *combobox, XGlobals *xg)
{
    explore_attractor_density_regenerate(xg);
    explore_viewer_repaint(NULL, NULL, xg);
}
/*----------------------------------------------------------------------------*/

static void xexplore_print_callback(GtkWidget *caller, XGlobals *xg)
{
    cairo_surface_t *surface = NULL;
    cairo_t *cr = NULL;
    PangoLayout *layout;
    char filename[128], prefix[64];
    int width  = explore_viewer_widget->allocation.width;
    int height = explore_viewer_widget->allocation.height;

    g_snprintf(prefix, 64, "%s_%s", xg->pattern_set_name, print_file_prefix[(int) explore_mode]);

    /* PDF version: */
    g_snprintf(filename, 128, "%s/%s.pdf", PRINT_FOLDER, prefix);
    surface = cairo_pdf_surface_create(filename, width, height);
    cr = cairo_create(surface);
    layout = pango_cairo_create_layout(cr);

    if (explore_mode == EXPLORE_SETTLE_NAME_FROM_VISUAL) {
        explore_viewer_repaint_naming(cr, xg);
    }
    else if (explore_mode == EXPLORE_TABULATE_FROM_VISUAL) {
        explore_viewer_repaint_name_table(cr, xg);
    }
    else if (explore_mode == EXPLORE_TABULATE_FROM_VERBAL) {
        explore_viewer_repaint_name_table(cr, xg);
    }
    else if (explore_mode == EXPLORE_TABULATE_FROM_NAME) {
        explore_viewer_repaint_name_table(cr, xg);
    }
    else if (explore_mode == EXPLORE_TABULATE_FROM_ALL) {
        explore_viewer_repaint_name_table(cr, xg);
    }
    else if (explore_mode == EXPLORE_ATTRACTOR_BAR_CHARTS) {
        explore_viewer_repaint_attractor_length(cr, xg);
    }
    else if (explore_mode == EXPLORE_ATTRACTOR_SIMILARITY) {
        explore_viewer_repaint_attractor_similarity_matrix(cr, xg);
    }
    else if (explore_mode == EXPLORE_ATTRACTOR_DENSITY) {
        explore_viewer_repaint_attractor_density(cr, xg);
    }
    else if (explore_mode == EXPLORE_ATTRACTOR_DENDROGRAMS) {
        explore_viewer_repaint_attractor_dendrogram(cr, xg);
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

static void explore_viewer_destroy_callback(GtkWidget *caller, XGlobals *xg)
{
    int i;

    if (explore_network != NULL) {
        network_destroy(explore_network);
        explore_network = NULL;
    }
    if (explore_table_name_all  != NULL) {
        table_destroy(explore_table_name_all);
        explore_table_name_all = NULL;
    }
    for (i = 0; i < 2; i++) {
        if (explore_graph_categories[i] != NULL) {
            graph_destroy(explore_graph_categories[i]);
            explore_graph_categories[i] = NULL;
        }
        if (explore_graph_domains[i] != NULL) {
            graph_destroy(explore_graph_domains[i]);
            explore_graph_domains[i] = NULL;
        }
    }
}

/*============================================================================*/


static Boolean is_non_dot_directory(struct dirent *de)
{
    return((de->d_type == 4) && (de->d_name[0] != '.'));
}

static void combo_box_append_folder_names(GtkWidget *cb, char *path)
{
    DIR *dir;
    struct dirent *de;

    if ((dir = opendir(path)) != NULL) {
        while ((de = readdir(dir)) != NULL) {
            if (is_non_dot_directory(de)) {
                gtk_combo_box_append_text(GTK_COMBO_BOX(cb), de->d_name);
            }
        }
        closedir(dir);
    }
}

/*----------------------------------------------------------------------------*/

void hub_explore_create_widgets(GtkWidget *page, XGlobals *xg)
{
    /* The buttons an widgets for exploring the hub model's behaviour: */

    GtkWidget *tmp, *hbox;
    GtkWidget *scrolled_win;
    CairoxFontProperties *fp;
    int k;

    fp = font_properties_create("Sans", 18, PANGO_STYLE_NORMAL, PANGO_WEIGHT_NORMAL, "black");

    /* Create the table structure: */

    if (explore_table_name_all == NULL) {
        explore_table_name_all = table_create();

        table_set_margins(explore_table_name_all, 10, 10, 20, 10);
        table_set_title(explore_table_name_all, "Error by Pattern");
        table_set_dimensions(explore_table_name_all, 0, 5);
        table_set_column_properties(explore_table_name_all, 0, "Visual Input", PANGO_ALIGN_LEFT, 100);
        table_set_column_properties(explore_table_name_all, 1, "Name Output", PANGO_ALIGN_LEFT, 100);
        table_set_column_properties(explore_table_name_all, 2, "Cycles", PANGO_ALIGN_RIGHT, 100);
        table_set_column_properties(explore_table_name_all, 3, "Error (Max. Bit)", PANGO_ALIGN_RIGHT, 100);
        table_set_column_properties(explore_table_name_all, 4, "Error (D)", PANGO_ALIGN_RIGHT, 100);
    }

    /* Create the graph structures: */

    char *cat_labels[2*CAT_MAX+1] = {"", "Birds", "", "Mammals", "", "Fruit", "", "Tools", "", "Vehicles", "", "HH Objects", ""};
    char *dom_labels[5] = {"", "Animals", "", "Artefacts", ""};

    if (explore_graph_categories[0] == NULL) {
        // Create the category graph and set its properties 
        explore_graph_categories[0] = graph_create(1);
        graph_set_margins(explore_graph_categories[0], 50, 10, 25, 40);
        font_properties_set_size(fp, 18);
        graph_set_title_font_properties(explore_graph_categories[0], fp);
        graph_set_axis_properties(explore_graph_categories[0], GTK_ORIENTATION_HORIZONTAL, -0.5, CAT_MAX-0.5, 2*CAT_MAX+1, "%s", "Categories");
        graph_set_axis_tick_marks(explore_graph_categories[0], GTK_ORIENTATION_HORIZONTAL, 2*CAT_MAX+1, cat_labels);
        graph_set_axis_properties(explore_graph_categories[0], GTK_ORIENTATION_VERTICAL, 3, 7, 5, "%3.1f", "Euclidean Length");
        graph_set_dataset_properties(explore_graph_categories[0], 0, NULL, 0.5, 0.5, 0.5, 1.0, 25, LS_SOLID, MARK_NONE);
        font_properties_set_size(fp, 14);
        graph_set_axis_font_properties(explore_graph_categories[0], GTK_ORIENTATION_HORIZONTAL, fp);
        graph_set_axis_font_properties(explore_graph_categories[0], GTK_ORIENTATION_VERTICAL, fp);
        graph_set_legend_font_properties(explore_graph_categories[0], fp);
    }

    if (explore_graph_categories[1] == NULL) {
        // Create the category graph and set its properties 
        explore_graph_categories[1] = graph_create(1);
        graph_set_margins(explore_graph_categories[1], 50, 10, 25, 40);
        font_properties_set_size(fp, 18);
        graph_set_title_font_properties(explore_graph_categories[1], fp);
        graph_set_axis_properties(explore_graph_categories[1], GTK_ORIENTATION_HORIZONTAL, -0.5, CAT_MAX-0.5, 2*CAT_MAX+1, "%s", "Categories");
        graph_set_axis_tick_marks(explore_graph_categories[1], GTK_ORIENTATION_HORIZONTAL, 2*CAT_MAX+1, cat_labels);
#ifdef SIMILAR
        graph_set_axis_properties(explore_graph_categories[1], GTK_ORIENTATION_VERTICAL, 0.0, 1.0, 6, "%3.1f", similarity_name[explore_similarity]);
#else
        graph_set_axis_properties(explore_graph_categories[1], GTK_ORIENTATION_VERTICAL, 0.0, 10.0, 6, "%3.1f", "Euclidean Distance");
#endif
        graph_set_dataset_properties(explore_graph_categories[1], 0, NULL, 0.5, 0.5, 0.5, 1.0, 25, LS_SOLID, MARK_NONE);
        font_properties_set_size(fp, 14);
        graph_set_axis_font_properties(explore_graph_categories[1], GTK_ORIENTATION_HORIZONTAL, fp);
        graph_set_axis_font_properties(explore_graph_categories[1], GTK_ORIENTATION_VERTICAL, fp);
        graph_set_legend_font_properties(explore_graph_categories[1], fp);
    }

    if (explore_graph_domains[0] == NULL) {
        // Create the domain graph and set its properties 
        explore_graph_domains[0] = graph_create(1);
        graph_set_margins(explore_graph_domains[0], 50, 10, 25, 40);
        font_properties_set_size(fp, 18);
        graph_set_title_font_properties(explore_graph_domains[0], fp);
        graph_set_axis_properties(explore_graph_domains[0], GTK_ORIENTATION_HORIZONTAL, -0.5, 1.5, 5, "%s", "Domains");
        graph_set_axis_tick_marks(explore_graph_domains[0], GTK_ORIENTATION_HORIZONTAL, 5, dom_labels);
        graph_set_axis_properties(explore_graph_domains[0], GTK_ORIENTATION_VERTICAL, 3, 7, 5, "%3.1f", "Euclidean Length");
        graph_set_dataset_properties(explore_graph_domains[0], 0, NULL, 0.5, 0.5, 0.5, 1.0, 50, LS_SOLID, MARK_NONE);
        font_properties_set_size(fp, 14);
        graph_set_axis_font_properties(explore_graph_domains[0], GTK_ORIENTATION_HORIZONTAL, fp);
        graph_set_axis_font_properties(explore_graph_domains[0], GTK_ORIENTATION_VERTICAL, fp);
        graph_set_legend_font_properties(explore_graph_domains[0], fp);
    }
    if (explore_graph_domains[1] == NULL) {
        // Create the domain graph and set its properties 
        explore_graph_domains[1] = graph_create(1);
        graph_set_margins(explore_graph_domains[1], 50, 10, 25, 40);
        font_properties_set_size(fp, 18);
        graph_set_title_font_properties(explore_graph_domains[1], fp);
        graph_set_axis_properties(explore_graph_domains[1], GTK_ORIENTATION_HORIZONTAL, -0.5, 1.5, 5, "%s", "Domains");
        graph_set_axis_tick_marks(explore_graph_domains[1], GTK_ORIENTATION_HORIZONTAL, 5, dom_labels);
#ifdef SIMILAR
        graph_set_axis_properties(explore_graph_domains[1], GTK_ORIENTATION_VERTICAL, 0.0, 1.0, 6, "%3.1f", similarity_name[explore_similarity]);
#else
        graph_set_axis_properties(explore_graph_domains[1], GTK_ORIENTATION_VERTICAL, 0.0, 10.0, 6, "%3.1f", "Euclidean Distance");
#endif
        graph_set_dataset_properties(explore_graph_domains[1], 0, NULL, 0.5, 0.5, 0.5, 1.0, 50, LS_SOLID, MARK_NONE);
        font_properties_set_size(fp, 14);
        graph_set_axis_font_properties(explore_graph_domains[1], GTK_ORIENTATION_HORIZONTAL, fp);
        graph_set_axis_font_properties(explore_graph_domains[1], GTK_ORIENTATION_VERTICAL, fp);
        graph_set_legend_font_properties(explore_graph_domains[1], fp);
    }

    for (k = 0; k < 3; k++) {
        if (explore_weight_histogram[k] == NULL) {
            // Create histogram of visible to hidden weights:
            explore_weight_histogram[k] = graph_create(1);
            graph_set_margins(explore_weight_histogram[k], 50, 10, 25, 40);
            font_properties_set_size(fp, 18);
            graph_set_title_font_properties(explore_weight_histogram[k], fp);
            graph_set_axis_properties(explore_weight_histogram[k], GTK_ORIENTATION_HORIZONTAL, -2.5, 2.5, 6, "%+3.1f", "Weight");
            graph_set_axis_properties(explore_weight_histogram[k], GTK_ORIENTATION_VERTICAL, 0.0, 0.5, 6, "%3.1f", "Rel. Freq.");
            graph_set_dataset_properties(explore_weight_histogram[k], 0, NULL, 1.0, 0.0, 0.0, 1.0, 0, LS_SOLID, MARK_NONE);
            font_properties_set_size(fp, 14);
            graph_set_axis_font_properties(explore_weight_histogram[k], GTK_ORIENTATION_HORIZONTAL, fp);
            graph_set_axis_font_properties(explore_weight_histogram[k], GTK_ORIENTATION_VERTICAL, fp);
            graph_set_legend_font_properties(explore_weight_histogram[k], fp);
        }
    }

    font_properties_destroy(fp);

    // Initialise attractor similarity to -1 to indicate this is not known
    explore_initialise_attractor_similarity();

    // Initialise attractor density to -1 to indicate this is not known
    explore_attractor_density_initialise();

    /* Now build the widgets for the network explorer */

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    /* Widgets for setting explore type: */
    tmp = gtk_label_new("Explore:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    tmp = gtk_combo_box_new_text();
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Settle (from Visual)");
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Tabulate (from Visual)");
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Tabulate (from Verbal)");
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Tabulate (from Name)");
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Tabulate (from All)");
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Weight Distribution");
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Attractor Bar Charts");
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Attractor Similarity");
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Attractor Density");
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Attractor Dendrogram");
    // Possibly add more combo options here 
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), (int) explore_mode);
    g_signal_connect(G_OBJECT(tmp), "changed", G_CALLBACK(callback_select_explore_mode), xg);
    gtk_widget_show(tmp);

    /*--- Filler: ---*/
    tmp = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 0);
    gtk_widget_show(tmp);

    /*--- Toolbar-like buttons for selecting a pattern etc.: ---*/
    explore_button[0] = gtkx_stock_image_button_create(hbox, GTK_STOCK_GOTO_FIRST, G_CALLBACK(test_first_callback), xg);
    explore_button[1] = gtkx_stock_image_button_create(hbox, GTK_STOCK_GO_FORWARD, G_CALLBACK(test_next_callback), xg);
    explore_button[2] = gtkx_stock_image_button_create(hbox, GTK_STOCK_EXECUTE, G_CALLBACK(test_settle_callback), xg);
    explore_button[3] = gtkx_stock_image_button_create(hbox, GTK_STOCK_REFRESH, G_CALLBACK(test_name_all_callback), xg);
    explore_button[4] = gtkx_stock_image_button_create(hbox, GTK_STOCK_REFRESH, G_CALLBACK(test_attractors_callback), xg);
    explore_button[5] = gtkx_stock_image_button_create(hbox, GTK_STOCK_REFRESH, G_CALLBACK(test_attractor_similarity_matrix_callback), xg);
    explore_button[6] = gtkx_stock_image_button_create(hbox, GTK_STOCK_REFRESH, G_CALLBACK(refresh_attractor_density_callback), xg);
    explore_button[7] = gtkx_stock_image_button_create(hbox, GTK_STOCK_REFRESH, G_CALLBACK(refresh_attractor_dendrogram_callback), xg);

    /*--- Filler: ---*/
    tmp = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 0);
    gtk_widget_show(tmp);

    /* Widgets for adjusting the lesion severity: */
    /*--- The label: ---*/
    tmp = gtk_label_new("  Severity (%): ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
    /*--- The spin button: ---*/
    tmp = gtk_spin_button_new_with_range(0, 100, 1);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(tmp), 0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tmp), explore_disconnection_severity * 100);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(tmp), "value-changed", G_CALLBACK(callback_spin_lesion_level), xg);
    gtk_widget_show(tmp);

    /* Widgets for adjusting the lesion noise: */
    /*--- The label: ---*/
    tmp = gtk_label_new("  Noise (SD): ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
    /*--- The spin button: ---*/
    tmp = gtk_spin_button_new_with_range(0, 5, 0.1);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(tmp), 2);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tmp), explore_noise_sd);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(tmp), "value-changed", G_CALLBACK(callback_spin_noise_level), xg);
    gtk_widget_show(tmp);

    /* Damage / reinstate the network: */
    tmp = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);

    gtkx_stock_image_button_create(hbox, GTK_STOCK_APPLY, G_CALLBACK(damage_net_callback), xg); // WAS GTK_STOCK_CUT
    gtkx_stock_image_button_create(hbox, GTK_STOCK_REVERT_TO_SAVED, G_CALLBACK(reinstate_net_callback), xg);

    /*--- Filler: ---*/
    tmp = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 0);
    gtk_widget_show(tmp);

    /* Widgets for setting screen update speed: */
    /*--- The label: ---*/
    tmp = gtk_label_new("  Slow: ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    explore_widget[0] = tmp;
    /*--- The checkbox: ---*/
    tmp = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), explore_slow);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(tmp), "toggled", G_CALLBACK(callback_toggle_slow), xg);
    explore_widget[1] = tmp;

    /* Dendrogram control widgets (for when mode = 4): */
    /* Widgets for setting the metric: */
    /*--- The label: ---*/
    tmp = gtk_label_new("  Metric: ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    explore_widget[2] = tmp;
    /*--- The combobox selector: ---*/
    tmp = gtk_combo_box_new_text();
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), metric_name[0]);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), metric_name[1]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), explore_metric);
    g_signal_connect(G_OBJECT(tmp), "changed", G_CALLBACK(callback_select_metric), xg);
    explore_widget[3] = tmp;

    /* Widgets for setting the linkage function: */
    /*--- The label: ---*/
    tmp = gtk_label_new("  Linkage: ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    explore_widget[4] = tmp;
    /*--- The combobox selector: ---*/
    tmp = gtk_combo_box_new_text();
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), linkage_name[0]);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), linkage_name[1]);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), linkage_name[2]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), explore_linkage);
    g_signal_connect(G_OBJECT(tmp), "changed", G_CALLBACK(callback_select_linkage), xg);
    explore_widget[5] = tmp;

    /* Widgets for setting attractr density analysis folder: */
    /*--- The label: ---*/
    tmp = gtk_label_new("  Folder: ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    explore_widget[9] = tmp;
    /*--- The combobox selector: ---*/
    tmp = gtk_combo_box_new_text();
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    combo_box_append_folder_names(tmp, "DataFiles");
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), 0);
    g_signal_connect(G_OBJECT(tmp), "changed", G_CALLBACK(callback_select_density_folder), xg);
    explore_widget[10] = tmp;

    /*--- Filler: ---*/
    tmp = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 0);
    explore_widget[6] = tmp;

    /* Widgets for setting colour/greyscale: */
    /*--- The label: ---*/
    tmp = gtk_label_new("  Colour: ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    explore_widget[7] = tmp;
    /*--- The checkbox: ---*/
    tmp = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), explore_colour);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(tmp), "toggled", G_CALLBACK(callback_toggle_colour), xg);
    explore_widget[8] = tmp;

    /*--- Spacer: ---*/
    tmp = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    /*--- And a print button: ---*/
    gtkx_stock_image_button_create(hbox, GTK_STOCK_PRINT, G_CALLBACK(xexplore_print_callback), xg);

    /* The rest of the page - a scrolling window: */
    tmp = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(page), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);

    scrolled_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(page), scrolled_win, TRUE, TRUE, 0);
    gtk_widget_show(scrolled_win);

    explore_viewer_widget = gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(explore_viewer_widget), "expose_event", G_CALLBACK(explore_viewer_repaint), xg);
    g_signal_connect(G_OBJECT(explore_viewer_widget), "configure_event", G_CALLBACK(gtkx_configure_surface_callback), &explore_viewer_surface);
    g_signal_connect(G_OBJECT(explore_viewer_widget), "destroy", G_CALLBACK(gtkx_destroy_surface_callback), &explore_viewer_surface);
    g_signal_connect(G_OBJECT(explore_viewer_widget), "destroy", G_CALLBACK(explore_viewer_destroy_callback), xg);
    gtk_widget_set_events(explore_viewer_widget, GDK_EXPOSURE_MASK);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_win), explore_viewer_widget);
    gtk_widget_show(explore_viewer_widget);

    explore_mode_activate_buttons();
    gtk_widget_show(page);
}

/******************************************************************************/

void hub_explore_initialise_network(XGlobals *xg)
{
    if (explore_network != NULL) {
        network_destroy(explore_network);
    }
    explore_network = network_copy(xg->net);
    explore_initialise_attractor_similarity();
}

/******************************************************************************/
