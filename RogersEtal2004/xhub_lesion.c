/*******************************************************************************

    File:       xhub_lesion.c
    Contents:   
    Author:     Rick Cooper
    Copyright (c) 2016 Richard P. Cooper

    Public procedures:

*******************************************************************************/
/*******************************************************************************

Damage the network and produce a graph of performance as a function of damage
severity. Currently two types of damage are supported: connection removal and
weight noise.

Defined functions:
    void hub_lesion_create_widgets(GtkWidget *page, XGlobals *xg)

TO DO:
    1) Tidy up!

If a weight folder is selected then weights are loaded from a file in that
folder for each network. At the time of writing it is expected that there 
will be 20 sets of saved weights, so this only works for 20 replications.

*******************************************************************************/
/******** Include files: ******************************************************/

#include "xhub.h"
#include "lib_maths.h"
#include "lib_string.h"
#include "lib_cairoxg_2_2.h"
#include <dirent.h>
#include <string.h>

#undef INDIVIDUAL_DIFF_GRAPHS

#define RELOAD_FIXED            0
#define RELOAD_REGENERATE       1
#define RELOAD_SAMPLE_P4_LINE   2
#define RELOAD_SAMPLE_P4_CLOUD  3

/******************************************************************************/

#define MAX_SEVER       0.50
#define MAX_PERTURB     1.00
#define MAX_ABLATE      1.00
#define MIN_SCALE       0.75
#define MAX_SCALE       0.55
#define MAX_NETWORKS     300
// Replications per network:
#define MAX_REPS         10

// Number of points in the graphs:
#define MAX_POINTS       21

// Possibly add more lesion types here
typedef enum lesion_type {LESION_SEVER_WEIGHTS, LESION_PERTURB_WEIGHTS, LESION_ABLATE_UNITS, LESION_SCALE_WEIGHTS} LesionType;

static GraphStruct *damage_graph_data = NULL;
static GtkWidget   *damage_graph_viewer = NULL;
static LesionType   damage_graph_damage_type  = LESION_SEVER_WEIGHTS;
static Boolean      damage_graph_colour = TRUE;
static Boolean      damage_graph_paused = FALSE;
static int          damage_graph_repetitions = 0;
static int          damage_graph_id = 0;
static int          damage_graph_reload = 0;
static char        *damage_graph_folder_name = NULL;

//  Add names of more damage types here if necessary
static char *damage_prefix[4] = {
    "zero",
    "noise",
    "ablate",
    "scale"
};

#undef SAVE_WEIGHT_STATISTICS
#undef SAVE_NAMING_ACCURACY
#define SAVE_DOMAIN_ACCURACY

/******************************************************************************/

static void lesion_viewer_paint_to_cairo(cairo_t *cr, XGlobals *xg, double w, double h, Boolean colour)
{
    char buffer[128];

    graph_set_extent(damage_graph_data, 0, 0, w, h);
    if (damage_graph_id == 0) { // Naming: Animals versus Artifacts
        if (damage_graph_reload == RELOAD_FIXED) {
            g_snprintf(buffer, 128, "Effect of Damage on Object Naming [%s; %d replications]", xg->pattern_set_name, damage_graph_repetitions);
        }
        else {
            g_snprintf(buffer, 128, "Effect of Damage on Object Naming [%s; %d networks]", xg->pattern_set_name, damage_graph_repetitions);
        }
        graph_set_title(damage_graph_data, buffer);
        graph_set_legend_properties(damage_graph_data, TRUE, 0.849, 0.0, NULL);
    }
    else { // Lambon Ralph data
        graph_set_title(damage_graph_data, NULL);
        graph_set_legend_properties(damage_graph_data, TRUE, 0.0, 0.0, NULL);
        //        graph_set_legend_properties(damage_graph_data, TRUE, 0.056, 0.018, NULL);
    }
    cairox_draw_graph(cr, damage_graph_data, colour);
}

/*----------------------------------------------------------------------------*/

static void save_graph_to_image_file(XGlobals *xg, int width, int height, char *prefix)
{
    cairo_surface_t *surface = NULL;
    cairo_t *cr = NULL;
    char filename[128];

    /* Colour PDF version: */
    g_snprintf(filename, 128, "%s/pdf_colour/%s.pdf", PRINT_FOLDER, prefix);
    surface = cairo_pdf_surface_create(filename, width, height);
    cr = cairo_create(surface);
    lesion_viewer_paint_to_cairo(cr, xg, width, height, TRUE);
    cairo_destroy(cr);
    /* Save the colour PNG while we're at it: */
    g_snprintf(filename, 128, "%s/png_colour/%s.png", PRINT_FOLDER, prefix);
    cairo_surface_write_to_png(surface, filename);
    /* And destroy the surface */
    cairo_surface_destroy(surface);

    /* B/W PDF version: */
    g_snprintf(filename, 128, "%s/pdf_bw/%s.pdf", PRINT_FOLDER, prefix);
    surface = cairo_pdf_surface_create(filename, width, height);
    cr = cairo_create(surface);
    lesion_viewer_paint_to_cairo(cr, xg, width, height, FALSE);
    cairo_destroy(cr);
    /* Save the B/W PNG while we're at it: */
    g_snprintf(filename, 128, "%s/png_bw/%s.png", PRINT_FOLDER, prefix);
    cairo_surface_write_to_png(surface, filename);
    /* And destroy the surface */
    cairo_surface_destroy(surface);
}

