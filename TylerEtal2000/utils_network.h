#include <stdio.h>

#define NET_SETTLING_THRESHOLD 0.000001

typedef struct pattern_list {
    char *name;
    double *vector_in;
    double *vector_out;
    struct pattern_list *next;
} PatternList;

typedef enum {SUM_SQUARE_ERROR, CROSS_ENTROPY, SOFT_MAX_ERROR} ErrorFunction;
typedef enum {UPDATE_BY_ITEM, UPDATE_BY_EPOCH} WeightUpdateTime;
typedef enum {ERROR_NONE, 
    ERROR_RW_HEADER1, ERROR_RW_ALLOC1, ERROR_RW_READ1,
    ERROR_RW_HEADER2, ERROR_RW_ALLOC2, ERROR_RW_READ2,
    ERROR_RW_HEADER3, ERROR_RW_ALLOC3, ERROR_RW_READ3} ErrorType;
typedef enum {NT_FEEDFORWARD, NT_RECURRENT, NT_MAX} NetworkType;

// We treat the network as an object, which we ask and tell things.

typedef struct network {
    NetworkType      nt;       // FF or Recurrent
    Boolean          settled;  // Only relevant for recurrent
    short            cycles;   // Cycles of settling for RAN
    int in_width, hidden_width, out_width;
    double *weights_ih, *weights_ho, *weights_hh;
    double *units_in, *units_hidden, *units_hidden_prev, *units_out;
    double *tmp_ih_deltas, *tmp_hh_deltas, *tmp_ho_deltas;
    double *previous_ih_deltas, *previous_hh_deltas, *previous_ho_deltas;
} Network;

typedef struct network_parameters {
    NetworkType      nt;       // FF or Settling_RAN
    double           wn;       // Initial weight noise
    double           lr;       // Learning rate
    double           momentum; // Momentum for learning
    double           wd;       // Weight decay
    ErrorFunction    ef;       // Error to be minimised
    WeightUpdateTime wut;      // Update by item or by epoch
    int epochs;                // Maximum number of epochs to train
    double criterion;          // Criterion error when training should terminate
} NetworkParameters;

// Compile for clamped or unclamped version of RAN:
#define CLAMPED
// FIXME: The above would be much better as a network parameter

/* Defined in utils_io.c: *****************************************************/

extern int pattern_list_length(PatternList *patterns);
extern void pattern_list_print(PatternList *patterns, int in_width, int out_width, FILE *fp);
extern void pattern_list_free(PatternList *patterns);

extern PatternList *read_training_patterns(char *filename, int in_width, int out_width);
extern void print_vector(FILE *fp, int n, double *vector_in);
extern void print_string(FILE *fp, int width, char *string);

#if FALSE
extern void pattern_set_free(Patterns *list);
extern Patterns *pattern_set_read(char *File, int width);
#endif

/* Defined in utils_network.c: ************************************************/

extern char *nt_name[NT_MAX];

extern void network_destroy(Network *n);
extern Network *network_initialise(NetworkType nt, int iw, int hw, int ow);
extern void network_initialise_weights(Network *net, double weight_noise);
extern Network *network_copy(Network *net);
extern void network_tell_input(Network *n, double *vector);
extern void network_tell_randomise_hidden(Network *n);
extern void network_tell_propagate(Network *n);
extern void network_tell_propagate_full(Network *n);
extern void network_ask_input(Network *n, double *vector);
extern void network_ask_hidden(Network *n, double *vector);
extern void network_ask_output(Network *n, double *vector);
extern void network_print_state(Network *n, FILE *fp, char *message);
extern void network_lesion_weights_ih(Network *n, double severity);
extern void network_lesion_weights_hh(Network *n, double severity);
extern void network_lesion_weights_ho(Network *n, double severity);
extern void network_lesion_weights(Network *net, double severity);
extern void network_perturb_weights_ih(Network *net, double noise_sd);
extern void network_perturb_weights_hh(Network *net, double noise_sd);
extern void network_perturb_weights_ho(Network *net, double noise_sd);
extern void network_perturb_weights(Network *net, double noise_sd);
extern void network_ablate_units(Network *net, double severity);
extern void network_scale_weights(Network *net, double scale);
extern double network_test(Network *n, PatternList *test_patterns, ErrorFunction ef);
extern Boolean network_train(Network *n, NetworkParameters *pars, PatternList *test_patterns);
extern void network_train_to_criterion(Network *n, NetworkParameters *pars, PatternList *seqs);
extern void network_train_to_epochs(Network *n, NetworkParameters *pars, PatternList *seqs);
extern void training_set_free(PatternList *patterns);
extern int training_set_length(PatternList *patterns);
extern PatternList *training_set_read(char *file, int in_w, int out_w);
extern PatternList *training_set_next(PatternList *patterns);
extern double *training_set_input_vector(PatternList *patterns);
extern Boolean network_dump_weights(Network *n, FILE *fp);
extern Network *network_read_weights_from_file(FILE *fp, int *error);
extern double net_conflict(Network *n);
extern void network_inject_noise(Network *n, double sv_noise);

extern double network_weight_minimum(Network *net);
extern double network_weight_maximum(Network *net);

/******************************************************************************/
