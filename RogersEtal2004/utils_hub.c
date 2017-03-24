/*******************************************************************************

    File:       utils_hub.c
    Contents:   Utility functions for training feedforward / recurrent nets
    Author:     Rick Cooper
    Copyright (c) 2016 Richard P. Cooper

    Public procedures:

Why isn't this lib_network.c???

*******************************************************************************/

#include "hub.h"
#include "lib_maths.h"
#include <ctype.h>
#include <string.h>
#include "lib_string.h"

#define BIAS -2.0
#define DEBUG TRUE

// These need to be lower case for compatability with PDPTOOL...
char *net_ef_label[EF_MAX] = {"sse", "cee", "sme"};
char *net_wu_label[WU_MAX] = {"pattern", "epoch"};

typedef struct clamped_pattern_list {
    double name_features[NUM_NAME];
    double verbal_features[NUM_VERBAL];
    double visual_features[NUM_VISUAL];
    ClampType clamp;
    struct clamped_pattern_list *next;
} ClampedPatternList;

/******************************************************************************/

int pattern_list_length(PatternList *list)
{
    int l = 0;
    while (list != NULL) {
        list = list->next;
        l++;
    }
    return(l);
}

void hub_pattern_set_free(PatternList *list)
{
    while (list != NULL) {
        PatternList *next = list->next;
        g_free(list->name);
        free(list);
        list = next;
    }
}

/******************************************************************************/

static Boolean scan_pattern_name(FILE *fp, PatternList *p)
{
    // Input file format is like:
    // p_one(2).name = 'sparrow';
    // Return TRUE on success

    char prefix[64], name[64];
    int i, l;

    if (fscanf(fp, "%s = %s", prefix, name) <= 0) {
        return(FALSE);
    }
    else if (((l = strlen(prefix)) < 4) || (strcmp(&prefix[l-4], "name") != 0)) {
        fprintf(stdout, "Syntax error while reading pattern name from pattern file (missing 'name' designation in %s)\n", prefix);
        return(FALSE);
    }
    else {
        // name should be something like 'sparrow';
        // We want to ignore the first character and the last two characters
        l = strlen(name) - 2;
        if ((p->name = (char *)malloc(sizeof(char) * l)) == NULL) {
            return(FALSE);
        }
        else {
            for (i = 1; i < l; i++) {
                p->name[i-1] = name[i];
            }
            p->name[l-1] = '\0';
            return(TRUE);
        }
    }
}

/*----------------------------------------------------------------------------*/

static Boolean scan_pattern_category(FILE *fp, PatternList *p)
{
    // Input file format is like:
    // p_one(2).category = 'BIRD';
    // Return TRUE on success

    char prefix[64], category[64];
    int l;

    if (fscanf(fp, "%s = %s", prefix, category) <= 0) {
        return(FALSE);
    }
    else if (((l = strlen(prefix)) < 8) || (strcmp(&prefix[l-8], "category") != 0)) {
        fprintf(stdout, "Syntax error while reading pattern category from pattern file (malformed 'category' designation in %s)\n", prefix);
        return(FALSE);
    }
    else if (strcmp(category, "'BIRD';") == 0) {
        p->category = CAT_BIRD;
    }
    else if (strcmp(category, "'MAMMAL';") == 0) {
        p->category = CAT_MAMMAL;
    }
    else if (strcmp(category, "'FRUIT';") == 0) {
        p->category = CAT_FRUIT;
    }
    else if (strcmp(category, "'HOUSEHOLD';") == 0) {
        p->category = CAT_HOUSEHOLD;
    }
    else if (strcmp(category, "'VEHICLE';") == 0) {
        p->category = CAT_VEHICLE;
    }
    else if (strcmp(category, "'TOOL';") == 0) {
        p->category = CAT_TOOL;
    }
    else {
        fprintf(stdout, "Warning: Unknown category: %s\n", category);
    }
    return(TRUE);
}

/*----------------------------------------------------------------------------*/

static Boolean scan_feature_vector(FILE *fp, char *fname, int num_f, double *val_f)
{
    // Input file format is like:
    // p_one(2).name_features = [1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0];
    // Return TRUE on success

    char prefix[64], postfix[64];
    int i, l, val;

    i = strlen(fname);

    if (fscanf(fp, "%s = [", prefix) <= 0) {
        return(FALSE);
    }
    else if (((l = strlen(prefix)) < i) || (strcmp(&prefix[l-i], fname) != 0)) {
        fprintf(stdout, "Syntax error while reading feature name from pattern file (missing '%s' designation in %s)\n", fname, prefix);
        return(FALSE);
    }
    else {
        for (i = 0; i < num_f; i++) {
            if (fscanf(fp, "%d", &val) <= 0) {
                fprintf(stdout, "Syntax error while reading feature value from pattern file - malformed value\n");
                return(FALSE);
            }
            else {
                val_f[i] = val;
            }
        }
        if (fscanf(fp, "%s", postfix) <= 0) {
            fprintf(stdout, "Syntax error while reading feature value from pattern file - malformed termination (%s)\n", postfix);
            return(FALSE);
        }
        return(TRUE);
    }
}

/*----------------------------------------------------------------------------*/

static PatternList *pattern_read_from_fp(FILE *fp)
{
    /* Read a single pattern from fp, or return NULL if EOF of failure for some other reason */

    PatternList *p = NULL;

    // Input file format is like:
    // p_one(2).name = 'sparrow';
    // p_one(2).category = 'BIRD';
    // p_one(2).name_features = [1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0];
    // p_one(2).verbal_features = [1 0 1 1 1 0 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 0 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 0 0 0 0 0 0 1 0 0 0 0 1 0];
    // p_one(2).visual_features = [1 1 1 1 1 1 1 1 1 1 1 0 1 1 1 0 0 0 0 0 0 0 0 1 1 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0];

    if ((p = (PatternList *)malloc(sizeof(PatternList))) == NULL) {
        fprintf(stdout, "Allocation failure in %s at %d\n", __FUNCTION__, __LINE__);
        return(NULL);
    }
    else {
        Boolean failed = FALSE;

        failed = failed || !scan_pattern_name(fp, p);
        failed = failed || !scan_pattern_category(fp, p);
        failed = failed || !scan_feature_vector(fp, "name_features", NUM_NAME, p->name_features);
        failed = failed || !scan_feature_vector(fp, "verbal_features", NUM_VERBAL, p->verbal_features);
        failed = failed || !scan_feature_vector(fp, "visual_features", NUM_VISUAL, p->visual_features);

        if (failed) {
            free(p);
            return(NULL);
        }
        else {
            return(p);
        }
    }
}

/*----------------------------------------------------------------------------*/

PatternList *hub_pattern_set_read(char *filename)
{
    PatternList *list = NULL, *p, *q;
    FILE *fp;

    if ((fp = fopen(filename, "r")) == NULL) {
        fprintf(stdout, "Failed to read pattern file (%s)\n", filename);
    }
    else {
        while ((p = pattern_read_from_fp(fp)) != NULL) {
            /* Assemble new patterns at the head of the list */
            p->next = list;
            list = p;
        }

        if (list != NULL) {
            /* And reverse the pattern list to get the original order as on file */
            p = list->next;
            list->next = NULL;
            while (p != NULL) {
                q = p->next;
                p->next = list;
                list = p;
                p = q;
            }
        }
    }
    return(list);
}

/******************************************************************************/

Boolean pattern_is_animal(PatternList *p)
{
    return((p->category == CAT_BIRD) || (p->category == CAT_MAMMAL));
}

Boolean pattern_is_artifact(PatternList *p)
{
    return((p->category == CAT_TOOL) || (p->category == CAT_VEHICLE) || (p->category == CAT_HOUSEHOLD));
}

double pattern_get_distance(PatternList *best, double *vector)
{
    double ssq = 0;
    int i;

    if (best != NULL) {
        for (i = 0; i < NUM_NAME; i++) {
            ssq += squared(best->name_features[i] - vector[i]);
        }
        for (i = 0; i < NUM_VERBAL; i++) {
            ssq += squared(best->verbal_features[i] - vector[i+NUM_NAME]);
        }
        for (i = 0; i < NUM_VISUAL; i++) {
            ssq += squared(best->visual_features[i] - vector[i+NUM_NAME+NUM_VERBAL]);
        }
    }
    return(sqrt(ssq));
}

double pattern_get_max_bit_error(PatternList *best, double *vector)
{
    double me = 0.0;
    int i;

    if (best != NULL) {
        for (i = 0; i < NUM_NAME; i++) {
            me = MAX(me, fabs(best->name_features[i] - vector[i]));
        }
        for (i = 0; i < NUM_VERBAL; i++) {
            me = MAX(me, fabs(best->verbal_features[i] - vector[i+NUM_NAME]));
        }
        for (i = 0; i < NUM_VISUAL; i++) {
            me = MAX(me, fabs(best->visual_features[i] - vector[i+NUM_NAME+NUM_VERBAL]));
        }
    }
    return(me);
}

/*----------------------------------------------------------------------------*/

PatternList *pattern_list_get_best_feature_match(PatternList *patterns, double *vector)
{
    if (patterns != NULL) {
        double d, d2;
        PatternList *best, *p;

        best = patterns;
        d = pattern_get_distance(best, vector);
        for (p = patterns->next; p != NULL; p = p->next) {
            d2 = pattern_get_distance(p, vector);
            if (d2 < d) {
                best = p;
                d = d2;
            }
        }
        return(best);
    }
    else {
        return(NULL);
    }
}

#define NAME_THRESHOLD  0.5

PatternList *pattern_list_get_best_name_match(PatternList *patterns, double *vector)
{
    if (patterns != NULL) {
        int i, j;

        j = 0; // J is the index of the best name unit
        for (i = 1; i < NUM_NAME; i++) {
            if (vector[i] > vector[j]) {
                j = i;
            }
        }
        if (vector[j] < NAME_THRESHOLD) {
            return(NULL);
        }
        else {
            PatternList *p, *best = NULL;
            double d, d2;

            for (p = patterns; p != NULL; p = p->next) {
                if (p->name_features[j] == 1.0) {
                    if (best == NULL) {
                        d = pattern_get_distance(p, vector);
                        best = p;
                    }
                    else if ((d2 = pattern_get_distance(p, vector)) < d) {
                        d = d2;
                        best = p;
                    }
                }
            }
            return(best);
        }
    }
    else {
        return(NULL);
    }
}