/*----------------------------------------------------------------------------*/

static Boolean lesion_viewer_repaint(XGlobals *xg)
{
    cairo_t *cr;
    double w, h;

    if (damage_graph_viewer != NULL) {
        w = damage_graph_viewer->allocation.width;
        h = damage_graph_viewer->allocation.height;
        cr = gdk_cairo_create(damage_graph_viewer->window);
        lesion_viewer_paint_to_cairo(cr, xg, w, h, damage_graph_colour);
        cairo_destroy(cr);
    }
    return(FALSE);
}

/******************************************************************************/

Boolean name_is_unique(PatternList *patterns, double name_vec[NUM_NAME])
{
    PatternList *p;
    int count = 0;

    for (p = patterns; p != NULL; p = p->next) {
        if (vector_sum_square_difference(NUM_NAME, p->name_features, name_vec) < 0.0001) {
            count++;
        }
    }
    return(count == 1);
}

/*----------------------------------------------------------------------------*/

static void test_naming(Network *test_net, PatternList *patterns, double *animal_error, double *artifact_error)
{
    PatternList *p;
    int animal_c;
    int artifact_c;
    int animal_t;
    int artifact_t;
    Boolean correct;
    ClampType clamp;

    animal_c = 0;
    artifact_c = 0;
    animal_t = 0;
    artifact_t = 0;

    for (p = patterns; p != NULL; p = p->next) {
        double vector_io[NUM_IO];

        /* Set up the clamp: */
        clamp_set_clamp_verbal(&clamp, 0, 3 * test_net->params.ticks, p);

        /* Initialise units to random values etc: */
        network_initialise(test_net);
        do {
            network_tell_recirculate_input(test_net, &clamp);
            network_tell_propagate(test_net);
        } while (!network_is_settled(test_net));

        network_ask_output(test_net, vector_io);
        correct = (p == pattern_list_get_best_name_match(patterns, vector_io));

        if (name_is_unique(patterns, p->name_features)) {
            if (pattern_is_animal(p)) {
                animal_t++;
                if (correct) {
                    animal_c++;
                }
            }
            else if (pattern_is_artifact(p)) {
                artifact_t++;
                if (correct) {
                    artifact_c++;
                }
            }
        }
    }
    *animal_error = animal_c / (double) animal_t;
    *artifact_error = artifact_c / (double) artifact_t;
}

/*----------------------------------------------------------------------------*/

