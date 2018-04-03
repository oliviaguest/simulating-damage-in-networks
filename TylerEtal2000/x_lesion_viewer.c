
#include "xframe.h"
#include "lib_string.h"
#include "lib_cairoxg_2_2.h"

#define MAX_NETWORKS 300
#define MAX_POINTS    21
static float animal_within[MAX_NETWORKS][MAX_POINTS];
static float animal_between[MAX_NETWORKS][MAX_POINTS];
static float animal_correct[MAX_NETWORKS][MAX_POINTS];
static float animal_error_s[MAX_NETWORKS][MAX_POINTS];
static float animal_error_d[MAX_NETWORKS][MAX_POINTS];
static float animal_error[MAX_NETWORKS][MAX_POINTS];
static float artifact_within[MAX_NETWORKS][MAX_POINTS];
static float artifact_between[MAX_NETWORKS][MAX_POINTS];
static float artifact_correct[MAX_NETWORKS][MAX_POINTS];
static float artifact_error[MAX_NETWORKS][MAX_POINTS];

// Tyler et al used 0.80
#define MAX_DAMAGE 1.00

// Maximum noise (SD) added when lesioning weights (Note: Tyler et al didn't use noise)
// A reasonable value is 4.00 for the feedforward network and 3.00 for the recurrent network
// Use 3.0 if normally distributed:
#define MAX_NOISE 3.00
// Use 5.0 if uniformly distributed:
// #define MAX_NOISE 5.00

static GtkWidget *viewer = NULL; // The drawing area

static int graph_id = 0;
static GraphStruct *gd = NULL;

static Boolean paused;

/******************************************************************************/

static void lesion_viewer_paint_to_cairo(cairo_t *cr, GraphStruct *gd, double w, double h, Boolean colour, int lesion_type, NetworkType nt, int reps)
{
    char buffer[128];

    graph_set_extent(gd, 0, 0, w, h);
    graph_set_legend_properties(gd, TRUE, 0.03, 0.05, NULL); // Default - override if appropriate
    if (lesion_type == 0) {
        graph_set_axis_properties(gd, GTK_ORIENTATION_HORIZONTAL, 0, 100*MAX_DAMAGE, 11, "%2.0f%%", "Percentage of severed connections");
    }
    else {
        graph_set_axis_properties(gd, GTK_ORIENTATION_HORIZONTAL, 0, MAX_NOISE, 11, "%3.1f", "Noise on weights (SD)");
    }
    switch (graph_id) {
        case 0: { // Fig 3: Animals versus Artefacts / Distinctive
            g_snprintf(buffer, 128, "Effect of Damage on Distinctive Perceptual Features [%s; %d reps]", nt_name[nt], reps);
            graph_set_title(gd, buffer);
            graph_set_axis_properties(gd, GTK_ORIENTATION_VERTICAL, 0, 0.5, 11, "%4.2f", "Mean absolute error (per feature)");
            break;
	}
        case 1: { // Fig 4: Shared versus Distinctive / Perceptual
            g_snprintf(buffer, 128, "Effect of Damage on Features of Animals [%s; %d reps]", nt_name[nt], reps);
            graph_set_title(gd, buffer);
            graph_set_axis_properties(gd, GTK_ORIENTATION_VERTICAL, 0, 0.5, 11, "%4.2f", "Mean absolute error (per feature)");
            break;
	}
        case 2: { // Fig 5: Animal versus Artefacts / Shared
            g_snprintf(buffer, 128, "Effect of Damage on Shared Features [%s; %d reps]", nt_name[nt], reps);
            graph_set_title(gd, buffer);
            graph_set_axis_properties(gd, GTK_ORIENTATION_VERTICAL, 0, 0.5, 11, "%4.2f", "Mean absolute error / shared feature");
            break;
	}
        case 3: { // Fig 6: Animal versus Artefacts / Functional
            g_snprintf(buffer, 128, "Effect of Damage on Functional Features [%s; %d reps]", nt_name[nt], reps);
            graph_set_title(gd, buffer);
            graph_set_axis_properties(gd, GTK_ORIENTATION_VERTICAL, 0, 0.5, 11, "%4.2f", "Mean absolute error / functional feature");
            break;
	}
        case 4: { // Fig 7: Identity: Animals versus Artefacts
            g_snprintf(buffer, 128, "Effect of Damage on Identity Judgements [%s; %d reps]", nt_name[nt], reps);
            graph_set_title(gd, buffer);    
            graph_set_axis_properties(gd, GTK_ORIENTATION_VERTICAL, 0, 100.0, 11, "%3.0f%%", "Percent patterns correct");
            graph_set_legend_properties(gd, TRUE, 0.789, 0.00, NULL); // Override the default
            break;
	}
        case 5: { // Fig 8: Error breakdown
            g_snprintf(buffer, 128, "Types of Error as a Function of Damage [%s; %d reps]", nt_name[nt], reps);
            graph_set_title(gd, buffer);
            graph_set_axis_properties(gd, GTK_ORIENTATION_VERTICAL, 0, 100.0, 11, "%3.0f%%", "Percent patterns incorrect");
            break;
	}
	default: {
	    fprintf(stderr, "Invalid graph selection\n");
	}
    }

    cairox_draw_graph(cr, gd, colour);
}

