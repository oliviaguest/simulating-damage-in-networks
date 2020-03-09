/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

typedef enum {FALSE, TRUE} Boolean;

#include "lib_network.h"
#include "utils_maths.h"

#include <math.h>
#include <ctype.h>

typedef enum {ERROR_NONE, 
    ERROR_RW_HEADER1, ERROR_RW_ALLOC1, ERROR_RW_READ1,
    ERROR_RW_HEADER2, ERROR_RW_ALLOC2, ERROR_RW_READ2,
    ERROR_RW_HEADER3, ERROR_RW_ALLOC3, ERROR_RW_READ3} ErrorType;

/******************************************************************************/

static double read_double(FILE *fp)
{
    double r = 0;
    int c;

    while (isdigit(c = fgetc(fp))) {
        r = r * 10.0 + (c - '0');
    }
    if (c == '.') {
        double denom = 1.0;
        while (isdigit(c = fgetc(fp))) {
            denom = denom * 10.0;
            r = r + ((c - '0') / denom);
        }
    }
    ungetc(c, fp);
    return(r);
}

static Boolean read_training_data_item(FILE *fp, int n, double *vector_in, int m, double *vector_out)
{
    char c;
    int i;

    for (i = 0; i < n; i++) {
        while ((c = fgetc(fp)) == ' ') { };
        if ((c == '\n') || (c == EOF)) {
            return(FALSE);
        }
        else {
            ungetc(c, fp);
            vector_in[i] = read_double(fp);
        }
    }
    while ((c = fgetc(fp)) == ' ') { };
    if (c != '>') {
        return(FALSE);
    }
    for (i = 0; i < m; i++) {
        while ((c = fgetc(fp)) == ' ') { };
        if ((c == '\n') || (c == EOF)) {
            return(FALSE);
        }
        else {
            ungetc(c, fp);
            vector_out[i] = read_double(fp);
        }
    }
    while ((c = fgetc(fp)) == ' ') { };
    return(c == '\n');
}

static PatternList *pattern_list_allocate(int in_width, int out_width)
{
    PatternList *new;

    if ((new = (PatternList *)malloc(sizeof(PatternList))) == NULL) {
        return(NULL);
    }
    else if ((new->vector_in = (double *)malloc(in_width * sizeof(double))) == NULL) {
        free(new);
        return(NULL);
    }
    else if ((new->vector_out = (double *)malloc(out_width * sizeof(double))) == NULL) {
        free(new);
        free(new->vector_in);
        return(NULL);
    }
    else {
        return(new);
    }
}

PatternList *read_training_patterns(char *filename, int in_width, int out_width)
{
    /* Read training data from a file: Each line should represent one case */

    FILE *fp;

    if ((fp = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "WARNING: Cannot read %s (training patterns)\n", filename);
        return(NULL);
    }
    else {
        PatternList dummy = {NULL, NULL, NULL};
        PatternList *last = &dummy, *new;
        Boolean ok = TRUE;

        do {
            if ((new = pattern_list_allocate(in_width, out_width)) == NULL) {
                ok = FALSE;
            }
            else if (read_training_data_item(fp, in_width, new->vector_in, out_width, new->vector_out)) {
                last->next = new;
                last = last->next;
            }
            else {
                last->next = NULL;
                ok = FALSE;
            }
        } while (ok);
        fclose(fp);
        return(dummy.next);
    }
}

/*----------------------------------------------------------------------------*/

static PatternList *read_training_sequence(FILE *fp, int in_width, int out_width)
{
    /* Read one training sequence. Each sequence should start with a comment  */
    /* line (beginning with '#') which should be ignored. There should then   */
    /* follow any number of pattern lines representing sequence steps, with   */
    /* the sequence terminated by either end of file or a new comment line.   */
    /* Return FALSE if there's an error or we're at end of file.              */

    PatternList dummy = {NULL, NULL, NULL};
    PatternList *last = &dummy, *new;
    Boolean ok = TRUE;
    char c = fgetc(fp);

    if (c == EOF) {
        return(NULL);
    }
    else if (c == '#') {
        do {
            if ((c = fgetc(fp)) == EOF) {
                return(NULL);
            }
        } while (c != '\n');
    }

    do {
        if ((new = pattern_list_allocate(in_width, out_width)) == NULL) {
            ok = FALSE;
        }
        else if (read_training_data_item(fp, in_width, new->vector_in, out_width, new->vector_out)) {
            last->next = new;
            last = new;
        }
        else {
            ok = FALSE;
        }
        ungetc(c = fgetc(fp), fp);
    } while (ok && (c != '#') && (c != EOF));

    last->next = NULL;
    return(dummy.next);
}