static void save_results_to_graph(int num_net, int i, double ll, double an_err, double art_err)
{
    // We need these to be static so that values persist while we sum
    // over replications
    static float animal_error[MAX_NETWORKS][MAX_POINTS];
    static float artifact_error[MAX_NETWORKS][MAX_POINTS];

    animal_error[num_net][i] = an_err;
    artifact_error[num_net][i] = art_err;

    if (damage_graph_reload == RELOAD_SAMPLE_P4_CLOUD) {
        double jitter = 0.0;

        if (damage_graph_damage_type == LESION_SEVER_WEIGHTS) {
            jitter = -0.33;
        }
        else if (damage_graph_damage_type == LESION_PERTURB_WEIGHTS) {
            jitter = -0.0033;
        }
        else if (damage_graph_damage_type == LESION_ABLATE_UNITS) {
            jitter = -0.33;
        }
        else if (damage_graph_damage_type == LESION_SCALE_WEIGHTS) {
            jitter = -0.0033;
        }

        /* When we're sampling from P4, we show each datapoint separately */
        damage_graph_data->dataset[2*num_net].x[i] = ll-jitter;
        damage_graph_data->dataset[2*num_net].y[i] = an_err;
        damage_graph_data->dataset[2*num_net].se[i] = 0.0;
        damage_graph_data->dataset[2*num_net].points = i+1;
        damage_graph_data->dataset[2*num_net+1].x[i] = ll+jitter;
        damage_graph_data->dataset[2*num_net+1].y[i] = art_err;
        damage_graph_data->dataset[2*num_net+1].se[i] = 0.0;
        damage_graph_data->dataset[2*num_net+1].points = i+1;
    }
    else {
        /* Otherwise, we show means and standard errors: */

        double sum_animal_e;
        double sum_artifact_e;
        double ssq_animal_e;
        double ssq_artifact_e;
        int k;

        sum_animal_e = 0;
        sum_artifact_e = 0;
        ssq_animal_e = 0;
        ssq_artifact_e = 0;

        for (k = 0; k <= num_net; k++) {
            sum_animal_e += animal_error[k][i];
            ssq_animal_e += (animal_error[k][i] * animal_error[k][i]);
            sum_artifact_e += artifact_error[k][i];
            ssq_artifact_e += (artifact_error[k][i] * artifact_error[k][i]);
        }

        // Now log the results on the graph:
        damage_graph_data->dataset[0].x[i] = ll;
        damage_graph_data->dataset[1].x[i] = ll;
        // Set line data to means of [num_net][i] for k = 0 < num_net
        damage_graph_data->dataset[0].y[i] = (num_net+1 > 0) ? sum_animal_e / (double) (num_net+1) : 0.0;
        damage_graph_data->dataset[0].se[i] = sqrt((ssq_animal_e - (sum_animal_e*sum_animal_e / (double) (num_net+1)))/((double) num_net)) / sqrt(num_net+1);
        damage_graph_data->dataset[1].y[i] = (num_net+1 > 0) ? sum_artifact_e / (double) (num_net+1) : 0.0;
        damage_graph_data->dataset[1].se[i] = sqrt((ssq_artifact_e - (sum_artifact_e*sum_artifact_e / (double) (num_net+1)))/((double) num_net)) / sqrt(num_net+1);

    // The above calculation of SE assumes that observations from each network
    // are independent. If we're looking at behaviour of the general
    // model, regardless of this training instance, then we should use the sqrt
    // of the number of networks, but if we're looking at a single training
    // instance, then we use the number of times the network has been lesioned.
    }
}

/*============================================================================*/

