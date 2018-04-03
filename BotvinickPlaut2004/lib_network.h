#ifndef _lib_network_h_

#define _lib_network_h_

/******************************************************************************/

typedef struct pattern_list {
    double *vector_in;
    double *vector_out;
    struct pattern_list *next;
} PatternList;

typedef struct sequence_list {
    PatternList *head;
    struct sequence_list *tail;
} SequenceList;

extern int pattern_list_length(PatternList *patterns);
extern void pattern_list_free(PatternList *patterns);

extern PatternList *read_training_patterns(char *filename, int in_width, int out_width);
extern SequenceList *read_training_sequences(char *filename, int in_width, int out_width);

extern void print_vector(FILE *fp, int n, double *vector_in);
extern void print_string(FILE *fp, int width, char *string);

extern int sequence_list_length(SequenceList *seqs);
extern void sequence_list_free(SequenceList *seqs);
extern void sequence_list_print_stats(SequenceList *seqs, FILE *fp);


/******************************************************************************/

typedef enum {UPDATE_BY_ITEM, UPDATE_BY_EPOCH} WeightUpdateTime;
typedef enum {SUM_SQUARE_ERROR, RMS_ERROR, CROSS_ENTROPY, SOFT_MAX_ERROR} ErrorFunction;

#define TrainingDataList SequenceList
#define WEIGHT_PREFIX "WEIGHTS_SRN"

typedef struct network {
    int in_width, hidden_width, out_width;
    double *weights_ih, *weights_ho, *weights_hh;
    double *units_in, *units_hidden, *units_out;
    double *tmp_hidden;
    double *tmp_out;
    double *tmp_ih_deltas;
    double *tmp_hh_deltas;
    double *tmp_ho_deltas;
} Network;

extern Network *network_create(int iw, int hw, int ow);
extern Network *network_copy(Network *net);
extern void network_tell_initialise(Network *net);
extern void network_tell_destroy(Network *net);
extern void network_tell_input(Network *net, double *vector);
extern void network_tell_randomise_hidden_units(Network *net);
extern void network_tell_propagate(Network *net);
extern void network_tell_propagate2(Network *net);
extern void network_ask_input(Network *net, double *vector);
extern void network_ask_hidden(Network *net, double *vector);
extern void network_ask_output(Network *net, double *vector);
extern void network_perturb_weights_ih(Network *net, double variance);
extern void network_perturb_weights_ch(Network *net, double variance);
extern void network_inject_noise(Network *net, double sv_noise);
extern void network_print_state(FILE *fp, Network *net, char *message);
extern void network_lesion_weights_ih(Network *net, double severity);
extern void network_lesion_weights_ch(Network *net, double severity);
extern double network_test(Network *net, TrainingDataList *test_patterns, ErrorFunction ef);
extern Boolean network_train(Network *net, TrainingDataList *test_patterns, double lr, ErrorFunction ef, WeightUpdateTime wut, Boolean penalty);

extern void training_set_free(TrainingDataList *patterns);
extern int training_set_length(TrainingDataList *patterns);
extern TrainingDataList *training_set_read(char *file, int in_w, int out_w);
extern TrainingDataList *training_set_next(TrainingDataList *patterns);
extern double *training_set_input_vector(TrainingDataList *patterns);

extern Boolean network_dump_weights(FILE *fp, Network *net);
extern int network_restore_weights(FILE *fp, Network *net);

extern double vector_net_conflict(double *vector, int width);

#endif