/******************************************************************************/
/* Create/destroy/copy a network: *********************************************/
/******************************************************************************/

void network_destroy(Network *n)
{
    /* Generalised Version (FF and SRN): */

    if (n != NULL) {
        if (n->weights_ih != NULL) { free(n->weights_ih); }
        if (n->weights_hh != NULL) { free(n->weights_hh); }
        if (n->weights_ho != NULL) { free(n->weights_ho); }
        if (n->units_in != NULL) { free(n->units_in); }
        if (n->units_hidden != NULL) { free(n->units_hidden); }
        if (n->units_hidden_prev != NULL) { free(n->units_hidden_prev); }
        if (n->units_out != NULL) { free(n->units_out); }
        if (n->tmp_ih_deltas != NULL) { free(n->tmp_ih_deltas); }
        if (n->tmp_hh_deltas != NULL) { free(n->tmp_hh_deltas); }
        if (n->tmp_ho_deltas != NULL) { free(n->tmp_ho_deltas); }
        if (n->previous_ih_deltas != NULL) { free(n->previous_ih_deltas); }
        if (n->previous_hh_deltas != NULL) { free(n->previous_hh_deltas); }
        if (n->previous_ho_deltas != NULL) { free(n->previous_ho_deltas); }
        free(n);
    }
}

/*----------------------------------------------------------------------------*/

// Empirical work suggests that the initial weight distribution does make a
// difference. To obtain results like those in Tyler et al., initial weights 
// need to be very small (e.g., in the range -0.01 to +0.01). Larger initial 
// weights don't completely nullify the reported effects, but they can blunt 
// them so they are less significant.

static double initial_weight_distribution(double weight_noise)
{
    // Could possibly alternatively use a zero-centered normal distribution
    return(random_uniform(-weight_noise, weight_noise));
}

void network_initialise_weights(Network *net)
{
    int i, j;

    if (net->weights_ih != NULL) {
        for (i = 0; i < (net->in_width+1); i++) {
            for (j = 0; j < net->hidden_width; j++) {
                net->weights_ih[i * net->hidden_width + j] = initial_weight_distribution(net->params.wn);
            }
        }
    }

    if (net->weights_hh != NULL) {
        for (i = 0; i < net->hidden_width; i++) {
            for (j = 0; j < net->hidden_width; j++) {
                net->weights_hh[i * net->hidden_width + j] = initial_weight_distribution(net->params.wn);
            }
        }
    }

    if (net->weights_ho != NULL) {
        for (i = 0; i < (net->hidden_width+1); i++) {
            for (j = 0; j < net->out_width; j++) {
                net->weights_ho[i * net->out_width + j] = initial_weight_distribution(net->params.wn);
            }
        }
    }

#ifdef BIAS
    if (net->weights_ih != NULL) {
        for (j = 0; j < net->hidden_width; j++) {
            net->weights_ih[net->in_width * net->hidden_width + j] = BIAS;
        }
    }

    if (net->weights_ho != NULL) {
        for (j = 0; j < net->out_width; j++) {
            net->weights_ho[net->hidden_width * net->out_width + j] = BIAS;
        }
    }
#endif 

    net->settled = FALSE;
    net->cycles = 0;
}

Network *network_create(NetworkType nt, int iw, int hw, int ow)
{
    /* Generalised version (for both FF and SRN) */

    Network *n;
    int i, j;

    if ((n = (Network *)malloc(sizeof(Network))) == NULL) {
        return(NULL);
    }
    else {
        n->in_width = iw;
        n->hidden_width = hw;
        n->out_width = ow;
        n->nt = nt;
        n->settled = FALSE;
        n->cycles = 0;

        /* 1. Weights: */

        n->weights_ih = (double *)malloc((n->in_width+1) * n->hidden_width * sizeof(double));

        if (nt == NT_RECURRENT) {
            n->weights_hh = (double *)malloc(n->hidden_width * n->hidden_width * sizeof(double));
        }
        else {
            n->weights_hh = NULL;
        }

        n->weights_ho = (double *)malloc((n->hidden_width+1) * n->out_width * sizeof(double));

        /* 2. Units: */

        n->units_in = (double *)malloc((n->in_width+1) * sizeof(double));
        n->units_hidden = (double *)malloc((n->hidden_width+1) * sizeof(double));
        if (nt == NT_RECURRENT) {
            n->units_hidden_prev = (double *)malloc(n->hidden_width * sizeof(double));
        }
        else {
            n->units_hidden_prev = NULL;
        }
        n->units_out = (double *)malloc(n->out_width * sizeof(double));

        /* 3. Allocate temporary space for use when propagating and training: */

        n->tmp_ih_deltas = (double *)malloc((n->in_width+1) * n->hidden_width * sizeof(double));
        if (nt == NT_RECURRENT) {
            n->tmp_hh_deltas = (double *)malloc(n->hidden_width * n->hidden_width * sizeof(double));
        }
        else {
            n->tmp_hh_deltas = NULL;
        }
        n->tmp_ho_deltas = (double *)malloc((n->hidden_width+1) * n->out_width * sizeof(double));

        n->previous_ih_deltas = (double *)malloc((n->in_width+1) * n->hidden_width * sizeof(double));
        if (nt == NT_RECURRENT) {
            n->previous_hh_deltas = (double *)malloc(n->hidden_width * n->hidden_width * sizeof(double));
        }
        else {
            n->previous_hh_deltas = NULL;
        }
        n->previous_ho_deltas = (double *)malloc((n->hidden_width+1) * n->out_width * sizeof(double));

        /* Initialise the previous_*_deltas, for momentum calculations: */
        for (i = 0; i < (n->in_width+1); i++) {
            for (j = 0; j < n->hidden_width; j++) {
                n->previous_ih_deltas[i * n->hidden_width + j] = 0.0;
            }
        }
        if (nt == NT_RECURRENT) {
            for (i = 0; i < n->hidden_width; i++) {
                for (j = 0; j < n->hidden_width; j++) {
                    n->previous_hh_deltas[i * n->hidden_width + j] = 0.0;
                }
            }
        }
        for (i = 0; i < (n->hidden_width+1); i++) {
             for (j = 0; j < n->out_width; j++) {
                 n->previous_ho_deltas[i * n->out_width + j] = 0.0;
            }
        }
        return(n);
    }
}

void network_parameters_set(Network *n, NetworkParameters *p)
{
    n->params.wn = p->wn;               // Initial weight noise
    n->params.lr = p->lr;               // Learning rate
    n->params.momentum = p->momentum;   // Momentum for learning
    n->params.wd = p->wd;               // Weight decay
    n->params.ui = p->ui;               // Unit initialisation
    n->params.ticks = p->ticks;         // For time average of activation flow
    n->params.st = p->st;               // Settling threshold
    n->params.sc = p->sc;               // Cycles to settle
    n->params.ef = p->ef;               // Error to be minimised
    n->params.wut = p->wut;             // Update by item or by epoch
    n->params.epochs = p->epochs;       // Maximum number of epochs to train
    n->params.criterion = p->criterion; // Training criterion
}

/*----------------------------------------------------------------------------*/

Network *network_copy(Network *n)
{
    /* Generalised Version */

    Network *r;
    int i, j;

    if ((r = (Network *)malloc(sizeof(Network))) == NULL) {
        return(NULL);
    }
    else {
        r->in_width = n->in_width;
        r->hidden_width = n->hidden_width;
        r->out_width = n->out_width;
        r->nt = n->nt;
        r->settled = n->settled;
        r->cycles = n->cycles;

        if ((r->weights_ih = (double *)malloc((r->in_width+1) * r->hidden_width * sizeof(double))) != NULL) {
            for (i = 0; i < (r->in_width+1); i++) {
                for (j = 0; j < r->hidden_width; j++) {
                    r->weights_ih[i * r->hidden_width + j] = n->weights_ih[i * n->hidden_width + j];
                }
            }
        }
        if (n->nt == NT_RECURRENT) {
            /* It's an SRN - copy the recurrent weights: */
            if ((r->weights_hh = (double *)malloc(r->hidden_width * r->hidden_width * sizeof(double))) != NULL) {
                for (i = 0; i < r->hidden_width; i++) {
                    for (j = 0; j < r->hidden_width; j++) {
                        r->weights_hh[i * r->hidden_width + j] = n->weights_hh[i * n->hidden_width + j];
                    }
                }
            }
        }
        else {
            r->weights_hh = NULL;
        }
        if ((r->weights_ho = (double *)malloc((r->hidden_width+1) * r->out_width * sizeof(double))) != NULL) {
            for (i = 0; i < (r->hidden_width+1); i++) {
                for (j = 0; j < r->out_width; j++) {
                    r->weights_ho[i * r->out_width + j] = n->weights_ho[i * n->out_width + j];
                }
            }
        }

        if ((r->units_in = (double *)malloc((r->in_width+1) * sizeof(double))) != NULL) {
            for (i = 0; i < r->in_width; i++) {
                r->units_in[i] = n->units_in[i];
            }
            r->units_in[r->in_width] = 1.0; // The bias unit ... never change this!
        }
        if ((r->units_hidden = (double *)malloc((r->hidden_width+1) * sizeof(double))) != NULL) {
            for (i = 0; i < r->hidden_width; i++) {
                r->units_hidden[i] = n->units_hidden[i];
            }
            r->units_hidden[r->hidden_width] = 1.0; // The bias unit ... never change this!
        }
        if (n->nt == NT_RECURRENT) {
            /* It's an SRN - copy the previous hidden unit values: */
            if ((r->units_hidden_prev = (double *)malloc(r->hidden_width * sizeof(double))) != NULL) {
                for (i = 0; i < r->hidden_width; i++) {
                    r->units_hidden_prev[i] = n->units_hidden_prev[i];
                }
            }
        }
        else {
            r->units_hidden_prev = NULL;
        }
        if ((r->units_out = (double *)malloc(r->out_width * sizeof(double))) != NULL) {
            for (i = 0; i < r->out_width; i++) {
                r->units_out[i] = n->units_out[i];
            }
        }

        /* Allocate temporary space for use when propagating and training: */
        r->tmp_ih_deltas = (double *)malloc((r->in_width+1) * r->hidden_width * sizeof(double));
        if (n->nt == NT_RECURRENT) {
            r->tmp_hh_deltas = (double *)malloc(r->hidden_width * r->hidden_width * sizeof(double));
        }
        else {
            r->tmp_hh_deltas = NULL;
        }
        r->tmp_ho_deltas = (double *)malloc((r->hidden_width+1) * r->out_width * sizeof(double));

        r->previous_ih_deltas = (double *)malloc((r->in_width+1) * r->hidden_width * sizeof(double));
        if (n->nt == NT_RECURRENT) {
            r->previous_hh_deltas = (double *)malloc(r->hidden_width * r->hidden_width * sizeof(double));
        }
        else {
            r->previous_hh_deltas = NULL;
        }
        r->previous_ho_deltas = (double *)malloc((r->hidden_width+1) * r->out_width * sizeof(double));

        /* Initialise the previous_*_deltas, for momentum calculations: */
        for (i = 0; i < (r->in_width+1); i++) {
            for (j = 0; j < r->hidden_width; j++) {
                r->previous_ih_deltas[i * r->hidden_width + j] = 0.0;
            }
        }
        if (n->nt == NT_RECURRENT) {
            for (i = 0; i < r->hidden_width; i++) {
                for (j = 0; j < r->hidden_width; j++) {
                    r->previous_hh_deltas[i * r->hidden_width + j] = 0.0;
                }
            }
        }
        for (i = 0; i < (r->hidden_width+1); i++) {
             for (j = 0; j < r->out_width; j++) {
                r->previous_ho_deltas[i * r->out_width + j] = 0.0;
            }
        }
        network_parameters_set(r, &(n->params));
        return(r);
    }
}