static void generate_lesion_data(XGlobals *xg, int steps)
{
    Network *tmp, *my_net;
    int i, num_net, num_rep;
    char filename[128];
    FILE *fp;

    damage_graph_paused = FALSE;

    if (damage_graph_repetitions + steps > MAX_NETWORKS) {
        steps = MAX_NETWORKS - damage_graph_repetitions;
    }


    for (num_net = 0; num_net < steps; num_net++) {
        damage_graph_repetitions++;
        if (damage_graph_reload == RELOAD_FIXED) {
            fprintf(stdout, "Network %3d of %3d ...", num_net+1, steps); fflush(stdout);
            my_net = network_copy(xg->net);
        }
        else if (damage_graph_reload == RELOAD_REGENERATE) {
            fprintf(stdout, "Network %3d of %3d ...", num_net+1, steps); fflush(stdout);
            my_net = network_create(xg->net->nt, NUM_IO, NUM_SEMANTIC, NUM_IO);
            network_parameters_set(my_net, &(xg->net->params));
            network_initialise_weights(my_net);
            network_train_to_epochs(my_net, xg->pattern_set);
        }
        else if ((damage_graph_reload == RELOAD_SAMPLE_P4_LINE) || (damage_graph_reload == RELOAD_SAMPLE_P4_CLOUD)) {
            // In this case we want to sample from the networks trained with
            // different versions of P4, so we load patterns and weights for
            // each, but note that at present we only have 10 nets
            if (damage_graph_folder_name != NULL) {
                free(damage_graph_folder_name);
            }
            damage_graph_folder_name = string_copy("p4X_1000");
            damage_graph_folder_name[2] = 'a' + num_net;

            pattern_file_load_from_folder(xg, damage_graph_folder_name);
            g_snprintf(filename, 128, "DataFiles/%s/%02d.wgt", damage_graph_folder_name, 1);
            if (!weight_file_read(xg, filename)) {
                damage_graph_repetitions--;
                lesion_viewer_repaint(xg);
                return;
            }
            my_net = network_copy(xg->net);

        }
        else { // Read weights from the given folder:
            pattern_file_load_from_folder(xg, damage_graph_folder_name);
            g_snprintf(filename, 128, "DataFiles/%s/%02d.wgt", damage_graph_folder_name, damage_graph_repetitions);
            if (!weight_file_read(xg, filename)) {
                damage_graph_repetitions--;
                lesion_viewer_repaint(xg);
                return;
            }
            my_net = network_copy(xg->net);
#ifdef SAVE_WEIGHT_STATISTICS
            g_snprintf(filename, 128, "NetworkStatistics/%s_%s_weight_stats.dat", xg->pattern_set_name, damage_prefix[damage_graph_damage_type]);
            // Write statistics of the weight matrices
            fp = fopen(filename, "a");
            ftabulate_weight_statistics(fp, my_net);
            fclose(fp);
#endif
#ifdef SAVE_NAMING_ACCURACY

            // Filename for saving stats:
            // FIXME : I BROKE THIS!
            g_snprintf(filename, 128, "NetworkStatistics/%s_%s_naming_accuracy.dat", xg->pattern_set_name, damage_prefix[damage_graph_damage_type]);
            if (num_net == 0) {
                fp = fopen(filename, "w");
                fprintf(fp, "N\tAnArea\tArtArea\tDiff\n");
            }

            if (num_net == 0) {
                fp = fopen(filename, "w");
                fprintf(fp, "N\tAnArea\tArtArea\tDiff\n");
            }
            else {
                fp = fopen(filename, "a");
            }
            ftabulate_naming_accuracy(fp, xg);
            fclose(fp);
#endif
        }

        // FIXME: Do we need this?
        damage_graph_data->dataset[0].points = MAX_POINTS;
        damage_graph_data->dataset[1].points = MAX_POINTS;

        // Used to calculate area under the curve
        double an_area = 0.0, art_area = 0.0;
        double prev_an_err = 0.0, prev_art_err = 0.0;
        double interval_width; 

        if (damage_graph_damage_type == LESION_SEVER_WEIGHTS) {
            interval_width = MAX_SEVER / (double) (MAX_POINTS - 1);
        }
        else if (damage_graph_damage_type == LESION_PERTURB_WEIGHTS) {
            interval_width = MAX_PERTURB / (double) (MAX_POINTS - 1);
        }
        else if (damage_graph_damage_type == LESION_ABLATE_UNITS) {
            interval_width = MAX_ABLATE / (double) (MAX_POINTS - 1);
        }
        else if (damage_graph_damage_type == LESION_SCALE_WEIGHTS) {
            interval_width = (MIN_SCALE - MAX_SCALE) / (double) (MAX_POINTS - 1);
        }
        else {
            interval_width = 0;
        }

        for (i = 0; i < MAX_POINTS; i++) {
            double ll;
            double an_err, art_err;
            double an_err_sum = 0.0, art_err_sum = 0.0;

            if (damage_graph_damage_type == LESION_SEVER_WEIGHTS) {
                ll = 100 * i * MAX_SEVER / (double) (MAX_POINTS - 1);
            }
            else if (damage_graph_damage_type == LESION_PERTURB_WEIGHTS) {
                ll = i * MAX_PERTURB / (double) (MAX_POINTS - 1);
            }
            else if (damage_graph_damage_type == LESION_ABLATE_UNITS) {
                ll = 100 * i * MAX_ABLATE / (double) (MAX_POINTS - 1);
            }
            else if (damage_graph_damage_type == LESION_SCALE_WEIGHTS) {
                ll = MIN_SCALE - (MIN_SCALE-MAX_SCALE) * (i / (double) (MAX_POINTS - 1));
            }
            else {
                ll = 0.0;
                interval_width = 0;
                fprintf(stdout, "WARNING: Unknown damage type!\n");
            }

            // Repeat multiple times per network, accumulating results as we go
            for (num_rep = 0; num_rep < MAX_REPS; num_rep++) {
                tmp = network_copy(my_net);

                if (damage_graph_damage_type == LESION_SEVER_WEIGHTS) {
                    network_sever_weights(tmp, ll / 100.0);
                }
                else if (damage_graph_damage_type == LESION_PERTURB_WEIGHTS) {
                    network_perturb_weights(tmp, ll);
                }
                else if (damage_graph_damage_type == LESION_ABLATE_UNITS) {
                    network_ablate_units(tmp, ll / 100.0);
                }
                else if (damage_graph_damage_type == LESION_SCALE_WEIGHTS) {
                    network_scale_weights(tmp, ll);
                }
                else {
                    fprintf(stdout, "WARNING: Unknown damage type!\n");
                }

                if (damage_graph_id == 0) { // Naming: Animals versus Artifacts
                    double an_err_tmp, art_err_tmp;
                    test_naming(tmp, xg->pattern_set, &an_err_tmp, &art_err_tmp);
                    an_err_sum += an_err_tmp;
                    art_err_sum += art_err_tmp;
                }
                else { // Lambon Ralph graphs:
                    // Do nothing
                }
                network_destroy(tmp);
            }
            an_err = an_err_sum / (double) MAX_REPS;
            art_err = art_err_sum / (double) MAX_REPS;
            save_results_to_graph(num_net, i, ll, an_err, art_err);

            // Keep track of area under each curve we can calculate the 95% CI of difference:
            if (i == 0) {
                an_area = 0;
                art_area = 0;
            }
            else {
                an_area += (an_err + prev_an_err) * interval_width / 2.0;
                art_area += (art_err + prev_art_err) * interval_width / 2.0;
            }
            prev_an_err = an_err;
            prev_art_err = art_err;

        }

#ifdef SAVE_DOMAIN_ACCURACY
        g_snprintf(filename, 128, "NetworkStatistics/%s_%s_domain_accuracy.dat", xg->pattern_set_name, damage_prefix[damage_graph_damage_type]);
        if (num_net == 0) {
            fp = fopen(filename, "w");
            fprintf(fp, "N\tAnArea\tArtArea\tDiff\n");
        }
        else {
            fp = fopen(filename, "a");
        }
        fprintf(fp, "%d\t%f\t%f\t%f\n", num_net, an_area, art_area, art_area - an_area);
        fclose(fp);
#endif

        network_destroy(my_net);
        lesion_viewer_repaint(xg);
        gtkx_flush_events();

	if (damage_graph_paused) {
	  num_net = MAX_NETWORKS;
	}
        fprintf(stdout, "\n");
    }
}