static Boolean lesion_viewer_repaint(XGlobals *xg)
{
    cairo_t *cr;
    double w, h;

    if (viewer != NULL) {
        w = viewer->allocation.width;
        h = viewer->allocation.height;
        cr = gdk_cairo_create(viewer->window);

        lesion_viewer_paint_to_cairo(cr, gd, w, h, xg->colour, xg->lesion_type, xg->pars.nt, xg->reps);
        cairo_destroy(cr);
    }
    return(FALSE);
}

static Boolean lesion_viewer_repaint_callback(GtkWidget *widget, GdkEventConfigure *event, XGlobals *xg)
{
    return(lesion_viewer_repaint(xg));
}

/******************************************************************************/

void test_features(Network *net, PatternList *training_set, int j, int i, double ll, FeatureType ft)
{
    double *r = (double *)malloc(IO_WIDTH * sizeof(double));
    PatternList *p;
    double animal_f;
    double artifact_f;
    int animal_t;
    int artifact_t;
    double sum_animal_e;
    double sum_artifact_e;
    double ssq_animal_e;
    double ssq_artifact_e;
    int k;

    gd->dataset[0].x[i] = ll;
    gd->dataset[1].x[i] = ll;

    animal_f = 0;
    artifact_f = 0;
    animal_t = 0;
    artifact_t = 0;

    for (p = training_set; p != NULL; p = p->next) {
        network_tell_input(net, p->vector_in);
        network_tell_propagate_full(net);
        network_ask_output(net, r);

        if (pattern_is_animal(p)) {
            animal_t++;
            animal_f += response_error(p, r, ft);
        }
        else {
            artifact_t++;
            artifact_f += response_error(p, r, ft);
        }
    }
    animal_error[j][i] = animal_f / (double) animal_t;
    artifact_error[j][i] = artifact_f / (double) artifact_t;

    sum_animal_e = 0;
    sum_artifact_e = 0;
    ssq_animal_e = 0;
    ssq_artifact_e = 0;

    for (k = 0; k <= j; k++) {
        sum_animal_e += animal_error[k][i];
        ssq_animal_e += (animal_error[k][i] * animal_error[k][i]);
        sum_artifact_e += artifact_error[k][i];
        ssq_artifact_e += (artifact_error[k][i] * artifact_error[k][i]);
    }

    // Set line data to means of [j][i] for k = 0 < j
    gd->dataset[0].y[i] = (j+1 > 0) ? sum_animal_e / (double) (j+1) : 0.0;
    gd->dataset[0].se[i] = sqrt((ssq_animal_e - (sum_animal_e*sum_animal_e / (double) (j+1)))/((double) j)) / sqrt(j+1);
    gd->dataset[1].y[i] = (j+1 > 0) ? sum_artifact_e / (double) (j+1) : 0.0;
    gd->dataset[1].se[i] = sqrt((ssq_artifact_e - (sum_artifact_e*sum_artifact_e / (double) (j+1)))/((double) j)) / sqrt(j+1);
}