/******************************************************************************/
/***************************** GENERIC UTILITIES ******************************/
/******************************************************************************/

void network_ask_input(Network *n, double *vector)
{
    int i;
    if (n->units_in != NULL) {
        for (i = 0; i < n->in_width; i++) {
            vector[i] = n->units_in[i];
        }
    }
}

void network_ask_hidden(Network *n, double *vector)
{
    int i;

    if (n == NULL) {
        fprintf(stderr, "ERROR: NULL network in %s\n", __FILE__);
    }
    else if (n->units_hidden == NULL) {
        fprintf(stderr, "ERROR: NULL hidden units in %s\n", __FILE__);
    }
    else {
        for (i = 0; i < n->hidden_width; i++) {
            vector[i] = n->units_hidden[i];
        }
    }
}

void network_ask_output(Network *n, double *vector)
{
    int i;

    if (n == NULL) {
        fprintf(stderr, "ERROR: NULL network in %s\n", __FILE__);
    }
    else if (n->units_out == NULL) {
        fprintf(stderr, "ERROR: NULL output units in %s\n", __FILE__);
    }
    else {
        for (i = 0; i < n->out_width; i++) {
            vector[i] = n->units_out[i];
        }
    }
}

static double network_unit_initial_value(Network *n)
{
    if (n->params.ui == UI_FIXED) {
        return(sigmoid(BIAS));
    }
    else {
        return(random_uniform(0.01, 0.99));
    }
}

void network_tell_initialise_input(Network *n)
{
    int i;

    if (n->units_in != NULL) {
        for (i = 0; i < n->in_width; i++) {
            n->units_in[i] = network_unit_initial_value(n);
        }
        n->units_in[n->in_width] = 1.0; // The bias unit ... always 1
    }
}

void network_tell_initialise_hidden(Network *n)
{
    int i;

    if (n->units_hidden != NULL) {
        for (i = 0; i < n->hidden_width; i++) {
            n->units_hidden[i] = network_unit_initial_value(n);
        }

        n->units_hidden[n->hidden_width] = 1.0; // The bias unit ... always 1
    }
    if (n->units_hidden_prev != NULL) {
        for (i = 0; i < n->hidden_width; i++) {
            n->units_hidden_prev[i] = network_unit_initial_value(n);
        }
    }
}

void network_tell_initialise_output(Network *n)
{
    int i;

    if (n->units_out != NULL) {
        for (i = 0; i < n->out_width; i++) {
            n->units_out[i] = network_unit_initial_value(n);
        }
    }
}

void network_inject_noise(Network *n, double variance)
{
    double sd = sqrt(variance);
    int i;

    for (i = 0; i < n->hidden_width; i++) {
        n->units_hidden[i] += random_normal(0, sd);
    }
}

/******************************************************************************/
/******************************************************************************/

void network_sever_weights_ih(Network *n, double severity)
{
    int i, j;

    if (n->weights_ih != NULL) {
// fprintf(stdout, "Sever IH weights at %f severity\n", severity);
        for (i = 0; i < n->in_width; i++) { // Don't lesion the bias!
            for (j = 0; j < n->hidden_width; j++) {
                if (random_uniform(0.0, 1.0) < severity) {
                    n->weights_ih[i * n->hidden_width + j] = 0.0;
                }
            }
        }
#ifdef BIAS_LESION
	i = n->in_width;
        for (j = 0; j < n->hidden_width; j++) {
            if (random_uniform(0.0, 1.0) < severity) {
                n->weights_ih[i * n->hidden_width + j] = 0.0;
            }
        }
#endif
    }
}

void network_sever_weights_hh(Network *n, double severity)
{
    int i, j;

    if (n->weights_hh != NULL) { // For SRN only
// fprintf(stdout, "Sever HH weights at %f severity\n", severity);
        for (i = 0; i < n->hidden_width; i++) { // No hidden to hidden bias to worry about
            for (j = 0; j < n->hidden_width; j++) {
                if (random_uniform(0.0, 1.0) < severity) {
                    n->weights_hh[i * n->hidden_width + j] = 0.0;
                }
            }
        }
    }
}

void network_sever_weights_ho(Network *n, double severity)
{
    int i, j;

    if (n->weights_ho != NULL) {
// fprintf(stdout, "Sever HO weights at %f severity\n", severity);
        for (i = 0; i < n->hidden_width; i++) { // Don't lesion the bias!
            for (j = 0; j < n->out_width; j++) {
                if (random_uniform(0.0, 1.0) < severity) {
                    n->weights_ho[i * n->out_width + j] = 0.0;
                }
            }
        }
#ifdef BIAS_LESION
	i = n->hidden_width;
        for (j = 0; j < n->out_width; j++) {
            if (random_uniform(0.0, 1.0) < severity) {
                n->weights_ho[i * n->out_width + j] = 0.0;
            }
        }
#endif
    }
}

/*----------------------------------------------------------------------------*/

void network_sever_weights(Network *net, double severity)
{
    network_sever_weights_ih(net, severity);
    network_sever_weights_hh(net, severity);
    network_sever_weights_ho(net, severity);
}

/******************************************************************************/

void network_perturb_weights_ih(Network *n, double noise_sd)
{
    int i, j;

    if (n->weights_ih != NULL) {
        for (i = 0; i < n->in_width; i++) { // Don't lesion the bias!
            for (j = 0; j < n->hidden_width; j++) {
                n->weights_ih[i * n->hidden_width + j] += random_uniform(-noise_sd, noise_sd);
                //                n->weights_ih[i * n->hidden_width + j] += random_normal(0, noise_sd);
            }
        }
#ifdef BIAS_LESION
	i = n->in_width;
        for (j = 0; j < n->hidden_width; j++) {
            n->weights_ih[i * n->hidden_width + j] += random_uniform(-noise_sd, noise_sd);
            //                n->weights_ih[i * n->hidden_width + j] += random_normal(0, noise_sd);
        }
#endif
    }
}

void network_perturb_weights_hh(Network *n, double noise_sd)
{
    int i, j;

    if (n->weights_hh != NULL) { // For SRN only
        for (i = 0; i < n->hidden_width; i++) { // No hidden to hidden bias to worry about
            for (j = 0; j < n->hidden_width; j++) {
                n->weights_hh[i * n->hidden_width + j] += random_uniform(-noise_sd, noise_sd);
                //                n->weights_hh[i * n->hidden_width + j] += random_normal(0, noise_sd);
            }
        }
    }
}

void network_perturb_weights_ho(Network *n, double noise_sd)
{
    int i, j;

    if (n->weights_ho != NULL) {
        for (i = 0; i < n->hidden_width; i++) { // Don't lesion the bias!
            for (j = 0; j < n->out_width; j++) {
                n->weights_ho[i * n->out_width + j] += random_uniform(-noise_sd, noise_sd);
                //                n->weights_ho[i * n->out_width + j] += random_normal(0.0, noise_sd);
            }
        }
#ifdef BIAS_LESION
	i = n->hidden_width;
        for (j = 0; j < n->out_width; j++) {
            n->weights_ho[i * n->out_width + j] += random_uniform(-noise_sd, noise_sd);
            //                n->weights_ho[i * n->out_width + j] += random_normal(0.0, noise_sd);
        }
#endif
    }
}

void network_perturb_weights(Network *net, double noise_sd)
{
    network_perturb_weights_ih(net, noise_sd);
    network_perturb_weights_hh(net, noise_sd);
    network_perturb_weights_ho(net, noise_sd);
}

/******************************************************************************/

void network_zero_input(Network *n)
{
    int i;

    if (n->units_in != NULL) {
        for (i = 0; i < n->in_width; i++) {
            n->units_in[i] = 0.0;
        }
    }
}

/*----------------------------------------------------------------------------*/

void network_tell_input(Network *n, double *vector)
{
    /* Generalised Version: */

    int i;

    if (n->units_in != NULL) {
        for (i = 0; i < n->in_width; i++) {
            n->units_in[i] = vector[i];
        }
    }
}

void network_initialise(Network *n)
{ 
    network_tell_initialise_input(n);
    network_tell_initialise_hidden(n);
    network_tell_initialise_output(n);

    /* Initialise the cycle count (only relevant for recurrent networks) */
    n->cycles = 0;
    n->settled = FALSE;
}