SequenceList *read_training_sequences(char *filename, int in_width, int out_width)
{
    /* Read training sequences from a file: Each line should represent one    */
    /* step, and each new sequence should be preceded by a line starting with */
    /* the '#' character.                                                     */

    FILE *fp;

    if ((fp = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "WARNING: Cannot read %s (training sequences)\n", filename);
        return(NULL);
    }
    else {
        SequenceList dummy = {NULL, NULL};
        SequenceList *last = &dummy, *new;
        Boolean ok = TRUE;

        do {
            if ((new = (SequenceList *)malloc(sizeof(SequenceList))) == NULL) {
                ok = FALSE;
            }
            else if ((new->head = read_training_sequence(fp, in_width, out_width)) == NULL) {
                free(new);
                last->tail = NULL;
                ok = FALSE;
            }
            else {
                last->tail = new;
                last = last->tail;
            }
        } while (ok);
        fclose(fp);
        return(dummy.tail);
    }
}

/******************************************************************************/

int pattern_list_length(PatternList *patterns)
{
    int n = 0;
    while (patterns != NULL) {
        n++;
        patterns = patterns->next;
    }
    return(n);
}

void pattern_list_print(PatternList *patterns, int in_width, int out_width, FILE *fp)
{
    int i = 0;
    while (patterns != NULL) {
        fprintf(fp, "%2d: ", ++i);
        print_vector(fp, in_width, patterns->vector_in);
        fprintf(fp, " => ");
        print_vector(fp, out_width, patterns->vector_out);
        fprintf(fp, "\n");
        patterns = patterns->next;
    }
}

void pattern_list_free(PatternList *patterns)
{
    PatternList *tmp;
    while (patterns != NULL) {
        tmp = patterns->next;
        free(patterns->vector_in);
        free(patterns->vector_out);
        free(patterns);
        patterns = tmp;
    }
}

/******************************************************************************/

void print_vector(FILE *fp, int n, double *vector_in)
{
    int i;

    for (i = 0; i < n; i++) {
        fprintf(fp, "%6.4f", vector_in[i]);
        if (i != n-1) {
            fputc(' ', fp);
        }
    }
}


void print_string(FILE *fp, int width, char *string)
{
    Boolean instring = TRUE;
    int i = 0;

    while (i < width) {
        if (!instring) {
            fputc(' ', fp);
        }
        else if (string[i] == '\0') {
            fputc(' ', fp);
            instring = FALSE;
        }
        else {
            fputc(string[i], fp);
        }
        i++;
    }
}

void sequence_list_print_stats(SequenceList *seqs, FILE *fp)
{
    int n = 0, sl = 0;

    while (seqs != NULL) {
        sl += pattern_list_length(seqs->head);
        seqs = seqs->tail;
        n++;
    }
    fprintf(fp, "%d sequences with average length %f\n", n, sl / (double) n);
}

int sequence_list_length(SequenceList *seqs)
{
    int n = 0;
    while (seqs != NULL) {
        n++;
        seqs = seqs->tail;
    }
    return(n);
}

void sequence_list_print(SequenceList *seqs, int in_width, int out_width, FILE *fp)
{
    while (seqs != NULL) {
        fprintf(fp, "# New sequence:\n");
        pattern_list_print(seqs->head, in_width, out_width, fp);
        seqs = seqs->tail;
    }
}