void test_animal_features(Network *net, PatternList *training_set, int j, int i, double ll)
{
    double *r = (double *)malloc(IO_WIDTH * sizeof(double));
    PatternList *p;
    double animal_s;
    double animal_d;
    int animal_t;
    double sum_animal_s;
    double sum_animal_d;
    double ssq_animal_s;
    double ssq_animal_d;
    int k;

    gd->dataset[0].x[i] = ll;
    gd->dataset[1].x[i] = ll;

    animal_s = 0;
    animal_d = 0;
    animal_t = 0;

    for (p = training_set; p != NULL; p = p->next) {
        if (pattern_is_animal(p)) {
            network_tell_input(net, p->vector_in);
            network_tell_propagate_full(net);
            network_ask_output(net, r);

            animal_t++;
            animal_s += response_error(p, r, F_SHARED);
            animal_d += response_error(p, r, F_DISTINCTIVE);
        }
    }
    animal_error_s[j][i] = animal_s / (double) animal_t;
    animal_error_d[j][i] = animal_d / (double) animal_t;

    sum_animal_s = 0;
    sum_animal_d = 0;
    ssq_animal_s = 0;
    ssq_animal_d = 0;

    for (k = 0; k <= j; k++) {
        sum_animal_s += animal_error_s[k][i];
        ssq_animal_s += (animal_error_s[k][i] * animal_error_s[k][i]);
        sum_animal_d += animal_error_d[k][i];
        ssq_animal_d += (animal_error_d[k][i] * animal_error_d[k][i]);
    }

    // Set line data to means of [j][i] for k = 0 < j
    gd->dataset[0].y[i] = (j+1 > 0) ? sum_animal_s / (double) (j+1) : 0.0;
    gd->dataset[0].se[i] = sqrt((ssq_animal_s - (sum_animal_s*sum_animal_s / (double) (j+1)))/((double) j)) / sqrt(j+1);
    gd->dataset[1].y[i] = (j+1 > 0) ? sum_animal_d / (double) (j+1) : 0.0;
    gd->dataset[1].se[i] = sqrt((ssq_animal_d - (sum_animal_d*sum_animal_d / (double) (j+1)))/((double) j)) / sqrt(j+1);
}

void test_patterns_correct(Network *net, PatternList *training_set, int j, int i, double ll)
{
    double *r = (double *)malloc(IO_WIDTH * sizeof(double));
    PatternList *p;
    int animal_t;
    int animal_c;
    int artifact_t;
    int artifact_c;
    double sum_animal_c;
    double sum_artifact_c;
    double ssq_animal_c;
    double ssq_artifact_c;
    int k;

    gd->dataset[0].x[i] = ll;
    gd->dataset[1].x[i] = ll;

    animal_t = 0;
    animal_c = 0;
    artifact_t = 0;
    artifact_c = 0;

    for (p = training_set; p != NULL; p = p->next) {

        network_tell_input(net, p->vector_in);
        network_tell_propagate_full(net);
        network_ask_output(net, r);

        if (pattern_is_animal(p)) {
            animal_t++;
            if (response_is_correct(training_set, p, r)) {
                animal_c++;
            }
        }
        else {
            artifact_t++;
            if (response_is_correct(training_set, p, r)) {
                artifact_c++;
            }
        }
    }
    animal_correct[j][i] = animal_c * 100.0 / (double) animal_t;
    artifact_correct[j][i] = artifact_c * 100.0 / (double) artifact_t;

    sum_animal_c = 0;
    sum_artifact_c = 0;
    ssq_animal_c = 0;
    ssq_artifact_c = 0;

    for (k = 0; k <= j; k++) {
        sum_animal_c += animal_correct[k][i];
        ssq_animal_c += (animal_correct[k][i] * animal_correct[k][i]);
        sum_artifact_c += artifact_correct[k][i];
        ssq_artifact_c += (artifact_correct[k][i] * artifact_correct[k][i]);
    }

    // Set line data to means of [j][i] for k = 0 < j
    gd->dataset[0].y[i] = (j+1 > 0) ? sum_animal_c / (double) (j+1) : 0.0;
    gd->dataset[0].se[i] = sqrt((ssq_animal_c - (sum_animal_c*sum_animal_c / (double) (j+1)))/((double) j)) / sqrt(j+1);
    gd->dataset[1].y[i] = (j+1 > 0) ? sum_artifact_c / (double) (j+1) : 0.0;
    gd->dataset[1].se[i] = sqrt((ssq_artifact_c - (sum_artifact_c*sum_artifact_c / (double) (j+1)))/((double) j)) / sqrt(j+1);
}