/******************************************************************************/

#ifdef INDIVIDUAL_DIFF_GRAPHS

static void callback_run_individual_differences(GtkWidget *button, XGlobals *xg)
{
    char filename[64];
    Network *tmp, *my_net;
    int individuals = 20;
    int i, j, k;

    damage_graph_paused = FALSE;

    pattern_file_load_from_folder(xg, damage_graph_folder_name);
    for (j = 0; j < individuals; j++) {
        damage_graph_repetitions = 0;
        fprintf(stdout, "Network %3d of %3d ...", j+1, individuals); fflush(stdout);

        // Load weights:
        g_snprintf(filename, 128, "DataFiles/%s/%02d.wgt", damage_graph_folder_name, j+1);

        weight_file_read(xg, filename);
        my_net = network_copy(xg->net);

        for (k = 0; k < 25; k++) {
            damage_graph_repetitions++;
            damage_graph_data->dataset[0].points = MAX_POINTS;
            damage_graph_data->dataset[1].points = MAX_POINTS;

            for (i = 0; i < MAX_POINTS; i++) {
                double ll;
                tmp = network_copy(my_net);

                if (damage_graph_damage_type == LESION_SEVER_WEIGHTS) {
                    ll = 100 * i * MAX_SEVER / (double) (MAX_POINTS - 1);
                    network_sever_weights(tmp, ll / 100.0);
                }
                else if (damage_graph_damage_type == LESION_PERTURB_WEIGHTS) {
                    ll = i * MAX_PERTURB / (double) (MAX_POINTS - 1);
                    network_perturb_weights(tmp, ll);
                }
                else if (damage_graph_damage_type == LESION_ABLATE_UNITS) {
                    ll = 100 * i * MAX_ABLATE / (double) (MAX_POINTS - 1);
                    network_ablate_units(tmp, ll / 100.0);
                }
                else if (damage_graph_damage_type == LESION_SCALE_WEIGHTS) {
                    ll = MIN_SCALE - (MIN_SCALE-MAX_SCALE) * (i / (double) (MAX_POINTS - 1));
                    network_scale_weights(tmp, ll);
                }
                else {
                    ll = 0.0;
                    fprintf(stdout, "WARNING: Unknown damage type!\n");
                }

                if (damage_graph_id == 0) { // Naming: Animals versus Artifacts
                    test_naming(tmp, xg->pattern_set, k, i, ll);
                }
                else { // Lambon Ralph et al. (2007) graphs:
                    // Do nothing
                }
                network_destroy(tmp);
            }
            lesion_viewer_repaint(xg);
            gtkx_flush_events();
    	    if (damage_graph_paused) {
                j = individuals; k = MAX_NETWORKS;
            }
        }
        network_destroy(my_net);
        fprintf(stdout, "\n");

        // Write the graph to a file (if it is legitimate):
        if (j < individuals) {
            char prefix[128];

            g_snprintf(prefix, 128, "%s_%02d_%s", xg->pattern_set_name, j+1, damage_prefix[damage_graph_damage_type]);
            save_graph_to_image_file(xg, 700, 500, prefix);
        }
    }
}

#endif

/******************************************************************************/

static Boolean callback_lesion_viewer_repaint(GtkWidget *widget, GdkEventConfigure *event, XGlobals *xg)
{
    return(lesion_viewer_repaint(xg));
}

/*----------------------------------------------------------------------------*/

static void callback_lesion_graph_pause(GtkWidget *button, XGlobals *xg)
{
    damage_graph_paused = TRUE;
    lesion_viewer_repaint(xg);
}

/*----------------------------------------------------------------------------*/

static void callback_lesion_graph_restart(GtkWidget *button, XGlobals *xg)
{
    int i;

    damage_graph_repetitions = 0;
    for (i = 0; i < 20; i++) {
        damage_graph_data->dataset[2*i].points = 0;
        damage_graph_data->dataset[2*i+1].points = 0;
    }
    lesion_viewer_repaint(xg);
}