/*----------------------------------------------------------------------------*/

void clamp_set_clamp_name(ClampType *clamp, int from, int to, PatternList *p)
{
    int i;

    clamp->from = from;
    clamp->to = to;
    for (i = 0; i < NUM_NAME; i++) {
        clamp->vector[i] = p->name_features[i];
    }
    for (i = 0; i < NUM_VERBAL; i++) {
        clamp->vector[NUM_NAME + i] = -1.0;
    }
    for (i = 0; i < NUM_VISUAL; i++) {
        clamp->vector[NUM_NAME + NUM_VERBAL + i] = -1.0;
    }
}

void clamp_set_clamp_verbal(ClampType *clamp, int from, int to, PatternList *p)
{
    int i;

    clamp->from = from;
    clamp->to = to;
    for (i = 0; i < NUM_NAME; i++) {
        clamp->vector[i] = -1.0;
    }
    for (i = 0; i < NUM_VERBAL; i++) {
        clamp->vector[NUM_NAME + i] = p->verbal_features[i];
    }
    for (i = 0; i < NUM_VISUAL; i++) {
        clamp->vector[NUM_NAME + NUM_VERBAL + i] = -1.0;
    }
}

void clamp_set_clamp_visual(ClampType *clamp, int from, int to, PatternList *p)
{
    int i;

    clamp->from = from;
    clamp->to = to;
    for (i = 0; i < NUM_NAME; i++) {
        clamp->vector[i] = -1.0;
    }
    for (i = 0; i < NUM_VERBAL; i++) {
        clamp->vector[NUM_NAME + i] = -1.0;
    }
    for (i = 0; i < NUM_VISUAL; i++) {
        clamp->vector[NUM_NAME + NUM_VERBAL + i] = p->visual_features[i];
    }
}

void clamp_set_clamp_all(ClampType *clamp, int from, int to, PatternList *p)
{
    int i;

    clamp->from = from;
    clamp->to = to;
    for (i = 0; i < NUM_NAME; i++) {
        clamp->vector[i] = p->name_features[i];
    }
    for (i = 0; i < NUM_VERBAL; i++) {
        clamp->vector[NUM_NAME + i] = p->verbal_features[i];
    }
    for (i = 0; i < NUM_VISUAL; i++) {
        clamp->vector[NUM_NAME + NUM_VERBAL + i] = p->visual_features[i];
    }
}

void network_tell_recirculate_input(Network *net, ClampType *clamp)
{
    double vector_io[NUM_IO];
    int i;

    // Input is the recycled output ...
    network_ask_output(net, vector_io);
    // except we may need to clamp some values:
    if ((net->cycles >= clamp->from) && (net->cycles < clamp->to)) {
        for (i = 0; i < NUM_IO; i++) {
            if (clamp->vector[i] >= 0.0) {
                vector_io[i] = clamp->vector[i];
                // Also clamp the output units so that time-averaging works
                net->units_out[i] = clamp->vector[i];
            }
        }
    }
    network_tell_input(net, vector_io);
}

/*----------------------------------------------------------------------------*/

static double time_average(double new_net, double net, int ticks)
{
    if (ticks == 1) {
        return(new_net);
    }
    else {
        double dt = 1 / (double) ticks;
        return(dt * new_net + (1.0 - dt) * net);
    }
}

void network_tell_propagate(Network *n)
{
    /* Generalised version (FF or SRN) */

    double net_in, new_net_in;
    int i, j;

    /* Propagate from input to hidden: */
    if (n->units_hidden != NULL) {
        for (j = 0; j < n->hidden_width; j++) {
            new_net_in = 0.0;
            net_in = sigmoid_inverse(n->units_hidden[j]);
            for (i = 0; i < (n->in_width+1); i++) {
                new_net_in += n->units_in[i] * n->weights_ih[i * n->hidden_width + j];
            }
            /* If recurrent, then add in recurrent input: */
            if (n->nt == NT_RECURRENT) {
                for (i = 0; i < n->hidden_width; i++) {
                    new_net_in += n->units_hidden_prev[i] * n->weights_hh[i * n->hidden_width + j];
                }
            }
            /* And calculate the post-synaptic value: */
            n->units_hidden[j] = sigmoid(time_average(new_net_in, net_in, n->params.ticks));
        }
    }

    /* Propagate from hidden to output: */
    if (n->units_out != NULL) {
        for (j = 0; j < n->out_width; j++) {
            new_net_in = 0.0;
            net_in = sigmoid_inverse(n->units_out[j]);
            for (i = 0; i < (n->hidden_width+1); i++) {
                new_net_in += n->units_hidden[i] * n->weights_ho[i * n->out_width + j];
            }
            n->units_out[j] = sigmoid(time_average(new_net_in, net_in, n->params.ticks));
        }
    }

    if (n->nt == NT_RECURRENT) {

        // Keep track of the number of cycles:
        n->cycles++;

        if (n->cycles > 1) {
            // Assume that a recurrent network has settled if the difference 
            // between its current and its previous hidden layer is less than
            // some threshold.
            n->settled = (euclidean_distance(n->hidden_width, n->units_hidden, n->units_hidden_prev) < n->params.st);
        }
        else {
            n->settled = FALSE;
        }
        // Copy the new hidden layer for next time:
        for (i = 0; i < n->hidden_width; i++) {
            n->units_hidden_prev[i] = n->units_hidden[i];
        }
    }
    else {
        n->settled = TRUE; /* Irrelevant for nonrecurrent networks */
    }
}

Boolean network_is_settled(Network *net)
{
    if (net->params.sc == 0) {
        return((net->settled) || (net->cycles >= 100));
    }
    else {
        return(net->cycles >= (net->params.sc * net->params.ticks));
    }
}

void network_tell_propagate_full(Network *net, ClampType *clamp)
{
    // propagate activation (until the network settles, or 100 cycles)

    if (net->nt == NT_FEEDFORWARD) {
        network_tell_propagate(net);
    }
    else if (net->nt == NT_RECURRENT) {
        do {
            network_tell_propagate(net);
            network_tell_recirculate_input(net, clamp);
        } while (!network_is_settled(net));
    }
}

/******************************************************************************/
/* SECTION XX: Test the network against a set of patterns *********************/
/******************************************************************************/

double net_error_ssq(double d, double y)
{
    if (d < 0) { // Target is not clamped so return zero error
        return(0);
    }
    else {
        return(d - y);
    }
}

double net_error_cross_entropy(double d, double y)
{
    if (d < 0) { // Target is not clamped so return zero error
        return(0);
    }
    else {
        if (y <= 0) {
            y = 0.00000001;
        }
        if (y >= 1) {
            y = 0.99999999;
        }
        return((d - y) / (y * (1 - y)));
    }
}

double net_error_soft_max(double d, double y)
{
    if (d < 0) { // Target is not clamped so return zero error
        return(0);
    }
    else {
        if (y <= 0) {
            y = 0.00000001;
        }
        return(d  / y); // Based on the LENS code...
    }
}

/*----------------------------------------------------------------------------*/

static void hub_build_target_vector(ClampedPatternList *cl_pat, double *vector)
{
    int i;

    for (i = 0; i < NUM_NAME; i++) {
        vector[i] = cl_pat->name_features[i];
    }
    for (i = 0; i < NUM_VERBAL; i++) {
        vector[NUM_NAME + i] = cl_pat->verbal_features[i];
    }
    for (i = 0; i < NUM_VISUAL; i++) {
        vector[NUM_NAME + NUM_VERBAL + i] = cl_pat->visual_features[i];
    }
}

void hub_build_name_input_vector(PatternList *pattern, double *vector)
{
    int i;

    for (i = 0; i < NUM_NAME; i++) {
        vector[i] = pattern->name_features[i];
    }
    for (i = 0; i < NUM_VERBAL; i++) {
        vector[NUM_NAME + i] = sigmoid(BIAS);
    }
    for (i = 0; i < NUM_VISUAL; i++) {
        vector[NUM_NAME + NUM_VERBAL + i] = sigmoid(BIAS);
    }
}

void hub_build_verbal_input_vector(PatternList *pattern, double *vector)
{
    int i;

    for (i = 0; i < NUM_NAME; i++) {
        vector[i] = sigmoid(BIAS);
    }
    for (i = 0; i < NUM_VERBAL; i++) {
        vector[NUM_NAME + i] = pattern->verbal_features[i];
    }
    for (i = 0; i < NUM_VISUAL; i++) {
        vector[NUM_NAME + NUM_VERBAL + i] = sigmoid(BIAS);
    }
}

void hub_build_visual_input_vector(PatternList *pattern, double *vector)
{
    int i;

    for (i = 0; i < NUM_NAME; i++) {
        vector[i] = sigmoid(BIAS);
    }
    for (i = 0; i < NUM_VERBAL; i++) {
        vector[NUM_NAME + i] = sigmoid(BIAS);
    }
    for (i = 0; i < NUM_VISUAL; i++) {
        vector[NUM_NAME + NUM_VERBAL + i] = pattern->visual_features[i];
    }
}

static void hub_build_input_vector(ClampedPatternList *clp, double *vector)
{
    if (clp != NULL) {
        int i;

        for (i = 0; i < NUM_IO; i++) {
            if ((clp->clamp.vector[i] < 0) || (clp->clamp.from > 0)) {
                vector[i] = sigmoid(BIAS);
            }
            else if (i < NUM_NAME) {
                vector[i] = clp->name_features[i];
            }
            else if (i < NUM_NAME + NUM_VERBAL) {
                vector[i] = clp->verbal_features[i-NUM_NAME];
            }
            else {
                vector[i] = clp->visual_features[i-NUM_NAME-NUM_VERBAL];
            }
        }
    }
    else {
        fprintf(stdout, "Warning: NULL clamp / pattern in %s\n", __FILE__);
    }
}

/******************************************************************************/
/* TRAINING *******************************************************************/
/******************************************************************************/

static int clamped_pattern_list_length(ClampedPatternList *clps)
{
    int l = 0;
    while (clps != NULL) {
        clps = clps->next; l++;
    }
    return(l);
}