void test_patterns_error_breakdown(Network *net, PatternList *training_set, int j, int i, double ll)
{
    double *r = (double *)malloc(IO_WIDTH * sizeof(double));
    PatternList *p;
    int animal_t;
    int animal_w;
    int animal_b;
    int artifact_t;
    int artifact_w;
    int artifact_b;
    double sum_animal_w;
    double sum_artifact_w;
    double ssq_animal_w;
    double ssq_artifact_w;
    double sum_animal_b;
    double sum_artifact_b;
    double ssq_animal_b;
    double ssq_artifact_b;
    int k;

    gd->dataset[0].x[i] = ll;
    gd->dataset[1].x[i] = ll;
    gd->dataset[2].x[i] = ll;
    gd->dataset[3].x[i] = ll;

    animal_t = 0;
    animal_w = 0;
    animal_b = 0;
    artifact_t = 0;
    artifact_w = 0;
    artifact_b = 0;

    for (p = training_set; p != NULL; p = p->next) {

        network_tell_input(net, p->vector_in);
        network_tell_propagate_full(net);
        network_ask_output(net, r);

	// DOMAIN_w means a "within category" error for that domain
	// DOMAIN_b means a "between category" error for that domain
	// There is another type of error - betweeen domain.
	// We don't count the latter.

        if (pattern_is_animal(p)) {
            animal_t++;
	    if (pattern_is_animal1(p) && response_is_animal2(training_set, r)) {
                animal_b++;
	    }
            else if (pattern_is_animal2(p) && response_is_animal1(training_set, r)) {
                animal_b++;
            }
            else if (!response_is_correct(training_set, p, r)) {
	      if (response_is_animal1(training_set, r) || response_is_animal2(training_set, r)) {
                animal_w++;
	      }
	    }
        }
        else {
            artifact_t++;
	    if (pattern_is_artifact1(p) && response_is_artifact2(training_set, r)) {
                artifact_b++;
	    }
            else if (pattern_is_artifact2(p) && response_is_artifact1(training_set, r)) {
                artifact_b++;
            }
            else if (!response_is_correct(training_set, p, r)) {
	      if (response_is_artifact1(training_set, r) || response_is_artifact2(training_set, r)) {
                artifact_w++;
	      }
	    }
        }
    }
    animal_within[j][i] = animal_w * 100.0 / (double) animal_t;
    animal_between[j][i] = animal_b * 100.0 / (double) animal_t;
    artifact_within[j][i] = artifact_w * 100.0 / (double) artifact_t;
    artifact_between[j][i] = artifact_b * 100.0 / (double) artifact_t;

    sum_animal_w = 0;
    sum_animal_b = 0;
    sum_artifact_w = 0;
    sum_artifact_b = 0;
    ssq_animal_w = 0;
    ssq_animal_b = 0;
    ssq_artifact_w = 0;
    ssq_artifact_b = 0;

    for (k = 0; k <= j; k++) {
        sum_animal_w += animal_within[k][i];
        sum_animal_b += animal_between[k][i];
        sum_artifact_w += artifact_within[k][i];
        sum_artifact_b += artifact_between[k][i];
        ssq_animal_w += (animal_within[k][i] * animal_within[k][i]);
        ssq_animal_b += (animal_between[k][i] * animal_between[k][i]);
        ssq_artifact_w += (artifact_within[k][i] * artifact_within[k][i]);
        ssq_artifact_b += (artifact_between[k][i] * artifact_between[k][i]);
    }

    // Set line data to means of [j][i] for k = 0 < j
    gd->dataset[0].y[i] = (j+1 > 0) ? sum_animal_b / (double) (j+1) : 0.0;
    gd->dataset[0].se[i] = sqrt((ssq_animal_b - (sum_animal_b*sum_animal_b / (double) (j+1)))/((double) j)) / sqrt(j+1);
    gd->dataset[1].y[i] = (j+1 > 0) ? sum_animal_w / (double) (j+1) : 0.0;
    gd->dataset[1].se[i] = sqrt((ssq_animal_w - (sum_animal_w*sum_animal_w / (double) (j+1)))/((double) j)) / sqrt(j+1);
    gd->dataset[2].y[i] = (j+1 > 0) ? sum_artifact_b / (double) (j+1) : 0.0;
    gd->dataset[2].se[i] = sqrt((ssq_artifact_b - (sum_artifact_b*sum_artifact_b / (double) (j+1)))/((double) j)) / sqrt(j+1);
    gd->dataset[3].y[i] = (j+1 > 0) ? sum_artifact_w / (double) (j+1) : 0.0;
    gd->dataset[3].se[i] = sqrt((ssq_artifact_w - (sum_artifact_w*sum_artifact_w / (double) (j+1)))/((double) j)) / sqrt(j+1);
}