void sequence_list_free(SequenceList *seqs)
{
    SequenceList *tmp;

    while (seqs != NULL) {
        tmp = seqs;
        pattern_list_free(tmp->head);
        seqs = tmp->tail;
        free(tmp);
    }
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

extern double random_uniform(double low, double high);
extern double random_normal(double mean, double sd);
extern int random_int(int n);
extern double sigmoid(double input);
extern double sigmoid_derivative(double input);
extern double vector_sum_square_difference(int w, double *v1, double *v2);
extern double vector_rms_difference(int w, double *v1, double *v2);
extern double vector_cross_entropy(int w, double *v1, double *v2);
extern double vector_set_variability(int n, int l, double *v);

/***************************** GENERIC UTILITIES ******************************/

void network_ask_input(Network *net, double *vector)
{
    if (net->units_in != NULL) {
        int i;

        for (i = 0; i < net->in_width; i++) {
            vector[i] = net->units_in[i];
        }
    }
}

void network_ask_hidden(Network *net, double *vector)
{
    if (net->units_hidden != NULL) {
        int i;

        for (i = 0; i < net->hidden_width; i++) {
            vector[i] = net->units_hidden[i];
        }
    }
}

void network_ask_output(Network *net, double *vector)
{
    if (net->units_out != NULL) {
        int i;

        for (i = 0; i < net->out_width; i++) {
            vector[i] = net->units_out[i];
        }
    }
}

void network_tell_randomise_hidden_units(Network *net)
{
    int i;

    for (i = 0; i < net->hidden_width; i++) {
        net->units_hidden[i] = random_uniform(0.01, 0.99);
    }
}

void network_perturb_weights_ih(Network *net, double variance)
{
    double sd = sqrt(variance);
    int i, j;

    if (net->weights_ih != NULL) {
        for (i = 0; i < (net->in_width+1); i++) {
            for (j = 0; j < net->hidden_width; j++) {
                net->weights_ih[i * net->hidden_width + j] += random_normal(0, sd);
            }
        }
    }
}

void network_perturb_weights_ch(Network *net, double variance)
{
    double sd = sqrt(variance);
    int i, j;

    if (net->weights_hh != NULL) {
        for (i = 0; i < net->hidden_width; i++) {
            for (j = 0; j < net->hidden_width; j++) {
                net->weights_hh[i * net->hidden_width + j] += random_normal(0, sd);
            }
        }
    }
}

void network_inject_noise(Network *net, double variance)
{
    double sd = sqrt(variance);
    int i;

    for (i = 0; i < net->hidden_width; i++) {
        net->units_hidden[i] += random_normal(0, sd);
    }
}

void network_print_state(FILE *fp, Network *net, char *message)
{
    int i;

    fprintf(fp, "%s\n", message);
    fprintf(fp, "INPUT: ");
    for (i = 0; i < net->in_width; i++) {
        fprintf(fp, "%f ", net->units_in[i]);
    }
    fprintf(fp, "\n");
    fprintf(fp, "HIDDEN: ");
    for (i = 0; i < net->hidden_width; i++) {
        fprintf(fp, "%f ", net->units_hidden[i]);
    }
    fprintf(fp, "\n");
    fprintf(fp, "OUT: ");
    for (i = 0; i < net->out_width; i++) {
        fprintf(fp, "%f ", net->units_out[i]);
    }
    fprintf(fp, "\n");
    fprintf(fp, "\n");
}

/******************************************************************************/

void network_lesion_weights_ih(Network *net, double severity)
{
    int i, j;

    if (net->weights_ih != NULL) {
        for (i = 0; i < (net->in_width+1); i++) {
            for (j = 0; j < net->hidden_width; j++) {
                if (random_uniform(0.0, 1.0) < severity) {
                    net->weights_ih[i * net->hidden_width + j] = 0;
                }
            }
        }
    }
}

void network_lesion_weights_ch(Network *net, double severity)
{
    int i, j;

    if (net->weights_hh != NULL) {
        for (i = 0; i < net->hidden_width; i++) {
            for (j = 0; j < net->hidden_width; j++) {
                if (random_uniform(0.0, 1.0) < severity) {
                    net->weights_hh[i * net->hidden_width + j] = 0;
                }
            }
        }
    }
}

/******************************************************************************/

void network_ablate_context(Network *net, double severity)
{
    int i, j;

    if (net->weights_hh != NULL) {
        for (i = 0; i < net->hidden_width; i++) {
            if (random_uniform(0.0, 1.0) < severity) {
                for (j = 0; j < net->hidden_width; j++) {
                    net->weights_hh[i * net->hidden_width + j] = 0;
                }
            }
        }
    }
}

/******************************************************************************/

void network_scale_weights(Network *net, double proportion)
{
    int i, j;

    /* Scale input to hidden weights: */
    if (net->weights_ih != NULL) {
        for (i = 0; i < (net->in_width+1); i++) {
            for (j = 0; j < net->hidden_width; j++) {
                net->weights_ih[i * net->hidden_width + j] *= proportion;
            }
        }
    }

    /* Scale hidden to hidden weights:*/
    if (net->weights_hh != NULL) {
        for (i = 0; i < net->hidden_width; i++) {
            for (j = 0; j < net->hidden_width; j++) {
                net->weights_hh[i * net->hidden_width + j] *= proportion;
            }
        }
    }

    /* Scale hidden to output weights:*/
    if (net->weights_ho != NULL) {
        for (i = 0; i < (net->hidden_width+1); i++) {
            for (j = 0; j < net->out_width; j++) {
                net->weights_ho[i * net->out_width + j] *= proportion;
            }
        }
    }
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

static double vector_compare(ErrorFunction ef, int width, double *v1, double *v2)
{
    switch (ef) {
        case SUM_SQUARE_ERROR: {
            return(vector_sum_square_difference(width, v1, v2));
        }
        case RMS_ERROR: {
            return(vector_rms_difference(width, v1, v2));
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
    return(d - y);
}

double net_error_cross_entropy(double d, double y)
{
    return((d - y) / (y * (1 - y)));
}

double net_error_soft_max(double d, double y)
{
    return(d  / y); // Based on the LENS code...
}

/***********************END GENERIC UTILITIES *********************************/

/************************* UTILITIES FOR SRN NETWORKS *************************/

static void network_tell_initialise_units(Network *net)
{
    int i;

    if (net->units_in != NULL) {
        for (i = 0; i < net->in_width; i++) {
            net->units_in[i] = random_uniform(0.01, 0.99);
        }
        net->units_in[net->in_width] = 1.0; // The bias unit ... never change this!
    }
    if (net->units_hidden != NULL) {
        for (i = 0; i < net->hidden_width; i++) {
            net->units_hidden[i] = random_uniform(0.01, 0.99);
        }
        net->units_hidden[net->hidden_width] = 1.0; // The bias unit ... never change this!
    }
    if (net->units_out != NULL) {
        for (i = 0; i < net->out_width; i++) {
            net->units_out[i] = random_uniform(0.01, 0.99);
        }
    }
}

static void network_tell_randomise_weights(Network *net)
{
    int i, j;

    if (net->weights_ih != NULL) {
        for (i = 0; i < (net->in_width+1); i++) {
            for (j = 0; j < net->hidden_width; j++) {
                net->weights_ih[i * net->hidden_width + j] = random_uniform(-1.0, 1.0);
            }
        }
    }
    if (net->weights_hh != NULL) {
        for (i = 0; i < net->hidden_width; i++) {
            for (j = 0; j < net->hidden_width; j++) {
                net->weights_hh[i * net->hidden_width + j] = random_uniform(-1.0, 1.0);
            }
        }
    }
    if (net->weights_ho != NULL) {
        for (i = 0; i < (net->hidden_width+1); i++) {
            for (j = 0; j < net->out_width; j++) {
                net->weights_ho[i * net->out_width + j] = random_uniform(-1.0, 1.0);
            }
        }
    }
}

/*----------------------------------------------------------------------------*/

void network_tell_initialise(Network *net)
{
    network_tell_randomise_hidden_units(net);
    network_tell_randomise_weights(net);
    network_tell_initialise_units(net);
}

void network_tell_destroy(Network *net)
{
    /* SRN Version: */

    if (net != NULL) {
        if (net->weights_ih != NULL) { free(net->weights_ih); }
        if (net->weights_hh != NULL) { free(net->weights_hh); }
        if (net->weights_ho != NULL) { free(net->weights_ho); }
        if (net->units_in != NULL) { free(net->units_in); }
        if (net->units_hidden != NULL) { free(net->units_hidden); }
        if (net->units_out != NULL) { free(net->units_out); }
        if (net->tmp_hidden != NULL) { free(net->tmp_hidden); }
        if (net->tmp_out != NULL) { free(net->tmp_out); }
        if (net->tmp_ih_deltas != NULL) { free(net->tmp_ih_deltas); }
        if (net->tmp_hh_deltas != NULL) { free(net->tmp_hh_deltas); }
        if (net->tmp_ho_deltas != NULL) { free(net->tmp_ho_deltas); }
        free(net);
    }
}

Network *network_create(int iw, int hw, int ow)
{
    Network *net;

    if ((net = (Network *)malloc(sizeof(Network))) != NULL) {

        /* SRN Version: */

        net->in_width = iw;
        net->hidden_width = hw;
        net->out_width = ow;

        net->units_in = (double *)malloc((net->in_width+1) * sizeof(double));
        net->units_hidden = (double *)malloc((net->hidden_width+1) * sizeof(double));
        net->units_out = (double *)malloc(net->out_width * sizeof(double));
        network_tell_initialise_units(net);

        // Note: No bias unit in hidden to hidden because hidden units get bias
        // from the input layer

        net->weights_ih = (double *)malloc((net->in_width+1) * net->hidden_width * sizeof(double));
        net->weights_hh = (double *)malloc(net->hidden_width * net->hidden_width * sizeof(double));
        net->weights_ho = (double *)malloc((net->hidden_width+1) * net->out_width * sizeof(double));
        network_tell_randomise_weights(net);

        /* Allocate temporary space for use when propagating and training: */

        net->tmp_hidden = (double *)malloc(net->hidden_width * sizeof(double));
        net->tmp_out = (double *)malloc(net->out_width * sizeof(double));
        net->tmp_ih_deltas = (double *)malloc((net->in_width+1) * net->hidden_width * sizeof(double));
        net->tmp_hh_deltas = (double *)malloc(net->hidden_width * net->hidden_width * sizeof(double));
        net->tmp_ho_deltas = (double *)malloc((net->hidden_width+1) * net->out_width * sizeof(double));
    }
    return(net);
}

Network *network_copy(Network *net)
{
    Network *new;
    int iw, hw, ow;
    int i, j;

    iw = net->in_width;
    hw = net->hidden_width;
    ow = net->out_width;

    if ((new = network_create(iw, hw, ow)) != NULL) {
        /* Copy unit activations: */
        if ((new->units_in != NULL) && (net->units_in != NULL)) {
            for (i = 0; i < (iw+1); i++) {
                new->units_in[i] = net->units_in[i];
            }
        }
        if ((new->units_hidden != NULL) && (net->units_hidden != NULL)) {
            for (i = 0; i < (hw+1); i++) {
                new->units_hidden[i] = net->units_hidden[i];
            }
        }
        if ((new->tmp_hidden != NULL) && (net->tmp_hidden != NULL)) {
            for (i = 0; i < hw; i++) {
                new->tmp_hidden[i] = net->tmp_hidden[i];
            }
        }
        if ((new->units_out != NULL) && (net->units_out != NULL)) {
            for (i = 0; i < ow; i++) {
                new->units_out[i] = net->units_out[i];
            }
        }
        if ((new->tmp_out != NULL) && (new->tmp_out != NULL)) {
            for (i = 0; i < ow; i++) {
                new->tmp_out[i] = net->tmp_out[i];
            }
        }

        /* Copy weights: */
        if ((new->weights_ih != NULL) && (net->weights_ih != NULL)) {
            for (i = 0; i < (iw+1); i++) {
                for (j = 0; j < hw; j++) {
                    new->weights_ih[i * hw + j] = net->weights_ih[i * hw + j];
                }
            }
        }
        if ((new->weights_hh != NULL) && (net->weights_hh != NULL)) {
            for (i = 0; i < hw; i++) {
                for (j = 0; j < hw; j++) {
                    new->weights_hh[i * hw + j] = net->weights_hh[i * hw + j];
                }
            }
        }
        if ((new->weights_ho != NULL) && (net->weights_ho != NULL)) {
            for (i = 0; i < (hw+1); i++) {
                for (j = 0; j < ow; j++) {
                    new->weights_ho[i * ow + j] = net->weights_ho[i * ow + j];
                }
            }
        }
    }
    return(new);
}

void network_tell_input(Network *net, double *vector)
{
    /* SRN Version: */

    int i;
    if (net->units_in != NULL) {
        for (i = 0; i < net->in_width; i++) {
            net->units_in[i] = vector[i];
        }
    }
}

void network_tell_propagate(Network *net)
{
    /* SRN Version: */

    int i, j;

    /* Propagate from hidden to output: */

    if (net->tmp_out != NULL) {
        for (j = 0; j < net->out_width; j++) {
            net->tmp_out[j] = 0;
            for (i = 0; i < (net->hidden_width+1); i++) {
                net->tmp_out[j] += net->units_hidden[i] * net->weights_ho[i * net->out_width + j];
            }
        }
        for (j = 0; j < net->out_width; j++) {
            net->units_out[j] = sigmoid(net->tmp_out[j]);
        }
    }

    /* Propagate from input and hidden to hidden: */

    if (net->tmp_hidden != NULL) {
        for (j = 0; j < net->hidden_width; j++) {
            net->tmp_hidden[j] = 0;
            for (i = 0; i < (net->in_width+1); i++) {
                net->tmp_hidden[j] += net->units_in[i] * net->weights_ih[i * net->hidden_width + j];
            }
            /* Add in the recycled activation from hidden to hidden: */
            for (i = 0; i < net->hidden_width; i++) {
                net->tmp_hidden[j] += net->units_hidden[i] * net->weights_hh[i * net->hidden_width + j];
            }
        }
        for (j = 0; j < net->hidden_width; j++) {
            net->units_hidden[j] = sigmoid(net->tmp_hidden[j]);
        }
    }
}

void network_tell_propagate2(Network *net)
{
    /* SRN Version: */

    int i, j;

    /* Propagate from input and hidden to hidden: */

    if (net->tmp_hidden != NULL) {
        for (j = 0; j < net->hidden_width; j++) {
            net->tmp_hidden[j] = 0;
            for (i = 0; i < (net->in_width+1); i++) {
                net->tmp_hidden[j] += net->units_in[i] * net->weights_ih[i * net->hidden_width + j];
            }
            /* Add in the recycled activation from hidden to hidden: */
            for (i = 0; i < net->hidden_width; i++) {
                net->tmp_hidden[j] += net->units_hidden[i] * net->weights_hh[i * net->hidden_width + j];
            }
        }
        for (j = 0; j < net->hidden_width; j++) {
            net->units_hidden[j] = sigmoid(net->tmp_hidden[j]);
        }
    }

    /* Propagate from hidden to output: */

    if (net->tmp_out != NULL) {
        for (j = 0; j < net->out_width; j++) {
            net->tmp_out[j] = 0;
            for (i = 0; i < (net->hidden_width+1); i++) {
                net->tmp_out[j] += net->units_hidden[i] * net->weights_ho[i * net->out_width + j];
            }
        }
        for (j = 0; j < net->out_width; j++) {
            net->units_out[j] = sigmoid(net->tmp_out[j]);
        }
    }
}

/******************************************************************************/
/******************************************************************************/

static void training_sequences_randomise(SequenceList *sequences)
{
    if (sequences != NULL) {
        SequenceList *tmp, *this;
    
        for (tmp = sequences; tmp->tail != NULL; tmp = tmp->tail) {
            PatternList *pl;
            int i = random_int(sequence_list_length(tmp));

            this = tmp->tail;
            while (--i > 0) {
                this = this->tail;
            }

            pl = this->head;
            this->head = tmp->head;
            tmp->head = pl;
        }
    }
}

static void network_train_initialise_deltas(Network *net)
{
    /* SRN Version: */

    int i, j;

    for (i = 0; i < (net->in_width+1); i++) {
        for (j = 0; j < net->hidden_width; j++) {
            net->tmp_ih_deltas[i * net->hidden_width + j] = 0.0;
        }
    }
    for (i = 0; i < net->hidden_width; i++) {
        for (j = 0; j < net->hidden_width; j++) {
            net->tmp_hh_deltas[i * net->hidden_width + j] = 0.0;
        }
    }
    for (i = 0; i < (net->hidden_width+1); i++) {
         for (j = 0; j < net->out_width; j++) {
            net->tmp_ho_deltas[i * net->out_width + j] = 0.0;
        }
    }
}

static void network_train_update_weights(Network *net, double lr)
{
    /* SRN Version: */

    int i, j;

    for (i = 0; i < (net->in_width+1); i++) {
        for (j = 0; j < net->hidden_width; j++) {
            net->weights_ih[i * net->hidden_width + j] -= lr * net->tmp_ih_deltas[i * net->hidden_width + j];
        }
    }
    for (i = 0; i < net->hidden_width; i++) {
        for (j = 0; j < net->hidden_width; j++) {
            net->weights_hh[i * net->hidden_width + j] -= lr * net->tmp_hh_deltas[i * net->hidden_width + j];
        }
    }
    for (i = 0; i < (net->hidden_width+1); i++) {
        for (j = 0; j < net->out_width; j++) {
            net->weights_ho[i * net->out_width + j] -= lr * net->tmp_ho_deltas[i * net->out_width + j];
        }
    }
}

static void network_calculate_weight_changes(Network *net, PatternList *patterns, double (*net_error_function)(), Boolean penalty)
{
    /* SRN learning: An implementation of BPTT, based on Williams & Zipser, 1995 */

    int n = pattern_list_length(patterns);
//    int hw = net->hidden_width;
    int units = net->hidden_width + net->out_width;
    double *history_hidden = (double *)malloc((n+2) * net->hidden_width * sizeof(double));
    double *history_error = (double *)malloc((n+2) * net->out_width * sizeof(double));
    double *history_out = (double *)malloc((n+2) * net->out_width * sizeof(double));
    double *delta = (double *)malloc((n+2) * units * sizeof(double));
    double *epsilon = (double *)malloc((n+2) * units * sizeof(double));
    PatternList *this, *prev;
    double y, weight_lk, df;
    int i, j, k, l, t;

    /* 1: Run the network over the entire sequence and collect its state: */

    prev = NULL;
    this = patterns;
    network_tell_randomise_hidden_units(net); /* 10/02/04 */
    for (t = 0; t < n+2; t++) {
        if (this != NULL) {
            network_tell_input(net, this->vector_in);
        }
        network_ask_hidden(net, &history_hidden[t * net->hidden_width]);
        network_ask_output(net, &history_out[t * net->out_width]);
        network_tell_propagate(net);

        this = (this == NULL ? NULL : this->next);
        prev = ((t == 2) ? patterns : (t > 2 ? prev->next : NULL));

        for (i = 0; i < net->out_width; i++) {
            history_error[t * net->out_width + i] = (prev == NULL ? 0.0 : net_error_function(prev->vector_out[i], history_out[t * net->out_width + i]));
        }
    }

#if DEBUG
    fprintf(stdout, "HISTORY_ERROR\n");
    for (t = 0; t < n+2; t++) {
        fprintf(stdout, "t@%d (i=0..%d): ", t, net->out_width);
        for (i = 0; i < net->out_width; i++) {
            fprintf(stdout, " %5.3f", history_error[t * net->out_width + i]);
        }
        fprintf(stdout, "\n");
    }
    fprintf(stdout, "\n");
#endif

    /* 2: Calculate epsilon and delta for each non-input unit (equations 17-19): */

    for (t = n+1; t > 0; t--) {
        for (k = 0; k < units; k++) {
            /* Calculate the external error for each node: */
            epsilon[t * units + k] = (k < net->hidden_width ? 0.0 : history_error[t * net->out_width + (k - net->hidden_width)]);

            /* Add in the sum of delta terms to epsilon if appropriate: */
            if (t < (n+1)) {
                for (l = 0; l < units; l++) {
                    if (k < net->hidden_width) {
                        if (l < net->hidden_width) {
                            /* Weight from hidden unit k to hidden unit l: */
                            weight_lk  = net->weights_hh[k*net->hidden_width+l];
                        }
                        else {
                            /* Weight from hidden unit k to output unit (l-hidden_width): */
                            weight_lk  = net->weights_ho[k*net->out_width+(l-net->hidden_width)];
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
            y = (k < net->hidden_width ? history_hidden[t * net->hidden_width + k] : history_out[t * net->out_width + (k - net->hidden_width)]);
            df = y * (1-y);
            if (penalty && (k < net->hidden_width)) {
                df = df + (2 * 0.05 * (history_hidden[t * net->hidden_width + k] - history_hidden[(t-1) * net->hidden_width + k]));
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

    this = patterns;
    for (t = 1; t < (n+2); t++) {

        /* 3.1: Input to hidden: */
        for (i = 0; i < net->in_width; i++) {
            for (j = 0; j < net->hidden_width; j++) {
                net->tmp_ih_deltas[i * net->hidden_width + j] -= delta[t * units + j] * (this == NULL ? 0.0 : this->vector_in[i]);
            }
        }
        /* The input bias to hidden weights: */
        for (j = 0; j < net->hidden_width; j++) {
            net->tmp_ih_deltas[net->in_width * net->hidden_width + j] -= delta[t * units + j] * 1.0;
        }

        /* 3.2: Hidden to hidden (remember no bias): */
        for (i = 0; i < net->hidden_width; i++) {
            for (j = 0; j < net->hidden_width; j++) {
                net->tmp_hh_deltas[i * net->hidden_width + j] -= delta[t * units + j] * history_hidden[(t-1) * net->hidden_width + i];
            }
        }

        /* 3.3: Hidden to output: */
        for (i = 0; i < net->hidden_width; i++) {
            for (j = 0; j < net->out_width; j++) {
                net->tmp_ho_deltas[i * net->out_width + j] -= delta[t * units + net->hidden_width + j] * history_hidden[(t-1) * net->hidden_width + i];
            }
        }
        /* The hidden bias to output weights: */
        for (j = 0; j < net->out_width; j++) {
            net->tmp_ho_deltas[net->hidden_width * net->out_width + j] -= delta[t * units + net->hidden_width + j] * 1.0;
        }
        if (this != NULL) {
            this = this->next;
        }
    }

    /* 4: Release all allocated space: */
    free(history_hidden);
    free(history_out);
    free(history_error);
    free(delta);
    free(epsilon);
}

Boolean network_train(Network *net, SequenceList *seqs, double lr, ErrorFunction ef, WeightUpdateTime wut, Boolean penalty)
{
    /* SRN Version: */

    double (*net_error_function)() = NULL;

    switch (ef) {
        case SUM_SQUARE_ERROR: {
            net_error_function = net_error_ssq; break;
        }
        case RMS_ERROR: { // Same as SSE
            net_error_function = net_error_ssq; break;
        }
        case CROSS_ENTROPY: {
            net_error_function = net_error_cross_entropy; break;
        }
        case SOFT_MAX_ERROR: {
            net_error_function = net_error_soft_max; break;
        }
    }

    if ((net->tmp_ih_deltas == NULL) || (net->tmp_hh_deltas == NULL) || (net->tmp_ho_deltas == NULL)) {
        return(FALSE);
    }

    if (wut == UPDATE_BY_EPOCH) {

        /* Initialise weight changes: */
        network_train_initialise_deltas(net);

        /* Run the training data, accumulating weight changes over all sequences: */
        while (seqs != NULL) {
            network_calculate_weight_changes(net, seqs->head, net_error_function, penalty);
            seqs = seqs->tail;
        }

        /* Now update the weights: */
        network_train_update_weights(net, lr);
    }
    else {

        /* First randomise the training patterns on each epoch: */
        training_sequences_randomise(seqs);

        /* Now train with each sequence */
        while (seqs != NULL) {

            /* Initialise weight changes: */
            network_train_initialise_deltas(net);

            /* Run the training data, over the current pattern: */
            network_calculate_weight_changes(net, seqs->head, net_error_function, penalty);

            /* Now update the weights: */
            network_train_update_weights(net, lr);

            /* And move on to the next sequence: */
            seqs = seqs->tail;
        }
    }

    return(TRUE);
}

/******************************************************************************/

static double network_srn_test_pattern(Network *net, PatternList *patterns, ErrorFunction ef)
{
    double *real_out = net->tmp_out;
    PatternList *this;
    double e = 0.0;

    for (this = patterns; this != NULL; this = this->next) {
        network_tell_input(net, this->vector_in);
        network_tell_propagate2(net);
        network_ask_output(net, real_out);
        e += vector_compare(ef, net->out_width, this->vector_out, real_out);
    }
    return(e);
//    return(e / (double) pattern_list_length(patterns));
}

double network_test(Network *net, SequenceList *seqs, ErrorFunction ef)
{
    /* SRN Version: */

    double error = 0.0;
    int l = 0;

    while (seqs != NULL) {
        PatternList *pattern = seqs->head;
        network_tell_randomise_hidden_units(net);
        error += network_srn_test_pattern(net, pattern, ef);
        l += pattern_list_length(pattern);
//        l++;
        seqs = seqs->tail;
    }
    return(error / (double) (l * net->out_width));
}

/******************************************************************************/

Boolean network_dump_weights(FILE *fp, Network *net)
{
    /* SRN Version: */

    fprintf(fp, "%% %dI > %dH\n", net->in_width, net->hidden_width);
    if (!dump_matrix(fp, net->in_width+1, net->hidden_width, net->weights_ih)) {
        return(FALSE);
    }

    fprintf(fp, "%% %dH > %dH\n", net->hidden_width, net->hidden_width);
    if (!dump_matrix(fp, net->hidden_width, net->hidden_width, net->weights_hh)) {
        return(FALSE);
    }

    fprintf(fp, "%% %dH > %dO\n", net->hidden_width, net->out_width);
    if (!dump_matrix(fp, net->hidden_width+1, net->out_width, net->weights_ho)) {
        return(FALSE);
    }

    return(TRUE);
}

/*----------------------------------------------------------------------------*/

int network_restore_weights(FILE *fp, Network *net)
{
    /* SRN Version: */

    int i, h, o;
    double *ih, *ho, *hh;

    /* Read the input to hidden matrix: */

    if (!net_read_matrix_header(fp, 'I', 'H', &i, &h)) {
        return(ERROR_RW_HEADER1);
    }
    else if ((ih = (double *)malloc((i+1) * h * sizeof(double))) == NULL) {
        return(ERROR_RW_ALLOC1);
    }
    else if (!net_read_matrix_data(fp, i+1, h, ih)) {
        free(ih);
        return(ERROR_RW_READ1);
    }

    /* Read the hidden to hidden matrix: */

    if (!net_read_matrix_header(fp, 'H', 'H', &h, &h)) {
        free(ih);
        return(ERROR_RW_HEADER2);
    }
    else if ((hh = (double *)malloc(h * h * sizeof(double))) == NULL) {
        free(ih);
        return(ERROR_RW_ALLOC2);
    }
    else if (!net_read_matrix_data(fp, h, h, hh)) {
        free(hh);
        free(ih);
        return(ERROR_RW_READ2);
    }

    /* Read the hidden to output matrix: */

    if (!net_read_matrix_header(fp, 'H', 'O', &h, &o)) {
        free(ih);
        free(hh);
        return(ERROR_RW_HEADER3);
    }
    else if ((ho = (double *)malloc((h+1) * o * sizeof(double))) == NULL) {
        free(ih);
        free(hh);
        return(ERROR_RW_ALLOC3);
    }
    else if (!net_read_matrix_data(fp, h+1, o, ho)) {
        free(ih);
        free(hh);
        free(ho);
        return(ERROR_RW_READ1);
    }

    /* Set up the input units, hidden and output units: */

    if (net->in_width != i) {
        net->in_width = i;
        free(net->units_in);
        net->units_in = (double *)malloc((net->in_width+1) * sizeof(double));
    }
    if (net->hidden_width != h) {
        net->hidden_width = h;
        free(net->units_hidden);
        net->units_hidden = (double *)malloc((net->hidden_width+1) * sizeof(double));
    }
    if (net->out_width != o) {
        net->out_width = o;
        free(net->units_out);
        net->units_out = (double *)malloc(net->out_width * sizeof(double));
    }

    /* Replace the old weights with the new: */

    free(net->weights_ih);
    net->weights_ih = ih;

    free(net->weights_hh);
    net->weights_hh = hh;

    free(net->weights_ho);
    net->weights_ho = ho;

    /* Reallocate the temporary space (in case the network's size has changed): */

    free(net->tmp_hidden);
    net->tmp_hidden = (double *)malloc(net->hidden_width * sizeof(double));
    free(net->tmp_out);
    net->tmp_out = (double *)malloc(net->out_width * sizeof(double));
    free(net->tmp_ih_deltas);
    net->tmp_ih_deltas = (double *)malloc((net->in_width+1) * net->hidden_width * sizeof(double));
    free(net->tmp_hh_deltas);
    net->tmp_hh_deltas = (double *)malloc(net->hidden_width * net->hidden_width * sizeof(double));
    free(net->tmp_ho_deltas);
    net->tmp_ho_deltas = (double *)malloc((net->hidden_width+1) * net->out_width * sizeof(double));

   return(0);
}

/******************************************************************************/

void training_set_free(SequenceList *patterns)
{
    sequence_list_free(patterns);
}

int training_set_length(SequenceList *patterns)
{
    return(sequence_list_length(patterns));
}

SequenceList *training_set_read(char *file, int w_in, int w_out)
{
    return(read_training_sequences(file, w_in, w_out));
}

SequenceList *training_set_next(SequenceList *current)
{
    return(current->tail);
}

double *training_set_input_vector(SequenceList *this)
{
    return(this->head->vector_in);
}

/******************************************************************************/

double vector_net_conflict(double *vector, int width)
{
    double conflict = 0.0;
    int i, j;

    for (i = 0; i < width; i++) {
        for (j = i+1; j < width; j++) {
            conflict += vector[i] * vector[j];
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