static ClampedPatternList *clamped_pattern_list_randomise(ClampedPatternList *clps)
{
    ClampedPatternList *first = NULL;

    if (clps != NULL) {
        ClampedPatternList *prev;
        int i = random_int(clamped_pattern_list_length(clps));

        first = clps;
        while (i-- > 0) {
            prev = first;
            first = first->next;
        }
        if (first != clps) {
            prev->next = NULL;
            /* We now have two NULL-terminated lists, first and clps */
            /* Append clps to the end of first */
            prev = first;
            while (prev->next != NULL) {
                prev = prev->next;
            }
            prev->next = clps;
        }
        first->next = clamped_pattern_list_randomise(first->next);
    }
    return(first);
}

static ClampedPatternList *clamped_pattern_make_item(PatternList *p, int pool)
{
    ClampedPatternList *new;
    int i;

    if ((new = (ClampedPatternList *)malloc(sizeof(ClampedPatternList))) != NULL) {
        for (i = 0; i < NUM_NAME; i++) {
            new->name_features[i] = p->name_features[i];
        }
        for (i = 0; i < NUM_VERBAL; i++) {
            new->verbal_features[i] = p->verbal_features[i];
        }
        for (i = 0; i < NUM_VISUAL; i++) {
            new->visual_features[i] = p->visual_features[i];
        }
        new->clamp.from = 0;
        new->clamp.to = 8;
        for (i = 0; i < NUM_IO; i++) {
            new->clamp.vector[i] = -1;
        }
        if ((pool == 0) || (pool == 3)) { // Clamp name features:
            for (i = 0; i < NUM_NAME; i++) {
                new->clamp.vector[i] = p->name_features[i];
            }
        }
        if ((pool == 1) || (pool == 3)) { // Clamp verbal features:
            for (i = 0; i < NUM_VERBAL; i++) {
                new->clamp.vector[NUM_NAME + i] = p->verbal_features[i];
            }
        }
        if ((pool == 2) || (pool == 3)) { // Clamp name features:
            for (i = 0; i < NUM_VISUAL; i++) {
                new->clamp.vector[NUM_NAME + NUM_VERBAL + i] = p->visual_features[i];
            }
        }
        new->next = NULL;
    }
    return(new);
}

static ClampedPatternList *clamped_pattern_list_prepend(ClampedPatternList *clps, ClampedPatternList *new)
{
    ClampedPatternList *last;

    last = new;
    while (last->next != NULL) {
        last = last->next;
    }
    last->next = clps;
    return(new);
}

static ClampedPatternList *network_generate_randomised_clamped_pattern_set(PatternList *patterns)
{
    ClampedPatternList *clps = NULL;
    PatternList *p;

    // Build the randomised clamped pattern list from the pattern list
    // Each pattern should come with 3 clamps: name, verbal, visual
#if TRUE
    for (p = patterns; p != NULL; p = p->next) {
        clps = clamped_pattern_list_prepend(clps, clamped_pattern_make_item(p, 0));
        clps = clamped_pattern_list_prepend(clps, clamped_pattern_make_item(p, 1));
        clps = clamped_pattern_list_prepend(clps, clamped_pattern_make_item(p, 2));
    }
    return(clamped_pattern_list_randomise(clps));
#else
    // Just train on one pattern: p1 with name clamped
    clps = clamped_pattern_list_prepend(clps, clamped_pattern_make_item(patterns, 0));
    return(clps);
#endif
}

static ClampedPatternList *network_generate_randomised_clamped_all_pattern_set(PatternList *patterns)
{
    ClampedPatternList *clps = NULL;
    PatternList *p;

    // Build the ranomised clamped pattern list from the pattern list
    // Each pattern has all features clamped at the beginning of the trial
    for (p = patterns; p != NULL; p = p->next) {
        clps = clamped_pattern_list_prepend(clps, clamped_pattern_make_item(p, 3));
    }
    return(clamped_pattern_list_randomise(clps));
}

static void network_free_clamped_pattern_list(ClampedPatternList *clamped_patterns)
{
    while (clamped_patterns != NULL) {
        ClampedPatternList *tmp = clamped_patterns;
        clamped_patterns = clamped_patterns->next;
        free(tmp);
    }
}

/*----------------------------------------------------------------------------*/

static void network_train_initialise_deltas(Network *n)
{
    /* Generalised version (FF and SRN): */

    int i, j;

    for (i = 0; i < (n->in_width+1); i++) {
        for (j = 0; j < n->hidden_width; j++) {
            n->tmp_ih_deltas[i * n->hidden_width + j] = 0.0;
        }
    }
    if (n->nt == NT_RECURRENT) {
        for (i = 0; i < n->hidden_width; i++) {
            for (j = 0; j < n->hidden_width; j++) {
                n->tmp_hh_deltas[i * n->hidden_width + j] = 0.0;
            }
        }
    }
    for (i = 0; i < (n->hidden_width+1); i++) {
        for (j = 0; j < n->out_width; j++) {
            n->tmp_ho_deltas[i * n->out_width + j] = 0.0;
        }
    }
}

static void network_train_update_weights(Network *n, double wd)
{
    /* Generalised Version (both FF and SRN):                   */
    /* From Hertz et al., 1991, pp. 116-117 (momentum: p. 123)  */

    double this_delta;
    int i, j;

    for (i = 0; i < (n->in_width+1); i++) {
        for (j = 0; j < n->hidden_width; j++) {
            n->weights_ih[i * n->hidden_width + j] *= (1 - wd);
            this_delta = n->params.lr * n->tmp_ih_deltas[i * n->hidden_width + j] + n->params.momentum * n->previous_ih_deltas[i * n->hidden_width + j];
            n->weights_ih[i * n->hidden_width + j] += this_delta;
            n->previous_ih_deltas[i * n->hidden_width + j] = this_delta;
        }
    }

    if (n->nt == NT_RECURRENT) {
        for (i = 0; i < n->hidden_width; i++) {
            for (j = 0; j < n->hidden_width; j++) {
                n->weights_hh[i * n->hidden_width + j] *= (1 - wd);
                this_delta = n->params.lr * n->tmp_hh_deltas[i * n->hidden_width + j] + n->params.momentum * n->previous_hh_deltas[i * n->hidden_width + j];
                n->weights_hh[i * n->hidden_width + j] += this_delta;
                n->previous_hh_deltas[i * n->hidden_width + j] = this_delta;
            }
        }
    }

    for (i = 0; i < (n->hidden_width+1); i++) {
        for (j = 0; j < n->out_width; j++) {
            n->weights_ho[i * n->out_width + j] *= (1 - wd);
            this_delta = n->params.lr * n->tmp_ho_deltas[i * n->out_width + j] + n->params.momentum * n->previous_ho_deltas[i * n->out_width + j];
            n->weights_ho[i * n->out_width + j] += this_delta;
            n->previous_ho_deltas[i * n->out_width + j] = this_delta;
        }
    }
}

/*----------------------------------------------------------------------------*/

static void network_accumulate_weight_changes_ff(Network *n, double *test_in, double *test_out, double (*net_error_function)())
{
    /* FF Version: From Hertz et al., 1991, pp 116-117. */

    double *delta_ho;
    int i, j, k;

    if ((delta_ho = (double *)malloc(n->out_width * sizeof(double))) == NULL) {
        fprintf(stderr, "ERROR: Allocation failure in %s at %d\n", __FILE__, __LINE__);
        return;
    }

    /* This will change activity in the network ... perhaps we should be */
    /* using our own copy of the network */
    network_tell_input(n, test_in);
    network_tell_propagate(n);

    /* Deltas for hidden to output weights: */
    for (j = 0; j < (n->hidden_width+1); j++) {
        for (i = 0; i < n->out_width; i++) {
            delta_ho[i] = n->units_out[i] * (1 - n->units_out[i]) * net_error_function(test_out[i], n->units_out[i]);
            n->tmp_ho_deltas[j * n->out_width + i] += delta_ho[i] * n->units_hidden[j];
        }
    }

#ifdef BIAS
    for (i = 0; i < n->out_width; i++) {
        n->tmp_ho_deltas[n->hidden_width * n->out_width + i] = 0;
    }
#endif

    /* Deltas for input to hidden weights: */
    for (k = 0; k < (n->in_width+1); k++) {
        for (j = 0; j < n->hidden_width; j++) {
            double sigma_weight_ij_delta_i = 0.0, delta_ih;

            for (i = 0; i < n->out_width; i++) {
                sigma_weight_ij_delta_i += delta_ho[i] * n->weights_ho[j * n->out_width + i];
            }
            delta_ih = n->units_hidden[j] * (1 - n->units_hidden[j]) * sigma_weight_ij_delta_i;
            n->tmp_ih_deltas[k * n->hidden_width + j] += delta_ih * n->units_in[k];
        }
    }

#ifdef BIAS
    for (j = 0; j < n->hidden_width; j++) {
        n->tmp_ih_deltas[n->in_width * n->hidden_width + j] = 0;
    }
#endif

    /* Deallocate temporary space: */
    free(delta_ho);
}

/*----------------------------------------------------------------------------*/