static void generate_lesion_data(XGlobals *xg)
{
    Network *tmp, *my_net;
    int i, j;

    paused = FALSE;

    for (j = 0; j < MAX_NETWORKS; j++) {
        xg->reps = j+1;
        fprintf(stdout, "Network %4d of %d ...", xg->reps, MAX_NETWORKS); fflush(stdout);
        if (xg->generate) {
            my_net = network_initialise(xg->pars.nt, IO_WIDTH, HIDDEN_WIDTH, IO_WIDTH);
            network_initialise_weights(my_net, xg->pars.wn);
            network_train_to_epochs(my_net, &(xg->pars), xg->training_set);
        }
        else {
            my_net = network_copy(xg->net);
        }
        fprintf(stdout, " Weight range: [%7.5f - %7.5f];",         network_weight_minimum(my_net), network_weight_maximum(my_net)); 
        fprintf(stdout, " Error: %7.5f\n", sqrt(network_test(my_net, xg->training_set, SUM_SQUARE_ERROR))); fflush(stdout);

        for (i = 0; i < MAX_POINTS; i++) {
            double ll;
            tmp = network_copy(my_net);

            if (xg->lesion_type == 0) {
                ll = 100 * i * MAX_DAMAGE / (double) (MAX_POINTS - 1);
                network_lesion_weights(tmp, ll / 100.0);
            }
            else {
                ll = i * MAX_NOISE / (double) (MAX_POINTS - 1);
                network_perturb_weights(tmp, ll);
            }

            switch (graph_id) {
                case 0: { // Fig 3: Animals versus Artefacts / Distinctive
                    test_features(tmp, xg->training_set, j, i, ll, F_DISTINCTIVE);
                    gd->dataset[0].points = MAX_POINTS;
                    gd->dataset[1].points = MAX_POINTS;
                    gd->dataset[2].points = 0;
                    gd->dataset[3].points = 0;
                    break;
                }
                case 1: { // Fig 4: Shared versus Distinctive / Perceptual
                    test_animal_features(tmp, xg->training_set, j, i, ll);
                    gd->dataset[0].points = MAX_POINTS;
                    gd->dataset[1].points = MAX_POINTS;
                    gd->dataset[2].points = 0;
                    gd->dataset[3].points = 0;
                    break;
                }
                case 2: { // Fig 5: Animal versus Artefacts / Shared
                    test_features(tmp, xg->training_set, j, i, ll, F_SHARED);
                    gd->dataset[0].points = MAX_POINTS;
                    gd->dataset[1].points = MAX_POINTS;
                    gd->dataset[2].points = 0;
                    gd->dataset[3].points = 0;
                    break;
                }
                case 3: { // Fig 6: Animal versus Artefacts / Functional
                    test_features(tmp, xg->training_set, j, i, ll, F_FUNCTIONAL);
                    gd->dataset[0].points = MAX_POINTS;
                    gd->dataset[1].points = MAX_POINTS;
                    gd->dataset[2].points = 0;
                    gd->dataset[3].points = 0;
                    break;
                }
                case 4: { // Fig 7: Identity: Animals versus Artefacts
                    test_patterns_correct(tmp, xg->training_set, j, i, ll);
                    gd->dataset[0].points = MAX_POINTS;
                    gd->dataset[1].points = MAX_POINTS;
                    gd->dataset[2].points = 0;
                    gd->dataset[3].points = 0;
                    break;
                }
                case 5: { // Fig 8: Error breakdown
                    test_patterns_error_breakdown(tmp, xg->training_set, j, i, ll);
                    gd->dataset[0].points = MAX_POINTS;
                    gd->dataset[1].points = MAX_POINTS;
                    gd->dataset[2].points = MAX_POINTS;
                    gd->dataset[3].points = MAX_POINTS;
                    break;
                }
                default: {
                    fprintf(stderr, "Invalid graph selection\n");
                }
            }
            network_destroy(tmp);
        }
        network_destroy(my_net);
        lesion_viewer_repaint(xg);
        gtkx_flush_events();

	if (paused) {
	  j = MAX_NETWORKS;
	}

    }
}

