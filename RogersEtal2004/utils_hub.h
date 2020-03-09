#ifndef _utils_hub_h_

#define _utils_hub_h_

#include <stdio.h>

#include <glib.h>

typedef enum category_type {CAT_BIRD, CAT_MAMMAL, CAT_FRUIT, 
                            CAT_TOOL, CAT_VEHICLE, CAT_HOUSEHOLD, CAT_MAX} CategoryType;

typedef struct pattern_list {
    char *name;
    CategoryType category;
    double name_features[NUM_NAME];
    double verbal_features[NUM_VERBAL];
    double visual_features[NUM_VISUAL];
    struct pattern_list *next;
} PatternList;

typedef struct clamp_type {
    int from;
    int to;
    double vector[NUM_IO]; // negative values mean don't clamp it
} ClampType;

typedef enum {EF_SUM_SQUARE, EF_CROSS_ENTROPY, EF_SOFT_MAX, EF_MAX} ErrorFunction;
typedef enum {WU_BY_ITEM, WU_BY_EPOCH, WU_MAX} WeightUpdateTime;
typedef enum {NT_FEEDFORWARD, NT_RECURRENT, NT_MAX} NetworkType;
typedef enum {UI_RANDOM, UI_FIXED, UI_MAX} UnitInitialisation;

extern char *net_ef_label[EF_MAX];
extern char *net_wu_label[WU_MAX];

// We treat the network as an object, which we ask and tell things.

typedef struct network_parameters {
    double           wn;         // Initial weight noise
    double           lr;         // Learning rate
    double           momentum;   // Momentum for learning
    double           wd;         // Weight decay
    UnitInitialisation ui;       // Initialise units to random or fixed values
    int              ticks;      // For time average of activation calculation
    int              sc;         // Settling cycles (or 0 if settle to threshold)
    double           st;         // Settling threshold
    ErrorFunction    ef;         // Error to be minimised
    WeightUpdateTime wut;        // Update by item or by epoch
    int              epochs;     // Maximum number of epochs to train
    double           criterion;  // Criterion error when training should terminate
} NetworkParameters;

typedef struct network {
    NetworkType      nt;       // FF or Recurrent
    Boolean          settled;  // True when settled
    short            cycles;   // Cycles of settling for SRN
    int in_width, hidden_width, out_width;
    double *weights_ih, *weights_ho, *weights_hh;
    double *units_in, *units_hidden, *units_hidden_prev, *units_out;
    double *tmp_ih_deltas, *tmp_hh_deltas, *tmp_ho_deltas;
    double *previous_ih_deltas, *previous_hh_deltas, *previous_ho_deltas;
    NetworkParameters params;
} Network;

/* Defined in utils_hub.c: ****************************************************/

extern int pattern_list_length(PatternList *list);
extern void hub_pattern_set_free(PatternList *list);
extern PatternList *hub_pattern_set_read(char *filename);

extern double pattern_get_distance(PatternList *best, double *vector);
extern double pattern_get_name_distance(PatternList *best, double *vector);
extern double pattern_get_max_bit_error(PatternList *best, double *vector);
extern PatternList *pattern_list_get_best_name_match(PatternList *patterns, double *vector);
extern PatternList *pattern_list_get_best_feature_match(PatternList *patterns, double *vector);

/* Defined in utils_network.c: ************************************************/

extern char *nt_name[NT_MAX];

extern void network_destroy(Network *net);
extern Network *network_create(NetworkType nt, int iw, int hw, int ow);
extern void network_parameters_set(Network *net, NetworkParameters *params);
extern void network_initialise_weights(Network *net);
extern Network *network_copy(Network *net);
extern void network_tell_input(Network *n, double *vector);
extern void network_tell_initialise_input(Network *n);
extern void network_tell_initialise_hidden(Network *n);
extern void network_tell_initialise_output(Network *n);
extern void network_initialise(Network *n);
extern Boolean network_is_settled(Network *net);
extern void network_tell_propagate(Network *n);
extern void network_tell_propagate_full(Network *n, ClampType *clamp);
extern void network_ask_input(Network *n, double *vector);
extern void network_ask_hidden(Network *n, double *vector);
extern void network_ask_output(Network *n, double *vector);
extern void network_print_state(Network *n, FILE *fp, char *message);
extern void network_sever_weights_ih(Network *n, double severity);
extern void network_sever_weights_hh(Network *n, double severity);
extern void network_sever_weights_ho(Network *n, double severity);
extern void network_sever_weights(Network *net, double severity);
extern void network_perturb_weights_ih(Network *net, double noise_sd);
extern void network_perturb_weights_hh(Network *net, double noise_sd);
extern void network_perturb_weights_ho(Network *net, double noise_sd);
extern void network_perturb_weights(Network *net, double noise_sd);
extern void network_scale_weights_ih(Network *net, double noise_sd);
extern void network_scale_weights_hh(Network *net, double noise_sd);
extern void network_scale_weights_ho(Network *net, double noise_sd);
extern void network_scale_weights(Network *net, double noise_sd);
extern void network_ablate_units(Network *net, double severity);
extern double network_test(Network *n, PatternList *test_patterns, ErrorFunction ef);
extern double network_test_max_bit(Network *n, PatternList *patterns);
extern Boolean network_train(Network *n, PatternList *test_patterns);
extern void network_train_to_criterion(Network *n, PatternList *seqs);
extern void network_train_to_epochs(Network *n, PatternList *seqs);
extern void training_set_free(PatternList *patterns);
extern int training_set_length(PatternList *patterns);
extern PatternList *training_set_read(char *file, int in_w, int out_w);
extern PatternList *training_set_next(PatternList *patterns);
extern double *training_set_input_vector(PatternList *patterns);
extern Boolean network_write_to_file(FILE *fp, Network *n);
extern Network *network_read_from_file(FILE *fp, char **error);
extern double net_conflict(Network *n);
extern void network_inject_noise(Network *n, double sv_noise);

extern void clamp_set_clamp_name(ClampType *clamp, int from, int to, PatternList *p);
extern void clamp_set_clamp_verbal(ClampType *clamp, int from, int to, PatternList *p);
extern void clamp_set_clamp_visual(ClampType *clamp, int from, int to, PatternList *p);
extern void clamp_set_clamp_all(ClampType *clamp, int from, int to, PatternList *p);
extern void network_tell_recirculate_input(Network *net, ClampType *clamp);

extern double network_weight_minimum(Network *net);
extern double network_weight_maximum(Network *net);

/******************************************************************************/

extern void hub_build_name_input_vector(PatternList *pattern, double *vector_out);
extern void hub_build_visual_input_vector(PatternList *pattern, double *vector_out);
extern void hub_build_verbal_input_vector(PatternList *pattern, double *vector_out);

extern Boolean pattern_is_animal(PatternList *p);
extern Boolean pattern_is_artifact(PatternList *p);

#endif
