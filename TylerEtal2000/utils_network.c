/******************************************************************************/

typedef enum boolean {
    FALSE, TRUE
} Boolean;

#include "utils_network.h"
#include "lib_maths.h"
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <glib.h>

#define DEBUG FALSE

char *nt_name[NT_MAX] = {
    "FFN",
#ifdef CLAMPED
    "SRN (Clamped)"
#else
    "SRN (Unclamped)"
#endif
};

// Set SRN_SETTLING_CYCLES to the number of cycles used when training the SRN
// (which is basically the length of the "sequence" for each input pattern).
// 10 seems ot be enough to settle ... at least for the Tyler et al. (2000) network
#define SRN_SETTLING_CYCLES 10

// Set INPUT_CLAMP_DURATION to zero to keep clamps on for the entire settling
// period, or N to keep the input clamped just for the first N cycles. If
// SRN_SETTLING_CYCLES is 10, then 4 seems to be a reasonable value for this
#ifdef CLAMPED
#define INPUT_CLAMP_DURATION 0
#else
#define INPUT_CLAMP_DURATION 4
#endif

// define BIAS to a value (e.g., -2.0, or 0.0) fix the bias weights at that
// value. Make sure BIAS is undefined to allow bias weights to be learned.
// #define BIAS 0.0
#undef BIAS

// There is an issue about whether bias weights should be subject to damage.
// If we do damage them, then we can't reproduce the effects of Tyler et al.
// (2000), so we assume not, but one could presumably make arguments either
// way about whether they should be susceptible to damage.
// #define BIAS_LESION
#undef BIAS_LESION

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