static void configure_lesion_graph(GraphStruct *gd, int graph_num)
{
    graph_id = graph_num;

    switch (graph_id) {
        case 0: { // Fig 3: Animals versus Artefacts / Distinctive
            graph_set_dataset_properties(gd, 0, "Living things", 0.7, 0.3, 0.7, 0, LS_SOLID, MARK_CIRCLE_FILLED);
            graph_set_dataset_properties(gd, 1, "Artefacts", 0.7, 0.7, 0.3, 0, LS_SOLID, MARK_CIRCLE_OPEN);
            break;
        }
        case 1: { // Fig 4: Shared versus Distinctive / Perceptual
            graph_set_dataset_properties(gd, 0, "Shared perceptual", 0.7, 0.2, 0.2, 0, LS_SOLID, MARK_TRIANGLE_OPEN);
            graph_set_dataset_properties(gd, 1, "Distinctive perceptual", 0.2, 0.2, 0.7, 0, LS_SOLID, MARK_TRIANGLE_FILLED);
            break;
        }
        case 2: { // Fig 5: Animal versus Artefacts / Shared
            graph_set_dataset_properties(gd, 0, "Living things", 0.7, 0.3, 0.7, 0, LS_SOLID, MARK_CIRCLE_FILLED);
            graph_set_dataset_properties(gd, 1, "Artefacts", 0.7, 0.7, 0.3, 0, LS_SOLID, MARK_CIRCLE_OPEN);
            break;
        }
        case 3: { // Fig 6: Animal versus Artefacts / Functional
            graph_set_dataset_properties(gd, 0, "Living things", 0.7, 0.3, 0.7, 0, LS_SOLID, MARK_CIRCLE_FILLED);
            graph_set_dataset_properties(gd, 1, "Artefacts", 0.7, 0.7, 0.3, 0, LS_SOLID, MARK_CIRCLE_OPEN);
            break;
        }
        case 4: { // Fig 7: Identity: Animals versus Artefacts
            graph_set_dataset_properties(gd, 0, "Living things", 0.7, 0.3, 0.7, 0, LS_SOLID, MARK_CIRCLE_FILLED);
            graph_set_dataset_properties(gd, 1, "Artefacts", 0.7, 0.7, 0.3, 0, LS_SOLID, MARK_CIRCLE_OPEN);
            break;
        }
        case 5: { // Fig 8: Error breakdown
            graph_set_dataset_properties(gd, 0, "Living things; Between category", 0.7, 0.3, 0.7, 0, LS_DOTTED, MARK_CIRCLE_FILLED);
            graph_set_dataset_properties(gd, 1, "Living things; Within category", 0.5, 0.1, 0.5, 0, LS_SOLID, MARK_SQUARE_FILLED);
            graph_set_dataset_properties(gd, 2, "Artefacts; Between category", 0.7, 0.7, 0.3, 0, LS_DOTTED, MARK_CIRCLE_OPEN);
            graph_set_dataset_properties(gd, 3, "Artefacts; Within category", 0.5, 0.5, 0.1, 0, LS_SOLID, MARK_SQUARE_OPEN);
            break;
        }
        default: {
        }
    }
}

static void set_dump_file(XGlobals *xg, int graph_num)
{
    char buffer[64];
    char *old;

    if ((old = g_object_get_data(G_OBJECT(xg->frame), "dump file prefix")) != NULL) {
        free(old);
    }

    g_snprintf(buffer, 64, "figure_%02d", graph_num+3);
    g_object_set_data(G_OBJECT(xg->frame), "dump file prefix", string_copy(buffer));
}