static void network_accumulate_weight_changes_srn(Network *n, double *test_in, double *test_out, ClampType *clamp, double (*net_error_function)())
{
    /* SRN learning: An implementation of epochwise BPTT, based on Williams & Zipser, 1995 */

    int cycles = n->params.ticks * n->params.sc;
    int units = n->hidden_width + n->out_width;
    double *history_hidden = (double *)malloc((cycles+2) * n->hidden_width * sizeof(double));
    double *history_error = (double *)malloc((cycles+2) * n->out_width * sizeof(double));
    double *history_out = (double *)malloc((cycles+2) * n->out_width * sizeof(double));
    double *delta = (double *)malloc((cycles+2) * units * sizeof(double));
    double *epsilon = (double *)malloc((cycles+2) * units * sizeof(double));
    double y, weight_lk, df;
    int i, j, k, l, t;

    Boolean penalty = FALSE;

#if DEBUG
    FILE *fp_dbg = fopen("debug.out", "a");
#endif

    /* 1: Run the network over the entire sequence and collect its state: */

    network_tell_initialise_hidden(n);
    network_tell_input(n, test_in);
    for (t = 0; t < cycles+2; t++) {
        network_ask_hidden(n, &history_hidden[t * n->hidden_width]);
        network_ask_output(n, &history_out[t * n->out_width]);
        network_tell_propagate(n);
        network_tell_recirculate_input(n, clamp);

        for (i = 0; i < n->out_width; i++) {
            // FIXME: Hardwired clamping of output beyond cycle 22
            history_error[t * n->out_width + i] = (t < 22) ? 0.0 : (net_error_function(test_out[i], history_out[t * n->out_width + i]) / (double) n->params.ticks);
        }
    }

#if DEBUG
    for (t = 0; t < cycles+2; t++) {
        fprintf(fp_dbg, "History Output  at t = %d: ", t);
        for (i = 0; i < n->out_width; i++) {
            fprintf(fp_dbg, "%+6.4f ", history_out[t*n->out_width+i]);
        }
        fprintf(fp_dbg, "\n");
    }
    fprintf(fp_dbg, "\n");

    for (t = 0; t < cycles+2; t++) {
        fprintf(fp_dbg, "History Error  at t = %d: ", t);
        for (i = 0; i < n->out_width; i++) {
            fprintf(fp_dbg, "%+6.4f ", history_error[t*n->out_width+i]);
        }
        fprintf(fp_dbg, "\n");
    }
    fprintf(fp_dbg, "\n");
#endif

    /* 2: Calculate epsilon and delta for each non-input unit (equations 17-19): */

    for (t = cycles+1; t > 0; t--) {
        for (k = 0; k < units; k++) {
            /* Calculate the external error for each node: */
            epsilon[t * units + k] = (k < n->hidden_width ? 0.0 : history_error[t * n->out_width + (k - n->hidden_width)]);

            /* Add in the sum of delta terms to epsilon if appropriate: */
            if (t < (cycles+1)) {
                for (l = 0; l < units; l++) {
                    if (k < n->hidden_width) {
                        if (l < n->hidden_width) {
                            /* Weight from hidden unit k to hidden unit l: */
                            weight_lk  = n->weights_hh[k*n->hidden_width+l];
                        }
                        else {
                            /* Weight from hidden unit k to output unit (l-hidden_width): */
                            weight_lk  = n->weights_ho[k*n->out_width+(l-n->hidden_width)];
                        }
                    }
                    else {
                        /* Weight from output unit (k-hidden_width) to unit l: */
                        weight_lk = 0.0;
                    }
                    epsilon[t * units + k] += weight_lk * delta[(t+1) * units + l];
                }
            }
            /* And calculate the deltas... */
            y = (k < n->hidden_width ? history_hidden[t * n->hidden_width + k] : history_out[t * n->out_width + (k - n->hidden_width)]);
            df = y * (1-y);
            if (penalty && (k < n->hidden_width)) {
                df = df + (2 * 0.05 * (history_hidden[t * n->hidden_width + k] - history_hidden[(t-1) * n->hidden_width + k]));
            }
            delta[t * units + k] = epsilon[t * units + k] * df;
        }

#if DEBUG
        fprintf(fp_dbg, "Epsilon at t = %d: ", t);
        for (k = 0; k < units; k++) {
            fprintf(fp_dbg, "%+6.4f ", epsilon[t*units+k]);
        }
        fprintf(fp_dbg, "\n");
        fprintf(fp_dbg, "Delta at t = %d: ", t);
        for (k = 0; k < units; k++) {
            fprintf(fp_dbg, "%+6.4f ", delta[t*units+k]);
        }
        fprintf(fp_dbg, "\n\n");
#endif
    }

    /* 3: Calculate the weight matrix updates (equation 20): */

    for (t = 1; t < (cycles+2); t++) {

        /* 3.1: Input to hidden: */
        for (i = 0; i < n->in_width; i++) {
            for (j = 0; j < n->hidden_width; j++) {
                n->tmp_ih_deltas[i * n->hidden_width + j] += delta[t * units + j] * test_in[i];
            }
        }
#ifndef BIAS
        /* The input bias to hidden weights: */
        for (j = 0; j < n->hidden_width; j++) {
            n->tmp_ih_deltas[n->in_width * n->hidden_width + j] += delta[t * units + j] * 1.0;
        }
#endif
        /* 3.2: Hidden to hidden (remember no bias): */
        for (i = 0; i < n->hidden_width; i++) {
            for (j = 0; j < n->hidden_width; j++) {
                n->tmp_hh_deltas[i * n->hidden_width + j] += delta[t * units + j] * history_hidden[(t-1) * n->hidden_width + i];
            }
        }

        /* 3.3: Hidden to output: */
        for (i = 0; i < n->hidden_width; i++) {
            for (j = 0; j < n->out_width; j++) {
                n->tmp_ho_deltas[i * n->out_width + j] += delta[t * units + n->hidden_width + j] * history_hidden[(t-1) * n->hidden_width + i];
            }
        }
#ifndef BIAS
        /* The hidden bias to output weights: */
        for (j = 0; j < n->out_width; j++) {
            n->tmp_ho_deltas[n->hidden_width * n->out_width + j] += delta[t * units + n->hidden_width + j] * 1.0;
        }
#endif
    }

#if DEBUG
        fprintf(fp_dbg, "Input to hidden adjustments:\n");
        for (i = 0; i < n->in_width+1; i++) {
            for (j = 0; j < n->hidden_width; j++) {
                fprintf(fp_dbg, "%f ", n->tmp_ih_deltas[i * n->hidden_width + j]);
            }
            fprintf(fp_dbg, "\n");
        }
        fprintf(fp_dbg, "\n");
#endif

    /* 4: Release all allocated space: */
    free(history_hidden);
    free(history_out);
    free(history_error);
    free(delta);
    free(epsilon);

#if DEBUG
    fclose(fp_dbg);
#endif
}

/*----------------------------------------------------------------------------*/

static void network_accumulate_weight_changes(Network *n, double *test_in, double *test_out, ClampType *clamp, double (*net_error_function)())
{
    switch (n->nt) {
        case NT_FEEDFORWARD: {
            network_accumulate_weight_changes_ff(n, test_in, test_out, net_error_function);
            break;
        }
        case NT_RECURRENT: {
            network_accumulate_weight_changes_srn(n, test_in, test_out, clamp, net_error_function);
            break;
        }
        case NT_MAX: {
            fprintf(stderr, "WARNING: Invalid network type\n");
            break;
        }
    }
}

Boolean network_train(Network *n, PatternList *patterns)
{
    double (*net_error_function)() = NULL;
    double *vector_in = NULL;
    double *vector_out = NULL;
    ClampedPatternList *clamped_patterns;

    switch (n->params.ef) {
        case EF_SUM_SQUARE: {
            net_error_function = net_error_ssq; break;
        }
        case EF_CROSS_ENTROPY: {
            net_error_function = net_error_cross_entropy; break;
        }
        case EF_SOFT_MAX: {
            net_error_function = net_error_soft_max; break;
        }
        case EF_MAX: {
            net_error_function = NULL; break;
        }
    }

    if ((n->tmp_ih_deltas == NULL) || (n->tmp_ho_deltas == NULL)) {
        return(FALSE);
    }
    if ((vector_in = (double *)malloc(NUM_IO * sizeof(double))) == NULL) {
        return(FALSE);
    }
    if ((vector_out = (double *)malloc(NUM_IO * sizeof(double))) == NULL) {
        return(FALSE);
    }

    /* First generate the randomised clamped training patterns: */
    clamped_patterns = network_generate_randomised_clamped_pattern_set(patterns);

    if (n->params.wut == WU_BY_EPOCH) {

        /* Initialise weight changes: */
        network_train_initialise_deltas(n);

        /* Run the training data, accumulating weight changes over all patterns: */
        while (clamped_patterns != NULL) {
            hub_build_target_vector(clamped_patterns, vector_out);
            hub_build_input_vector(clamped_patterns, vector_in);
            network_accumulate_weight_changes(n, vector_in, vector_out, &(clamped_patterns->clamp), net_error_function);
            clamped_patterns = clamped_patterns->next;
        }

        /* Now update the weights: */
        network_train_update_weights(n, n->params.wd);
    }
    else if (n->params.wut == WU_BY_ITEM) {
        /* Adjust the weight decay rate for item-wise decay */
        int l = pattern_list_length(patterns);
        double wd = 1.0 - exp(log(1.0 - n->params.wd) / (double) l);

        /* Now train with each sequence */
        while (clamped_patterns != NULL) {
            hub_build_target_vector(clamped_patterns, vector_out);

            /* Train on this pattern  */
            hub_build_input_vector(clamped_patterns, vector_in);
            network_train_initialise_deltas(n);
            network_accumulate_weight_changes(n, vector_in, vector_out, &(clamped_patterns->clamp), net_error_function);
            network_train_update_weights(n, wd);

            /* And move on to the next pattern: */
            clamped_patterns = clamped_patterns->next;
        }
    }

    network_free_clamped_pattern_list(clamped_patterns);

    free(vector_in);
    free(vector_out);
    return(TRUE);
}

void network_train_to_criterion(Network *n, PatternList *patterns)
{
    int i;

    for (i = 0; i < n->params.epochs; i++) {
        network_train(n, patterns);

        if (network_test(n, patterns, n->params.ef) < n->params.criterion) {
            return;
        }
    }
}

void network_train_to_epochs(Network *n, PatternList *patterns)
{
    int i;

    for (i = 0; i < n->params.epochs; i++) {
        network_train(n, patterns);
    }
}

/******************************************************************************/
/* SECTION XX: Test the network against a set of patterns *********************/
/******************************************************************************/

static double vector_compare(ErrorFunction ef, int width, double *v1, double *v2)
{
    double (*net_error_function)() = NULL;
    double err;
    int i;

    switch (ef) {
        case EF_SUM_SQUARE: {
            net_error_function = net_error_ssq; break;
        }
        case EF_CROSS_ENTROPY: {
            net_error_function = net_error_cross_entropy; break;
        }
        case EF_SOFT_MAX: {
            net_error_function = net_error_soft_max; break;
        }
        case EF_MAX: {
            net_error_function = NULL; break;
        }
    }

    err = 0.0;
    if (net_error_function != NULL) {
        for (i = 0; i < width; i++) {
            err += fabs(net_error_function(v1[i], v2[1]));
        }
    }
    return(err / width);
}