void network_tell_randomise_hidden(Network *n)
{
    int i;

    for (i = 0; i < n->hidden_width; i++) {
        n->units_hidden[i] = random_uniform(0.01, 0.99);
    }
    if (n->nt == NT_RECURRENT) {
        for (i = 0; i < n->hidden_width; i++) {
            n->units_hidden_prev[i] = random_uniform(0.01, 0.99);
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

void network_print_state(Network *n, FILE *fp, char *message)
{
    int i;

    fprintf(fp, "%s\n", message);
    fprintf(fp, "INPUT: ");
    for (i = 0; i < n->in_width; i++) {
        fprintf(fp, "%f ", n->units_in[i]);
    }
    fprintf(fp, "\n");
    fprintf(fp, "HIDDEN: ");
    for (i = 0; i < n->hidden_width; i++) {
        fprintf(fp, "%f ", n->units_hidden[i]);
    }
    fprintf(fp, "\n");
    fprintf(fp, "OUT: ");
    for (i = 0; i < n->out_width; i++) {
        fprintf(fp, "%f ", n->units_out[i]);
    }
    fprintf(fp, "\n");
    fprintf(fp, "\n");
}

/******************************************************************************/

void network_lesion_weights_ih(Network *n, double severity)
{
    int i, j;

    if (n->weights_ih != NULL) {
// fprintf(stdout, "Lesion IH weights at %f severity\n", severity);
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

void network_lesion_weights_hh(Network *n, double severity)
{
    int i, j;

    if (n->weights_hh != NULL) { // For SRN only
// fprintf(stdout, "Lesion HH weights at %f severity\n", severity);
        for (i = 0; i < n->hidden_width; i++) { // No hidden to hidden bias to worry about
            for (j = 0; j < n->hidden_width; j++) {
                if (random_uniform(0.0, 1.0) < severity) {
                    n->weights_hh[i * n->hidden_width + j] = 0.0;
                }
            }
        }
    }
}

void network_lesion_weights_ho(Network *n, double severity)
{
    int i, j;

    if (n->weights_ho != NULL) {
// fprintf(stdout, "Lesion HO weights at %f severity\n", severity);
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


void network_lesion_weights(Network *net, double severity)
{
    network_lesion_weights_ih(net, severity);
    network_lesion_weights_hh(net, severity);
    network_lesion_weights_ho(net, severity);
}

/******************************************************************************/

static double perturb(double w0, double sd)
{
    return(random_normal(w0, sd));
    // return(w0 + random_uniform(-sd, sd));
}

void network_perturb_weights_ih(Network *n, double noise_sd)
{
    int i, j;

    if (n->weights_ih != NULL) {
        for (i = 0; i < n->in_width; i++) { // Don't lesion the bias!
            for (j = 0; j < n->hidden_width; j++) {
                n->weights_ih[i * n->hidden_width + j] = perturb(n->weights_ih[i * n->hidden_width + j], noise_sd);
            }
        }
#ifdef BIAS_LESION
	i = n->in_width;
        for (j = 0; j < n->hidden_width; j++) {
            n->weights_ih[i * n->hidden_width + j] = perturb(n->weights_ih[i * n->hidden_width + j], noise_sd);
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
                n->weights_hh[i * n->hidden_width + j] = perturb(n->weights_hh[i * n->hidden_width + j], noise_sd);
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
                n->weights_ho[i * n->out_width + j] = perturb(n->weights_ho[i * n->out_width + j], noise_sd);
            }
        }
#ifdef BIAS_LESION
	i = n->hidden_width;
        for (j = 0; j < n->out_width; j++) {
            n->weights_ho[i * n->out_width + j] = perturb(n->weights_ho[i * n->out_width + j], noise_sd);
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

static double vector_compare(ErrorFunction ef, int width, double *v1, double *v2)
{
    switch (ef) {
        case SUM_SQUARE_ERROR: {
            return(vector_sum_square_difference(width, v1, v2));
        }
        case CROSS_ENTROPY: {
            return(vector_cross_entropy(width, v1, v2));
        }
        case SOFT_MAX_ERROR: {
            return(0.0); // TO DO: What's softmax error?
        }
    }
    return(0.0);
}

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

/******************************************************************************/

double net_conflict(Network *n)
{
    double conflict = 0.0;
    int i, j;

    for (i = 0; i < n->out_width; i++) {
        for (j = i+1; j < n->out_width; j++) {
            conflict += n->units_out[i] * n->units_out[j];
        }
    }
    return(conflict);
}

double vector_maximum_activation(double *vector_out, int width)
{
    double result;
    int i;

    result = vector_out[0];
    for (i = 1; i < width; i++) {
        if (vector_out[i] > result) {
            result = vector_out[i];
	}
    }
    return(result);    
}

/******************************************************************************/

void write_network_weights(Network *n, int c)
{
    char buffer[64];
    FILE *fp;

    g_snprintf(buffer, 64, "network_%04d.wgt", c);
    fp = fopen(buffer, "w");
    network_dump_weights(n, fp);
    fclose(fp);
}

void write_network_deltas(FILE *fp, Network *n)
{
    int i, j, k;

    fprintf(fp, "INPUT TO HIDDEN:\n");
    for (k = 0; k < (n->in_width+1); k++) {
         for (j = 0; j < n->hidden_width; j++) {
            fprintf(fp, "\t%f", n->tmp_ih_deltas[k * n->hidden_width + j]);
         }
         fprintf(fp, "\n");
    }
    fprintf(fp, "\n");

    fprintf(fp, "\nHIDDEN TO OUT:\n");
    for (j = 0; j < (n->hidden_width+1); j++) {
         for (i = 0; i < n->out_width; i++) {
            fprintf(fp, "\t%f", n->tmp_ho_deltas[j * n->out_width + i]);
         }
         fprintf(fp, "\n");
    }
    fprintf(fp, "\n");
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
// Could probably alternatively use a zero-centered normal distribution here
    return(random_uniform(-weight_noise, weight_noise));
}

void network_initialise_weights(Network *net, double weight_noise)
{
    int i, j;

    if (net->weights_ih != NULL) {
        for (i = 0; i < (net->in_width+1); i++) {
            for (j = 0; j < net->hidden_width; j++) {
                net->weights_ih[i * net->hidden_width + j] = initial_weight_distribution(weight_noise);
            }
        }
    }

    if (net->weights_hh != NULL) {
        for (i = 0; i < net->hidden_width; i++) {
            for (j = 0; j < net->hidden_width; j++) {
                net->weights_hh[i * net->hidden_width + j] = initial_weight_distribution(weight_noise);
            }
        }
    }

    if (net->weights_ho != NULL) {
        for (i = 0; i < (net->hidden_width+1); i++) {
            for (j = 0; j < net->out_width; j++) {
                net->weights_ho[i * net->out_width + j] = initial_weight_distribution(weight_noise);
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

}

Network *network_initialise(NetworkType nt, int iw, int hw, int ow)
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

        if ((n->units_in = (double *)malloc((n->in_width+1) * sizeof(double))) != NULL) {
            for (i = 0; i < n->in_width; i++) {
                n->units_in[i] = random_uniform(0.01, 0.99);
            }
            n->units_in[n->in_width] = 1.0; // The bias unit ... never change this!
        }
        if ((n->units_hidden = (double *)malloc((n->hidden_width+1) * sizeof(double))) != NULL) {
            for (i = 0; i < n->hidden_width; i++) {
                n->units_hidden[i] = random_uniform(0.01, 0.99);
            }
            n->units_hidden[n->hidden_width] = 1.0; // The bias unit ... never change this!
        }
        if (nt == NT_RECURRENT) {
            if ((n->units_hidden_prev = (double *)malloc(n->hidden_width * sizeof(double))) != NULL) {
                for (i = 0; i < n->hidden_width; i++) {
                    n->units_hidden_prev[i] = random_uniform(0.01, 0.99);
                }
            }
        }
        else {
            n->units_hidden_prev = NULL;
        }
        if ((n->units_out = (double *)malloc(n->out_width * sizeof(double))) != NULL) {
            for (i = 0; i < n->out_width; i++) {
                n->units_out[i] = random_uniform(0.01, 0.99);
            }
        }

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

        return(r);
    }
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

void network_tell_input(Network *n, double *vector)
{
    /* Generalised Version: */

    int i;
    if (n->units_in != NULL) {
        for (i = 0; i < n->in_width; i++) {
            n->units_in[i] = vector[i];
        }
    }
    /* Initialise the cycle count (only relevant for recurrent networks) */
    n->cycles = 0;
}

/*----------------------------------------------------------------------------*/

void network_tell_propagate(Network *n)
{
    /* Generalised version (FF or SRN) */

    int i, j;

    if (n->nt == NT_RECURRENT) {
        if ((INPUT_CLAMP_DURATION > 0) && (n->cycles >= INPUT_CLAMP_DURATION)) {
            network_zero_input(n);
        }
    }

    /* Propagate from input to hidden: */
    if (n->units_hidden != NULL) {
        for (j = 0; j < n->hidden_width; j++) {
            n->units_hidden[j] = 0.0;
            for (i = 0; i < (n->in_width+1); i++) {
                n->units_hidden[j] += n->units_in[i] * n->weights_ih[i * n->hidden_width + j];
            }
            /* If recurrent, then add in recurrent input: */
            if (n->nt == NT_RECURRENT) {
                for (i = 0; i < n->hidden_width; i++) {
                    n->units_hidden[j] += n->units_hidden_prev[i] * n->weights_hh[i * n->hidden_width + j];
                }
            }
            /* And calculate the post-synaptic value: */
            n->units_hidden[j] = sigmoid(n->units_hidden[j]);
        }
    }

    /* Propagate from hidden to output: */
    if (n->units_out != NULL) {
        for (j = 0; j < n->out_width; j++) {
            n->units_out[j] = 0.0;
            for (i = 0; i < (n->hidden_width+1); i++) {
                n->units_out[j] += n->units_hidden[i] * n->weights_ho[i * n->out_width + j];
            }
            n->units_out[j] = sigmoid(n->units_out[j]);
        }
    }

    if (n->nt == NT_RECURRENT) {

        // Keep track of the number of cycles:
        n->cycles++;

        if (n->cycles > INPUT_CLAMP_DURATION) {
            // Assume that a recurrent network has settled if the difference 
            // between its current and its previous hidden layer is less than
            // some threshold.
            n->settled = (vector_sum_square_difference(n->hidden_width, n->units_hidden, n->units_hidden_prev) < NET_SETTLING_THRESHOLD);
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

/*----------------------------------------------------------------------------*/

void network_tell_propagate_full(Network *n)
{
    // propagate activation (until the network settles, or 100 cycles)

    if (n->nt == NT_FEEDFORWARD) {
        network_tell_propagate(n);
    }
    else if (n->nt == NT_RECURRENT) {
        do {
            network_tell_propagate(n);
        } while ((!n->settled) && (n->cycles < 100));
    }
}

/******************************************************************************/
/* TRAINING *******************************************************************/
/******************************************************************************/

static void training_patterns_randomise(PatternList *patterns)
{
    PatternList *tmp, *this;

    for (tmp = patterns; tmp->next != NULL; tmp = tmp->next) {
        double *pl;
        int i = random_int(pattern_list_length(tmp));

        this = tmp->next;
        while (--i > 0) {
            this = this->next;
        }

        pl = this->vector_in;
        this->vector_in = tmp->vector_in;
        tmp->vector_in = pl;

        pl = this->vector_out;
        this->vector_out = tmp->vector_out;
        tmp->vector_out = pl;
    }
}

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

static void network_train_update_weights(Network *n, double lr, double wd, double momentum)
{
    /* Generalised Version (both FF and SRN):                   */
    /* From Hertz et al., 1991, pp. 116-117 (momentum: p. 123)  */

    double this_delta;
    int i, j;

    for (i = 0; i < (n->in_width+1); i++) {
        for (j = 0; j < n->hidden_width; j++) {
            n->weights_ih[i * n->hidden_width + j] *= (1 - wd);
            this_delta = lr * n->tmp_ih_deltas[i * n->hidden_width + j] + momentum * n->previous_ih_deltas[i * n->hidden_width + j];
            n->weights_ih[i * n->hidden_width + j] += this_delta;
            n->previous_ih_deltas[i * n->hidden_width + j] = this_delta;
        }
    }

    if (n->nt == NT_RECURRENT) {
        for (i = 0; i < n->hidden_width; i++) {
            for (j = 0; j < n->hidden_width; j++) {
                n->weights_hh[i * n->hidden_width + j] *= (1 - wd);
                this_delta = lr * n->tmp_hh_deltas[i * n->hidden_width + j] + momentum * n->previous_hh_deltas[i * n->hidden_width + j];
                n->weights_hh[i * n->hidden_width + j] += this_delta;
                n->previous_hh_deltas[i * n->hidden_width + j] = this_delta;
            }
        }
    }

    for (i = 0; i < (n->hidden_width+1); i++) {
        for (j = 0; j < n->out_width; j++) {
            n->weights_ho[i * n->out_width + j] *= (1 - wd);
            this_delta = lr * n->tmp_ho_deltas[i * n->out_width + j] + momentum * n->previous_ho_deltas[i * n->out_width + j];
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

static void network_accumulate_weight_changes_srn(Network *n, double *test_in, double *test_out, double (*net_error_function)())
{
    /* SRN learning: An implementation of epochwise BPTT, based on Williams & Zipser, 1995 */

    int cycles = SRN_SETTLING_CYCLES; 
    int units = n->hidden_width + n->out_width;
    double *history_hidden = (double *)malloc((cycles+2) * n->hidden_width * sizeof(double));
    double *history_error = (double *)malloc((cycles+2) * n->out_width * sizeof(double));
    double *history_out = (double *)malloc((cycles+2) * n->out_width * sizeof(double));
    double *delta = (double *)malloc((cycles+2) * units * sizeof(double));
    double *epsilon = (double *)malloc((cycles+2) * units * sizeof(double));
    double y, weight_lk, df;
    int i, j, k, l, t;

    Boolean penalty = FALSE;

    /* 1: Run the network over the entire sequence and collect its state: */

    network_tell_randomise_hidden(n);
    network_tell_input(n, test_in);
    for (t = 0; t < cycles+2; t++) {
        network_ask_hidden(n, &history_hidden[t * n->hidden_width]);
        network_ask_output(n, &history_out[t * n->out_width]);
        network_tell_propagate(n);

        for (i = 0; i < n->out_width; i++) {
            history_error[t * n->out_width + i] = (t < 2) ? 0.0 : net_error_function(test_out[i], history_out[t * n->out_width + i]);
        }

    }

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
        fprintf(stdout, "Epsilon at t = %d: ", t);
        for (k = 0; k < units; k++) {
            fprintf(stdout, "%+6.4f ", epsilon[t*units+k]);
        }
        fprintf(stdout, "\n");
        fprintf(stdout, "Delta at t = %d: ", t);
        for (k = 0; k < units; k++) {
            fprintf(stdout, "%+6.4f ", delta[t*units+k]);
        }
        fprintf(stdout, "\n\n");
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

    write_network_deltas(stdout, n);

#endif

    /* 4: Release all allocated space: */
    free(history_hidden);
    free(history_out);
    free(history_error);
    free(delta);
    free(epsilon);
}

/*----------------------------------------------------------------------------*/

static void network_accumulate_weight_changes(Network *n, double *test_in, double *test_out, double (*net_error_function)())
{
    switch (n->nt) {
        case NT_FEEDFORWARD: {
            network_accumulate_weight_changes_ff(n, test_in, test_out, net_error_function);
            break;
        }
        case NT_RECURRENT: {
            network_accumulate_weight_changes_srn(n, test_in, test_out, net_error_function);
            break;
        }
        case NT_MAX: {
            fprintf(stderr, "WARNING: Invalid network type\n");
            break;
        }
    }
}

Boolean network_train(Network *n, NetworkParameters *pars, PatternList *seqs)
{
    /* FF Version: */

    double (*net_error_function)() = NULL;

    switch (pars->ef) {
        case SUM_SQUARE_ERROR: {
            net_error_function = net_error_ssq; break;
        }
        case CROSS_ENTROPY: {
            net_error_function = net_error_cross_entropy; break;
        }
        case SOFT_MAX_ERROR: {
            net_error_function = net_error_soft_max; break;
        }
    }

    if ((n->tmp_ih_deltas == NULL) || (n->tmp_ho_deltas == NULL)) {
        return(FALSE);
    }

    if (pars->wut == UPDATE_BY_EPOCH) {

        /* Initialise weight changes: */
        network_train_initialise_deltas(n);

        /* Run the training data, accumulating weight changes over all sequences: */
        while (seqs != NULL) {
            network_accumulate_weight_changes(n, seqs->vector_in, seqs->vector_out, net_error_function);
            seqs = seqs->next;
        }

        /* Now update the weights: */
        network_train_update_weights(n, pars->lr, pars->wd, pars->momentum);
    }
    else {

        /* First randomise the training patterns on each epoch: */
        training_patterns_randomise(seqs);

        /* Adjust the weight decay rate for item-wise decay */
        int l = pattern_list_length(seqs);
        double wd = 1.0 - exp(log(1.0 - pars->wd) / (double) l);

        /* Now train with each sequence */
        while (seqs != NULL) {

            /* Initialise weight changes: */
            network_train_initialise_deltas(n);

            /* Run the training data, over the current pattern: */
            network_accumulate_weight_changes(n, seqs->vector_in, seqs->vector_out, net_error_function);

            /* Now update the weights: */
            network_train_update_weights(n, pars->lr, wd, pars->momentum);

            /* And move on to the next sequence: */
            seqs = seqs->next;
        }
    }

    return(TRUE);
}

void network_train_to_criterion(Network *n, NetworkParameters *pars, PatternList *seqs)
{
    int i;

    for (i = 0; i < pars->epochs; i++) {
        network_train(n, pars, seqs);

        if (network_test(n, seqs, pars->ef) < pars->criterion) {
            return;
        }
    }
}

void network_train_to_epochs(Network *n, NetworkParameters *pars, PatternList *seqs)
{
    int i;

    for (i = 0; i < pars->epochs; i++) {
        network_train(n, pars, seqs);
    }
}

/******************************************************************************/

static double network_test_pattern(Network *n, PatternList *patterns, ErrorFunction ef)
{
    double *real_out;
    double e = 0.0;

    if ((real_out = (double *)malloc(n->out_width * sizeof(double))) == NULL) {
        fprintf(stderr, "ERROR: Allocation failure in %s at %d\n", __FILE__, __LINE__);
    }
    else {
        network_tell_input(n, patterns->vector_in);
        network_tell_propagate_full(n);
        network_ask_output(n, real_out);
        e = vector_compare(ef, n->out_width, patterns->vector_out, real_out);
        free(real_out);
    }
    return(e);
}

double network_test(Network *n, PatternList *patterns, ErrorFunction ef)
{
    double error = 0.0;
    int l = 0;

    while (patterns != NULL) {
        error += network_test_pattern(n, patterns, ef);
        l++;
        patterns = patterns->next;
    }
    return(error / (double) (l * n->out_width));
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

Boolean network_dump_weights(Network *n, FILE *fp)
{
    fprintf(fp, "%% %dI > %dH\n", n->in_width, n->hidden_width);
    if (!dump_matrix(fp, n->in_width+1, n->hidden_width, n->weights_ih)) {
        return(FALSE);
    }

    if (n->weights_hh) { /* This is a recurrent network - dump hidden to hidden weights */
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

static int net_read_header_integer(FILE *fp)
{
    int r = 0, d;

    while (isdigit(d = getc(fp))) {
        r = r * 10 + (d - '0');
    }
    ungetc(d, fp);
    return(r);
}

static Boolean net_read_matrix_header(FILE *fp, char c1, char c2, int *u1, int *u2)
{
    /* Header should be of the form: "% 19I > 28H"                      */
    /* The header could be preceeded by a comment, of the form "% x..." */
    /* where "x" is a non-digit                                         */

    int c;

    net_read_skip_blank_lines(fp);
    if (getc(fp) != '%') { return(FALSE); }
    if (getc(fp) != ' ') { return(FALSE); }

    if (!isdigit(c = getc(fp))) {
        while ((c != '\n') && (c != EOF)) {
            c = getc(fp);
        }
        return(net_read_matrix_header(fp, c1, c2, u1, u2));
    }
    else {
        ungetc(c, fp);
    }

    if (!((*u1 = net_read_header_integer(fp)) > 0)) { return(FALSE); }
    if (getc(fp) != c1) { return(FALSE); }
    if (getc(fp) != ' ') { return(FALSE); }
    if (getc(fp) != '>') { return(FALSE); }
    if (getc(fp) !=' ') { return(FALSE); }
    if (!((*u2 = net_read_header_integer(fp)) > 0)) { return(FALSE); }
    if (getc(fp) != c2) { return(FALSE); }
    if (getc(fp) != '\n') { return(FALSE); }
    return(TRUE);
}

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

Network *network_read_weights_from_file(FILE *fp, int *error)
{
    /* FF Version: */

    Network *n;
    int i, h, o;
    double *ih, *ho;

    /* Read the input to hidden matrix: */

    if (!net_read_matrix_header(fp, 'I', 'H', &i, &h)) {
        *error = ERROR_RW_HEADER1;
        return(NULL);
    }
    else if ((ih = (double *)malloc((i+1) * h * sizeof(double))) == NULL) {
        *error = ERROR_RW_ALLOC1;
        return(NULL);
    }
    else if (!net_read_matrix_data(fp, i+1, h, ih)) {
        free(ih);
        *error = ERROR_RW_READ1;
        return(NULL);
    }

    /* Read the hidden to output matrix: */

    if (!net_read_matrix_header(fp, 'H', 'O', &h, &o)) {
        free(ih);
        *error = ERROR_RW_HEADER3;
        return(NULL);
    }
    else if ((ho = (double *)malloc((h+1) * o * sizeof(double))) == NULL) {
        free(ih);
        *error = ERROR_RW_ALLOC3;
        return(NULL);
    }
    else if (!net_read_matrix_data(fp, h+1, o, ho)) {
        free(ih);
        free(ho);
        *error = ERROR_RW_READ1;
        return(NULL);
    }

    /* Replace the old with the new: */

    n = network_initialise(NT_FEEDFORWARD, i, h, o);
    free(n->weights_ih);
    n->weights_ih = ih;
    free(n->weights_ho);
    n->weights_ho = ho;
    return(n);
}

/******************************************************************************/
/* SECTION XX */
/******************************************************************************/

void training_set_free(PatternList *patterns)
{
    pattern_list_free(patterns);
}

int pattern_set_length(PatternList *patterns)
{
    int i = 0;
    while (patterns != NULL) {
        i++; patterns = patterns->next;
    }
    return(i);
}

PatternList *training_set_read(char *file, int w_in, int w_out)
{
    return(read_training_patterns(file, w_in, w_out));
}

PatternList *training_set_next(PatternList *current)
{
    return(current->next);
}

double *training_set_input_vector(PatternList *this)
{
    return(this->vector_in);
}

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