/*----------------------------------------------------------------------------*/

static void callback_lesion_graph_step(GtkWidget *button, XGlobals *xg)
{
    if (damage_graph_id == 0) {
        generate_lesion_data(xg, 1);
    }
}

/*----------------------------------------------------------------------------*/

static void callback_lesion_graph_run(GtkWidget *button, XGlobals *xg)
{
    if (damage_graph_id == 0) {
        generate_lesion_data(xg, 20);
    }
}

/*----------------------------------------------------------------------------*/

static void callback_toggle_colour(GtkWidget *button, XGlobals *xg)
{
    damage_graph_colour = (Boolean) (GTK_TOGGLE_BUTTON(button)->active);
    lesion_viewer_repaint(xg);
}

/*----------------------------------------------------------------------------*/

static void reset_graph_data()
{
    int i;

    for (i = 0; i < 20; i++) {
        damage_graph_data->dataset[2*i].points = 0;
        damage_graph_data->dataset[2*i+1].points = 0;
    }

    if (damage_graph_id == 0) {
        graph_set_axis_properties(damage_graph_data, GTK_ORIENTATION_VERTICAL, 0, 1.0, 11, "%4.2f", "Proportion Correct");
        if (damage_graph_damage_type == LESION_SEVER_WEIGHTS) {
            graph_set_axis_properties(damage_graph_data, GTK_ORIENTATION_HORIZONTAL, 0, 100*MAX_SEVER, 11, "%2.0f%%", "Percentage of severed connections");
        }
        else if (damage_graph_damage_type == LESION_PERTURB_WEIGHTS) {
            graph_set_axis_properties(damage_graph_data, GTK_ORIENTATION_HORIZONTAL, 0, MAX_PERTURB, 11, "%3.1f", "Noise on weights (SD)");
        }
        else if (damage_graph_damage_type == LESION_ABLATE_UNITS) {
            graph_set_axis_properties(damage_graph_data, GTK_ORIENTATION_HORIZONTAL, 0, 100*MAX_ABLATE, 11, "%2.0f%%", "Probability of removal of each hub unit");
        }
        else if (damage_graph_damage_type == LESION_SCALE_WEIGHTS) {
            graph_set_axis_properties(damage_graph_data, GTK_ORIENTATION_HORIZONTAL, MIN_SCALE, MAX_SCALE, 11, "%4.2f%", "Weight scaling factor");
        }

        if (damage_graph_reload == RELOAD_SAMPLE_P4_CLOUD) {
            int i;
            // We only put a legend label on the first dataset of each type:
            graph_set_dataset_properties(damage_graph_data, 0, "Animals", 1.0, 0.0, 0.0, 0.5, 0, LS_NONE, MARK_CIRCLE_FILLED);
            graph_set_dataset_properties(damage_graph_data, 1, "Artefacts", 0.0, 0.0, 1.0, 0.5, 0, LS_NONE, MARK_TRIANGLE_FILLED);
            for (i = 1; i < 20; i++) {
                graph_set_dataset_properties(damage_graph_data, 2*i, NULL, 1.0, 0.0, 0.0, 0.5, 0, LS_NONE, MARK_CIRCLE_FILLED);
                graph_set_dataset_properties(damage_graph_data, 2*i+1, NULL, 0.0, 0.0, 1.0, 0.5, 0, LS_NONE, MARK_TRIANGLE_FILLED);
            }
        }
        else {
            graph_set_dataset_properties(damage_graph_data, 0, "Animals", 1.0, 0.0, 0.0, 1.0, 0, LS_SOLID, MARK_CIRCLE_FILLED);
            graph_set_dataset_properties(damage_graph_data, 1, "Artefacts", 0.0, 0.0, 1.0, 1.0, 0, LS_DOTTED, MARK_CIRCLE_OPEN);
        }
    }
    else {  // Lambon Ralph et al (2007) data
        graph_set_axis_properties(damage_graph_data, GTK_ORIENTATION_VERTICAL, 0, 1.0, 11, "%4.2f", "Proportion Correct");
        if (damage_graph_damage_type == LESION_SEVER_WEIGHTS) {
            graph_set_axis_properties(damage_graph_data, GTK_ORIENTATION_HORIZONTAL, 0, 2, 0, NULL, "Severed\nConnections");
        }
        else if (damage_graph_damage_type == LESION_PERTURB_WEIGHTS) {
            graph_set_axis_properties(damage_graph_data, GTK_ORIENTATION_HORIZONTAL, 0, 2, 0, NULL, "Perturbed\nWeights");
        }
        else if (damage_graph_damage_type == LESION_ABLATE_UNITS) {
            graph_set_axis_properties(damage_graph_data, GTK_ORIENTATION_HORIZONTAL, 0, 2, 0, NULL, "Ablated\nUnits");
        }
        else if (damage_graph_damage_type == LESION_SCALE_WEIGHTS) {
            graph_set_axis_properties(damage_graph_data, GTK_ORIENTATION_HORIZONTAL, 0, 2, 0, NULL, "Weight\nScaling");
        }
        graph_set_dataset_properties(damage_graph_data, 0, "Animals", 1.0, 0.0, 0.0, 1.0, 30, LS_SOLID, MARK_NONE);
        graph_set_dataset_properties(damage_graph_data, 1, "Artefacts", 0.0, 0.0, 1.0, 1.0, 30, LS_SOLID, MARK_NONE);
        if (damage_graph_damage_type == LESION_SEVER_WEIGHTS) {
            damage_graph_data->dataset[0].points = 1; 
            damage_graph_data->dataset[0].x[0] = 1; 
            damage_graph_data->dataset[0].y[0] = 0.370;
            damage_graph_data->dataset[0].se[0] = 0.122; 

            damage_graph_data->dataset[1].points = 1; 
            damage_graph_data->dataset[1].x[0] = 1; 
            damage_graph_data->dataset[1].y[0] = 0.344; 
            damage_graph_data->dataset[1].se[0] = 0.113; 
        }
        else if (damage_graph_damage_type == LESION_PERTURB_WEIGHTS) {
            damage_graph_data->dataset[0].points = 1; 
            damage_graph_data->dataset[0].x[0] = 1; 
            damage_graph_data->dataset[0].y[0] = 0.647; 
            damage_graph_data->dataset[0].se[0] = 0.088; 

            damage_graph_data->dataset[1].points = 1; 
            damage_graph_data->dataset[1].x[0] = 1; 
            damage_graph_data->dataset[1].y[0] = 0.801; 
            damage_graph_data->dataset[1].se[0] = 0.070; 
        }
        else { // No other data sets
            damage_graph_data->dataset[0].points = 0; 
            damage_graph_data->dataset[1].points = 0; 
        }
    }

    damage_graph_repetitions = 0;
}