/******************************************************************************/

static void print_graph_callback(GtkWidget *button, XGlobals *xg)
{
    double width = 700.0;
    double height = 500.0;
    char *prefix1 = NULL;
    char *prefix2 = NULL;
#ifdef CLAMPED
    char *prefix3[3] = {"ffn", "ranc"};
#else
    char *prefix3[3] = {"ffn", "ranu"};
#endif
    cairo_surface_t *surface = NULL;
    cairo_t *cr = NULL;
    char filename[128];

    if (xg != NULL) {
        prefix1 = g_object_get_data(G_OBJECT(xg->frame), "dump file prefix");
        prefix2 = (xg->lesion_type == 0) ? "zero" : "noise";
    }

    if ((prefix1 == NULL) || (prefix2 == NULL)) {
        fprintf(stderr, "NULL dump file ... snapshot will not be saved\n");
    }
    else {
        /* Colour PDF version: */
        g_snprintf(filename, 128, "FIGURES/%s_%s_%s_colour.pdf", prefix1, prefix3[xg->pars.nt], prefix2);
        surface = cairo_pdf_surface_create(filename, width, height);
        cr = cairo_create(surface);
        lesion_viewer_paint_to_cairo(cr, gd, width, height, TRUE, xg->lesion_type, xg->pars.nt, xg->reps);
        cairo_destroy(cr);
        /* Save the colour PNG while we're at it: */
        g_snprintf(filename, 128, "FIGURES/%s_%s_%s_colour.png", prefix1, prefix3[xg->pars.nt], prefix2);
        cairo_surface_write_to_png(surface, filename);
        /* And destroy the surface */
        cairo_surface_destroy(surface);

        /* B/W PDF version: */
        g_snprintf(filename, 128, "FIGURES/%s_%s_%s_bw.pdf", prefix1, prefix3[xg->pars.nt], prefix2);
        surface = cairo_pdf_surface_create(filename, width, height);
        cr = cairo_create(surface);
        lesion_viewer_paint_to_cairo(cr, gd, width, height, FALSE, xg->lesion_type, xg->pars.nt, xg->reps);
        cairo_destroy(cr);
        /* Save the B/W PNG while we're at it: */
        g_snprintf(filename, 128, "FIGURES/%s_%s_%s_bw.png", prefix1, prefix3[xg->pars.nt], prefix2);
        cairo_surface_write_to_png(surface, filename);
        /* And destroy the surface */
        cairo_surface_destroy(surface);
    }
}

static void stop_graph_callback(GtkWidget *button, XGlobals *xg)
{
    paused = TRUE;
    lesion_viewer_repaint(xg);
}

static void regenerate_graph_callback(GtkWidget *button, XGlobals *xg)
{
    xg->reps = 0;
    generate_lesion_data(xg);
    lesion_viewer_repaint(xg);
}

static void callback_toggle_colour(GtkWidget *button, XGlobals *xg)
{
    xg->colour = (Boolean) (GTK_TOGGLE_BUTTON(button)->active);
    lesion_viewer_repaint(xg);
}

static void callback_toggle_generate(GtkWidget *button, XGlobals *xg)
{
    xg->generate = (Boolean) (GTK_TOGGLE_BUTTON(button)->active);
    lesion_viewer_repaint(xg);
}

static void callback_select_lesion_type(GtkWidget *combobox, XGlobals *xg)
{
    xg->lesion_type = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox));
    // Erase existing data:
    gd->dataset[0].points = 0; 
    gd->dataset[1].points = 0; 
    gd->dataset[2].points = 0; 
    gd->dataset[3].points = 0; 
    xg->reps = 0;
    lesion_viewer_repaint(xg);
}

static void callback_select_graph(GtkWidget *combobox, XGlobals *xg)
{
    configure_lesion_graph(gd, gtk_combo_box_get_active(GTK_COMBO_BOX(combobox)));
    set_dump_file(xg, gtk_combo_box_get_active(GTK_COMBO_BOX(combobox)));
    gd->dataset[0].points = 0; 
    gd->dataset[1].points = 0; 
    gd->dataset[2].points = 0; 
    gd->dataset[3].points = 0; 
    xg->reps = 0;
    lesion_viewer_repaint(xg);
}