static double network_test_pattern(Network *n, ClampedPatternList *clamped_pattern, ErrorFunction ef)
{
    double *vector_in = NULL, *vector_out = NULL, *real_out = NULL;
    double e = 0.0;

    if ((vector_in = (double *)malloc(n->in_width * sizeof(double))) == NULL) {
        fprintf(stderr, "ERROR: Allocation failure in %s at %d\n", __FILE__, __LINE__);
    }
    else if ((vector_out = (double *)malloc(n->out_width * sizeof(double))) == NULL) {
        fprintf(stderr, "ERROR: Allocation failure in %s at %d\n", __FILE__, __LINE__);
    }
    else if ((real_out = (double *)malloc(n->out_width * sizeof(double))) == NULL) {
        fprintf(stderr, "ERROR: Allocation failure in %s at %d\n", __FILE__, __LINE__);
    }
    else {
        network_initialise(n);
        hub_build_target_vector(clamped_pattern, vector_out);
        hub_build_input_vector(clamped_pattern, vector_in);
        network_tell_input(n, vector_in);
        network_tell_propagate_full(n, &(clamped_pattern->clamp));
        network_ask_output(n, real_out);
        e += vector_compare(ef, NUM_IO, vector_out, real_out);
    }

    g_free(vector_in);
    g_free(vector_out);
    g_free(real_out);

    return(e);
}

double network_test(Network *n, PatternList *patterns, ErrorFunction ef)
{
    ClampedPatternList *clamped_patterns, *cp;
    double error = 0.0;
    int l = 0;

    clamped_patterns = network_generate_randomised_clamped_all_pattern_set(patterns);
    for (cp = clamped_patterns; cp != NULL; cp = cp->next) {
        error += network_test_pattern(n, cp, ef);
        l++;
    }
    network_free_clamped_pattern_list(clamped_patterns);
    return(error / (double) l);
}

/*----------------------------------------------------------------------------*/

static double vector_max_bit_diff(int n, double *vector1, double *vector2)
{
    double m = 0.0;
    int i;
    for (i = 0; i < n; i++) {
        m = MAX(m, fabs(vector1[i] - vector2[i]));
        //        fprintf(stdout, "%f - %f; %f\n", vector1[i], vector2[i], m);
    }
    return(m);
}


static double network_test_pattern_maxbit(Network *n, ClampedPatternList *clamped_pattern)
{
    double *vector_in = NULL, *vector_out = NULL, *real_out = NULL;
    double e = 0.0;

    if ((vector_in = (double *)malloc(n->in_width * sizeof(double))) == NULL) {
        fprintf(stderr, "ERROR: Allocation failure in %s at %d\n", __FILE__, __LINE__);
    }
    else if ((vector_out = (double *)malloc(n->out_width * sizeof(double))) == NULL) {
        fprintf(stderr, "ERROR: Allocation failure in %s at %d\n", __FILE__, __LINE__);
    }
    else if ((real_out = (double *)malloc(n->out_width * sizeof(double))) == NULL) {
        fprintf(stderr, "ERROR: Allocation failure in %s at %d\n", __FILE__, __LINE__);
    }
    else {
        network_initialise(n);
        hub_build_target_vector(clamped_pattern, vector_out);
        hub_build_input_vector(clamped_pattern, vector_in);
        network_tell_input(n, vector_in);
        network_tell_propagate_full(n, &(clamped_pattern->clamp));
        network_ask_output(n, real_out);
        e = vector_max_bit_diff(NUM_IO, vector_out, real_out);
    }

    g_free(vector_in);
    g_free(vector_out);
    g_free(real_out);

    return(e);
}

double network_test_max_bit(Network *net, PatternList *patterns)
{
    ClampedPatternList *clamped_patterns, *cp;
    double error = 0.0;

    clamped_patterns = network_generate_randomised_clamped_all_pattern_set(patterns);
    for (cp = clamped_patterns; cp != NULL; cp = cp->next) {
        error = MAX(error, network_test_pattern_maxbit(net, cp));
    }
    network_free_clamped_pattern_list(clamped_patterns);
    return(error);

}

/******************************************************************************/
/* SECTION XX: Read/Write a network to/from a file ****************************/
/******************************************************************************/

static Boolean dump_matrix(FILE *fp, int w1, int w2, double *data)
{
    int i, j;

    if (data == NULL) {
        return(FALSE);
    }
    else {
        for (i = 0; i < w1; i++) {
            for (j = 0; j < w2; j++) {
                fprintf(fp, "\t%+10.8f", data[i * w2 + j]);
            }
            fprintf(fp, "\n");
        }
        fprintf(fp, "\n");
        return(TRUE);
    }
}

/*----------------------------------------------------------------------------*/

static Boolean dump_parameters(FILE *fp, NetworkParameters *params)
{
    if (fp != NULL) {
        fprintf(fp, "%% Weight noise: %f\n", params->wn);
        fprintf(fp, "%% Learning rate: %f\n", params->lr);
        fprintf(fp, "%% Momentum: %f\n", params->momentum);
        fprintf(fp, "%% Weight decay: %f\n", params->wd);
        fprintf(fp, "%% Settling threshold: %f\n", params->st);
        fprintf(fp, "%% Error function: %s\n", net_ef_label[params->ef]);
        fprintf(fp, "%% Update time: %s\n", net_wu_label[params->wut]);
        fprintf(fp, "%% Training epochs: %d\n", params->epochs);
        fprintf(fp, "%% Training criterion: %f\n", params->criterion);
        fprintf(fp, "\n");
        return(TRUE);
    }
    else {
        return(FALSE);
    }
}

/*----------------------------------------------------------------------------*/

Boolean network_write_to_file(FILE *fp, Network *n)
{
    if (!dump_parameters(fp, &(n->params))) {
        return(FALSE);
    }

    fprintf(fp, "%% %dI > %dH\n", n->in_width, n->hidden_width);
    if (!dump_matrix(fp, n->in_width+1, n->hidden_width, n->weights_ih)) {
        return(FALSE);
    }

    if (n->weights_hh) {
        /* This is a recurrent network - dump hidden to hidden weights */
        fprintf(fp, "%% %dH > %dH\n", n->hidden_width, n->hidden_width);
        if (!dump_matrix(fp, n->hidden_width, n->hidden_width, n->weights_hh)) {
            return(FALSE);
        }
    }

    fprintf(fp, "%% %dH > %dO\n", n->hidden_width, n->out_width);
    if (!dump_matrix(fp, n->hidden_width+1, n->out_width, n->weights_ho)) {
        return(FALSE);
    }

    return(TRUE);
}

/******************************************************************************/

static void net_read_skip_blank_lines(FILE *fp)
{
    int c;
    do {
        c = getc(fp);
    } while ((c != EOF) && ((c == '\n') || (c == ' ')));
    ungetc(c, fp);
}

static void *net_weight_file_read_segment(FILE *fp)
{
    // Reads a "segment" of the weight file. This should be a single line that
    // specifies a parameter/value, or a set of connections, as follows:
    // % Training set: features
    // % Learning rate: 0.001000
    // % Weight decay: 0.000007
    // % Momentum: 0.000000
    // % Update time: pattern
    // % Error function: cee
    // % Epochs: 0
    // % 216I > 64H

    char c, buffer[1024]; // Maximum line length
    int l = 0;

    net_read_skip_blank_lines(fp);

    if (getc(fp) != '%') { return(NULL); }
    if (getc(fp) != ' ') { return(NULL); }

    // So we've read "% ". Read the rest of the line into buffer:

    do {
        c = getc(fp);
        buffer[l++] = c;
    } while ((c != '\n') && (c != EOF) && (l < 1024));

    if ((c != '\n') && (c != EOF)) { // Line is too long ... perhaps warn; definitely put the char back
        ungetc(c, fp);
    }
    buffer[l-1] = '\0';

    return(string_copy(buffer));
}

/*----------------------------------------------------------------------------*/

static Boolean net_read_matrix_data(FILE *fp, int w1, int w2, double *weights)
{
    int i, j;
    double x;

    for (i = 0; i < w1; i++) {
        for (j = 0; j < w2; j++) {
            if (fscanf(fp, "%lf", &x) == 0) { return(FALSE); }
            weights[i * w2 + j] = x;
        }
    }
    return(TRUE);    
}

/*----------------------------------------------------------------------------*/

static Boolean network_segment_specifies_weights(char *segment, char c1, char c2, int *d1, int *d2)
{
    // Segment might be something like: 216I > 64H
    // 'I', 'H', &i, &h)

    if (segment == NULL) {
        return(FALSE);
    }
    else {
        int i = 0, w1, w2;

        w1 = 0;
        while (isdigit(segment[i])) {
            w1 = w1 * 10 + (segment[i] - '0');
            i++;
        }
        if (segment[i] != c1) {
            return(FALSE);
        }

        /* So at this stage we've parsed the first part of the spec */

        i++;
        if (segment[i] != ' ') {
            return(FALSE);
        }
        i++;
        if (segment[i] != '>') {
            return(FALSE);
        }
        i++;
        if (segment[i] != ' ') {
            return(FALSE);
        }
        i++;

        /* Now check the second half: */

        w2 = 0;
        while (isdigit(segment[i])) {
            w2 = w2 * 10 + (segment[i] - '0');
            i++;
        }
        if (segment[i] != c2) {
            return(FALSE);
        }

        *d1 = w1;
        *d2 = w2;

        return(TRUE);
    }
}

static void segment_parse_int(char *string, int *val)
{
    int d = 0;
    int i = 0;

    while (isdigit(string[i])) {
        d = d * 10 + (string[i] - '0');
        i++;
    }
    *val = d;
}

static void segment_parse_double(char *string, double *val)
{
    double d = 0.0;
    double denom;
    int i = 0;

    while (isdigit(string[i])) {
        d = d * 10 + (string[i] - '0');
        i++;
    }
    if (string[i] == '.') {
        i++;
        denom = 10.0;
        while (isdigit(string[i])) {
            d = d + (string[i] - '0') / denom;
            denom = denom * 10.0;
            i++;
        }
    }
    *val = d;
}