/*----------------------------------------------------------------------------*/

static void callback_select_weights(GtkWidget *cb, XGlobals *xg)
{
    damage_graph_reload = gtk_combo_box_get_active(GTK_COMBO_BOX(cb));
    if (damage_graph_folder_name != NULL) {
        free(damage_graph_folder_name);
    }
    damage_graph_folder_name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(cb));
    reset_graph_data();
    lesion_viewer_repaint(xg);
}

/*----------------------------------------------------------------------------*/

static void callback_select_damage_type(GtkWidget *combobox, XGlobals *xg)
{
    damage_graph_damage_type = (LesionType) gtk_combo_box_get_active(GTK_COMBO_BOX(combobox));
    reset_graph_data();
    lesion_viewer_repaint(xg);
}

/*----------------------------------------------------------------------------*/

static void callback_select_graph(GtkWidget *combobox, XGlobals *xg)
{
    damage_graph_id = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox));
    reset_graph_data();
    lesion_viewer_repaint(xg);
}

/*----------------------------------------------------------------------------*/

static char *psn_short_name(char *psn)
{
    // Get a pattern set's short name
    int i = strlen(psn);

    while ((i > 0) && (psn[i] != ' ')) {
        i--;
    }

    if (psn[i] == ' ') {
        return(&psn[i+1]);
    }
    else {
        return(psn);
    }
}

static void callback_print_graph(GtkWidget *button, XGlobals *xg)
{
    int width, height;
    char prefix[128];

    if (damage_graph_id == 0) {
        g_snprintf(prefix, 128, "%s_%03d_%s", psn_short_name(xg->pattern_set_name), xg->net->hidden_width, damage_prefix[damage_graph_damage_type]);
        width = 188*4; height = 500;
    }
    else if (damage_graph_id == 1) {
        g_snprintf(prefix, 128, "%s_%s", "lambon_ralph_results", damage_prefix[damage_graph_damage_type]);
        width = 188; height = 500;
    }
    else {
        // Should never happen...
        g_snprintf(prefix, 128, "%s_%s", "unknown", damage_prefix[damage_graph_damage_type]);
        width = 188*4; height = 500;
    }

    save_graph_to_image_file(xg, width, height, prefix);
}

/*----------------------------------------------------------------------------*/

static void lesion_viewer_destroy(GtkWidget *viewer1, XGlobals xg)
{
    graph_destroy(damage_graph_data);
    damage_graph_data = NULL;
    damage_graph_viewer = NULL;
}

/*----------------------------------------------------------------------------*/

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

/******************************************************************************/