static void lesion_viewer_destroy(GtkWidget *viewer1, XGlobals xg)
{
    // Destroying viewer ...
    viewer = NULL;
}

void lesion_viewer_create_widgets(GtkWidget *vbox, XGlobals *xg)
{
    GtkWidget *hbox, *tmp, *toolbar;
    CairoxFontProperties *fp;
    GtkTooltips *tips;
    int position = 0;

    graph_destroy(gd);
    gd = graph_create(4);
    graph_set_margins(gd, 72, 26, 30, 45);

    if ((fp = font_properties_create("Sans", 20, PANGO_STYLE_NORMAL, PANGO_WEIGHT_NORMAL, "black")) != NULL) {
        graph_set_title_font_properties(gd, fp);
        font_properties_set_size(fp, 18);
        graph_set_legend_font_properties(gd, fp);
        graph_set_axis_font_properties(gd, GTK_ORIENTATION_HORIZONTAL, fp);
        graph_set_axis_font_properties(gd, GTK_ORIENTATION_VERTICAL, fp);
        font_properties_destroy(fp);
    }

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    /* Widgets for setting graph type: */
    tmp = gtk_label_new("Graph:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    tmp = gtk_combo_box_new_text();
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Fig 3: Animals versus Artefacts / Distinctive");
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Fig 4: Shared versus Distinctive / Perceptual");
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Fig 5: Animal versus Artefacts / Shared");
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Fig 6: Animal versus Artefacts / Functional");
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Fig 7: Identity: Animals versus Artefacts");
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Fig 8: Error breakdown");
    g_signal_connect(G_OBJECT(tmp), "changed", GTK_SIGNAL_FUNC(callback_select_graph), xg);
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), 0);
    gtk_widget_show(tmp);

    /* Widgets for setting lesion type: */
    tmp = gtk_label_new("Damage:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    tmp = gtk_combo_box_new_text();
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Disconnection");
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Noise");
    g_signal_connect(G_OBJECT(tmp), "changed", GTK_SIGNAL_FUNC(callback_select_lesion_type), xg);
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), xg->lesion_type);
    gtk_widget_show(tmp);

    /* Widgets for setting whether the current net is used, or new ones generated: */
    tmp = gtk_label_new("Regenerate:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    tmp = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), xg->generate);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(tmp), "toggled", GTK_SIGNAL_FUNC(callback_toggle_generate), xg);
    gtk_widget_show(tmp);

    /* Widgets for setting colour/greyscale: */
    tmp = gtk_label_new("Colour:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    tmp = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), xg->colour);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(tmp), "toggled", GTK_SIGNAL_FUNC(callback_toggle_colour), xg);
    gtk_widget_show(tmp);

    /*--- Toolbar with generate/print buttons: ---*/
    toolbar = gtk_toolbar_new();
    tips = gtk_tooltips_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
    gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(hbox), toolbar, TRUE, TRUE, 0);
    gtk_widget_show(toolbar);

    gtkx_toolbar_insert_space(GTK_TOOLBAR(toolbar), TRUE, position++);
    gtkx_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_REFRESH, tips, "Regenerate the current graph", G_CALLBACK(regenerate_graph_callback), xg, position++);
    gtkx_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_STOP, tips, "Stop the current set of simulations", G_CALLBACK(stop_graph_callback), xg, position++);
    gtkx_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_PRINT, tips, "Save PDF of the current graph", G_CALLBACK(print_graph_callback), xg, position++);

    /*--- Horizontal separator and the drawing area: ---*/
    tmp = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);

    viewer = gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(viewer), "expose_event", GTK_SIGNAL_FUNC(lesion_viewer_repaint_callback), xg);
    g_signal_connect(G_OBJECT(viewer), "destroy", GTK_SIGNAL_FUNC(lesion_viewer_destroy), xg);
    gtk_box_pack_start(GTK_BOX(vbox), viewer, TRUE, TRUE, 0);
    gtk_widget_show(viewer);
}