static int segment_parse_label_string(char *string, char *labels[], int i_max)
{
    int i = 0;

    while ((i < i_max) && (strcmp(string, labels[i]) != 0)) {
        i++;
    }
    return(i);
}

static Boolean network_segment_specifies_parameter(char *segment, NetworkParameters *np)
{
    if (segment == NULL) {
        return(FALSE);
    }
    else if (strncmp(segment, "Training set: ", 14) == 0) {
        // Actually the training set name is not considered to be a network parameter. We ignore it!
        return(TRUE);
    }
    else if (strncmp(segment, "Weight noise: ", 14) == 0) {
        segment_parse_double(&segment[14], &(np->wn));
        return(TRUE);
    }
    else if (strncmp(segment, "Learning rate: ", 15) == 0) {
        segment_parse_double(&segment[15], &(np->lr));
        return(TRUE);
    }
    else if (strncmp(segment, "Momentum: ", 10) == 0) {
        segment_parse_double(&segment[10], &(np->momentum));
        return(TRUE);
    }
    else if (strncmp(segment, "Weight decay: ", 14) == 0) {
        segment_parse_double(&segment[14], &(np->wd));
        return(TRUE);
    }
    else if (strncmp(segment, "Settling threshold: ", 20) == 0) {
        segment_parse_double(&segment[20], &(np->st));
        return(TRUE);
    }
    else if (strncmp(segment, "Error function: ", 16) == 0) {
        np->ef = (ErrorFunction) segment_parse_label_string(&segment[16], net_ef_label, EF_MAX);
        return(TRUE);
    }
    else if (strncmp(segment, "Update time: ", 13) == 0) {
        np->wut = (WeightUpdateTime) segment_parse_label_string(&segment[13], net_wu_label, WU_MAX);
        return(TRUE);
    }
    else if (strncmp(segment, "Epochs: ", 8) == 0) {
        segment_parse_int(&segment[8], &(np->epochs));
        return(TRUE);
    }
    else if (strncmp(segment, "Training epochs: ", 17) == 0) {
        segment_parse_int(&segment[17], &(np->epochs));
        return(TRUE);
    }
    else if (strncmp(segment, "Training criterion: ", 20) == 0) {
        segment_parse_double(&segment[20], &(np->criterion));
        return(TRUE);
    }
    else {
        return(FALSE);
    }
}

Network *network_read_from_file(FILE *fp, char  **error)
{
    Network *n = NULL;
    NetworkParameters np;
    double *ih = NULL, *hh = NULL, *ho = NULL;
    int i, h = 0, o, h1, h2;
    char *segment;

    while ((segment = net_weight_file_read_segment(fp)) != NULL) {
        if (network_segment_specifies_weights(segment, 'I', 'H', &i, &h)) {
            if ((ih = (double *)malloc((i+1) * h * sizeof(double))) == NULL) {
                *error = "Allocation failure (IH)";
                return(NULL);
            }
            else if (!net_read_matrix_data(fp, i+1, h, ih)) {
                g_free(ih);
                g_free(hh);
                g_free(ho);
                *error = "Failed to read IH weights";
                return(NULL);
            }
        }
        else if (network_segment_specifies_weights(segment, 'H', 'H', &h1, &h2)) {
            if ((h1 != h) && (h2 != h)) {
                g_free(ih);
                g_free(hh);
                g_free(ho);
                *error = "Layer size mismatch (HH)";
                return(NULL);
            }
            else if ((hh = (double *)malloc(h * h * sizeof(double))) == NULL) {
                g_free(ih);
                g_free(hh);
                g_free(ho);
                *error = "Allocation failed (HH)";
                return(NULL);
            }
            else if (!net_read_matrix_data(fp, h, h, hh)) {
                g_free(ih);
                g_free(hh);
                g_free(ho);
                *error = "Failed to read HH weights";
                return(NULL);
            }
        }
        else if (network_segment_specifies_weights(segment, 'H', 'O', &h1, &o)) {
            if (h1 != h) {
                g_free(ih);
                g_free(hh);
                g_free(ho);
                *error = "Layer size mismatch (HO)";
                return(NULL);
            }
            else if ((ho = (double *)malloc((h+1) * o * sizeof(double))) == NULL) {
                g_free(ih);
                g_free(hh);
                g_free(ho);
                *error = "Allocation failed (HO)";
                return(NULL);
            }
            else if (!net_read_matrix_data(fp, h+1, o, ho)) {
                g_free(ih);
                g_free(hh);
                g_free(ho);
                *error = "Failed to read HO weights";
                return(NULL);
            }
        }
        else if (!network_segment_specifies_parameter(segment, &np)) {
            g_free(ih);
            g_free(hh);
            g_free(ho);
            *error = "Malformed segment - neither a weight nor a parameter specifier";
            return(NULL);
        }
        string_free(segment);
    }

    /* Now create the new network: */
    if (hh == NULL) {
        n = network_create(NT_FEEDFORWARD, i, h, o);
        free(n->weights_ih);
        n->weights_ih = ih;
        free(n->weights_ho);
        n->weights_ho = ho;
    }
    else {
        n = network_create(NT_RECURRENT, i, h, o);
        free(n->weights_ih);
        n->weights_ih = ih;
        free(n->weights_hh);
        n->weights_hh = hh;
        free(n->weights_ho);
        n->weights_ho = ho;
    }

    /* And set parameters to those read from the file: */
    network_parameters_set(n, &np);

    return(n);
}

#if FALSE
Network *network_read_from_file(FILE *fp, char  **error)
{
    /* SRN Version: */

    Network *n;
    NetworkParameters np;
    int i, h, h1, h2, o;
    double *ih, *ho, *hh;



    /* Read the network parameters into an appropriate structure: */
    if (!net_read_parameters_from_header(fp, &np)) {
        *error = "Malformed parameter specification in file header";
        return(NULL);
    }

    /* Read the input to hidden matrix (with bias weights): */
    if (!net_read_matrix_header(fp, 'I', 'H', &i, &h)) {
        *error = "Malformed IH header";
        return(NULL);
    }
    else if ((ih = (double *)malloc((i+1) * h * sizeof(double))) == NULL) {
        *error = "Allocation failure (IH)";
        return(NULL);
    }
    else if (!net_read_matrix_data(fp, i+1, h, ih)) {
        free(ih);
        *error = "Failed to read IH weights";
        return(NULL);
    }

    /* Read the hidden to hidden matrix (no bias weights): */
    if (!net_read_matrix_header(fp, 'H', 'H', &h1, &h2)) {
        free(ih);
        *error = "Malformed HH header";
        return(NULL);
    }
    else if ((h1 != h) && (h2 != h)) {
        free(ih);
        *error = "Layer size mismatch (HH)";
        return(NULL);
    }
    else if ((hh = (double *)malloc(h * h * sizeof(double))) == NULL) {
        free(ih);
        *error = "Allocation failed (HH)";
        return(NULL);
    }
    else if (!net_read_matrix_data(fp, h, h, hh)) {
        free(ih);
        free(hh);
        *error = "Failed to read HH weights";
        return(NULL);
    }

    /* Read the hidden to output matrix (with bias weights): */
    if (!net_read_matrix_header(fp, 'H', 'O', &h, &o)) {
        free(ih);
        *error = "Malformed HO header";
        return(NULL);
    }
    else if ((ho = (double *)malloc((h+1) * o * sizeof(double))) == NULL) {
        free(ih);
        free(hh);
        *error = "Allocation failed (HO)";
        return(NULL);
    }
    else if (!net_read_matrix_data(fp, h+1, o, ho)) {
        free(ih);
        free(hh);
        free(ho);
        *error = "Failed to read HO weights";
        return(NULL);
    }

    /* Replace the old with the new: */
    n = network_create(NT_RECURRENT, i, h, o);
    free(n->weights_ih);
    n->weights_ih = ih;
    free(n->weights_hh);
    n->weights_hh = hh;
    free(n->weights_ho);
    n->weights_ho = ho;

    /* And set parameters to those read from the file: */
    network_parameters_set(n, &np);

    return(n);
}
#endif
/******************************************************************************/

double network_weight_minimum(Network *net)
{
    double r = 0.0;
    int i, j;

    if ((net->weights_ih == NULL) || (net->weights_ho == NULL)) {
      return(0.0);
    }
    else {
        r = net->weights_ih[0];

	for (i = 0; i < (net->in_width+1); i++) {
            for (j = 0; j < net->hidden_width; j++) {
                r = MIN(r, net->weights_ih[i * net->hidden_width + j]);
	    }
	}

        for (i = 0; i < (net->hidden_width+1); i++) {
            for (j = 0; j < net->out_width; j++) {
	        r = MIN(r, net->weights_ho[i * net->out_width + j]);
            }
        }

        if ((net->nt == NT_RECURRENT) && (net->weights_hh != NULL)) {
            for (i = 0; i < net->hidden_width; i++) {
                for (j = 0; j < net->hidden_width; j++) {
		    r = MIN(r, net->weights_hh[i * net->hidden_width + j]);
		}
	    }
	}

        return(r);
    }
}

/*----------------------------------------------------------------------------*/

double network_weight_maximum(Network *net)
{
    double r = 0.0;
    int i, j;

    if ((net->weights_ih == NULL) || (net->weights_ho == NULL)) {
      return(0.0);
    }
    else {
        r = net->weights_ih[0];

	for (i = 0; i < (net->in_width+1); i++) {
            for (j = 0; j < net->hidden_width; j++) {
                r = MAX(r, net->weights_ih[i * net->hidden_width + j]);
	    }
	}

        for (i = 0; i < (net->hidden_width+1); i++) {
            for (j = 0; j < net->out_width; j++) {
	        r = MAX(r, net->weights_ho[i * net->out_width + j]);
            }
        }

        if ((net->nt == NT_RECURRENT) && (net->weights_hh != NULL)) {
            for (i = 0; i < net->hidden_width; i++) {
                for (j = 0; j < net->hidden_width; j++) {
		    r = MAX(r, net->weights_hh[i * net->hidden_width + j]);
		}
	    }
	}

        return(r);
    }
}

/******************************************************************************/