void hub_lesion_create_widgets(GtkWidget *page, XGlobals *xg)
{
    GtkWidget *hbox, *tmp;

    graph_destroy(damage_graph_data);
    if ((damage_graph_data = graph_create(40)) != NULL) {
        CairoxFontProperties *fp;

        graph_set_margins(damage_graph_data, 62, 26, 30, 45);
        fp = font_properties_create("Sans", 22, PANGO_STYLE_NORMAL, PANGO_WEIGHT_NORMAL, "black");
        graph_set_title_font_properties(damage_graph_data, fp);
        font_properties_set_size(fp, 18);
        graph_set_legend_font_properties(damage_graph_data, fp);
        graph_set_axis_font_properties(damage_graph_data, GTK_ORIENTATION_HORIZONTAL, fp);
        graph_set_axis_font_properties(damage_graph_data, GTK_ORIENTATION_VERTICAL, fp);
        font_properties_destroy(fp);
    }

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    /* Widgets for setting graph type: */
    tmp = gtk_label_new("Graph:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    tmp = gtk_combo_box_new_text();
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Naming: Animals versus Artifacts");
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Naming: Lambon Ralph et al. (2007)");
    // Possibly add more combo options here 
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), damage_graph_id);
    g_signal_connect(G_OBJECT(tmp), "changed", G_CALLBACK(callback_select_graph), xg);
    gtk_widget_show(tmp);

    /*--- Filler: ---*/
    tmp = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 0);
    gtk_widget_show(tmp);

    /* Widgets for setting lesion type: */
    tmp = gtk_label_new("Damage:");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);
    tmp = gtk_combo_box_new_text();
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Sever");
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Perturb");
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Ablate");
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), "Scale");
    g_signal_connect(G_OBJECT(tmp), "changed", G_CALLBACK(callback_select_damage_type), xg);
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), damage_graph_damage_type);
    gtk_widget_show(tmp);

    /*--- Filler: ---*/
    tmp = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 0);
    gtk_widget_show(tmp);

    /* Widgets for setting whether the current net is used, or new ones generated: */
    tmp = gtk_label_new("Weights: ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
    tmp = gtk_combo_box_new_text();
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), ": Fixed");      // RELOAD_FIXED
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), ": Regenerate"); // RELOAD_REGENERATE
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), ": Sample P4 (line)");  // RELOAD_SAMPLE_P4_LINE
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp), ": Sample P4 (cloud)");  // RELOAD_SAMPLE_P4_CLOUD
    combo_box_append_folder_names(tmp, "DataFiles");
    g_signal_connect(G_OBJECT(tmp), "changed", G_CALLBACK(callback_select_weights), xg);
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), damage_graph_reload);
    gtk_widget_show(tmp);

    /*--- Spacer: ---*/
    tmp = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    /* Widgets for setting colour/greyscale: */
    tmp = gtk_label_new("Colour: ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);
    tmp = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), damage_graph_colour);
    gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(tmp), "toggled", G_CALLBACK(callback_toggle_colour), xg);
    gtk_widget_show(tmp);

    /*--- Filler: ---*/
    tmp = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX(hbox), tmp, TRUE, TRUE, 0);
    gtk_widget_show(tmp);

#ifdef INDIVIDUAL_DIFF_GRAPHS
    gtkx_stock_image_button_create(hbox, GTK_STOCK_REFRESH, G_CALLBACK(callback_run_individual_differences), xg);
#endif

    /*--- Toolbar-like generate/stop/print buttons: ---*/
    gtkx_stock_image_button_create(hbox, GTK_STOCK_GOTO_FIRST, G_CALLBACK(callback_lesion_graph_restart), xg);
    gtkx_stock_image_button_create(hbox, GTK_STOCK_GO_FORWARD, G_CALLBACK(callback_lesion_graph_step), xg);
    gtkx_stock_image_button_create(hbox, GTK_STOCK_JUMP_TO, G_CALLBACK(callback_lesion_graph_run), xg);
    gtkx_stock_image_button_create(hbox, GTK_STOCK_MEDIA_PAUSE, G_CALLBACK(callback_lesion_graph_pause), xg);
    gtkx_stock_image_button_create(hbox, GTK_STOCK_PRINT, G_CALLBACK(callback_print_graph), xg);

    /*--- Horizontal separator and the drawing area: ---*/
    tmp = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(page), tmp, FALSE, FALSE, 0);
    gtk_widget_show(tmp);

    damage_graph_viewer = gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(damage_graph_viewer), "expose_event", G_CALLBACK(callback_lesion_viewer_repaint), xg);
    g_signal_connect(G_OBJECT(damage_graph_viewer), "destroy", G_CALLBACK(lesion_viewer_destroy), xg);
    gtk_box_pack_start(GTK_BOX(page), damage_graph_viewer, TRUE, TRUE, 0);
    gtk_widget_show(damage_graph_viewer);

    // Erase existing data:
    reset_graph_data();
}

/******************************************************************************/
