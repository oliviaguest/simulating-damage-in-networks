/******************************************************************************/

// Run MAX_CYCLES cycles, writing the error to standard output every
// ERR_WRITE_CYCLES cycles

#include "bp.h"
#include <glib.h>
#include <time.h>
#include <sys/stat.h>

#define LEARNING_RATE 0.001
#define ERROR_FUNCTION CROSS_ENTROPY
#define UPDATE_TIME UPDATE_BY_EPOCH

#define MAX_CYCLES           20000
#define ERR_WRITE_CYCLES      1000

// extern int usertime(); /* Return total milliseconds of user time */
// extern int systime();  /* Return total milliseconds of system time */

void action_log_initialise() { }
void action_log_record(ActionType act, int cycle, char *arg1, char *arg2) { }

static void save_weights(Network *net, char *weight_prefix, int i)
{
    char filename[64];
    FILE *fp;

    g_snprintf(filename, 64, "%s_%d.weights", weight_prefix, i);

    if ((fp = fopen(filename, "w")) == NULL) {
        fprintf(stderr, "ERROR: Cannot write to %s\n", filename);
    }
    else if (!network_dump_weights(fp, net)) {
        fprintf(stderr, "ERROR: Problem dumping weights ... weights not saved\n");
        fclose(fp);
        remove(filename);
    }
    else {
        fprintf(stderr, "Weights successfully saved to %s\n", filename);
        fclose(fp);
    }
}

static void run_and_save(int j, char *training_set, char *weight_prefix)
{
    Network *net;
    TrainingDataList *tdl;
    double rms, cross_entropy, sme;
    TaskType task = {TASK_COFFEE, DAMAGE_NONE, {TRUE, FALSE, FALSE, FALSE, FALSE}};
    int i;

    world_initialise(&task);
    tdl = training_set_read(training_set, IN_WIDTH, OUT_WIDTH);
    if ((net = network_create(IN_WIDTH, HIDDEN_WIDTH, OUT_WIDTH)) != NULL) {
        for (i = 0; i < MAX_CYCLES; i++) {
            if (i == (ERR_WRITE_CYCLES * (i / ERR_WRITE_CYCLES))) {
                rms = sqrt(network_test(net, tdl, SUM_SQUARE_ERROR));
                cross_entropy = network_test(net, tdl, CROSS_ENTROPY);
                sme = network_test(net, tdl, SOFT_MAX_ERROR);
                fprintf(stdout, "%3d: RMS Error: %7.5f; Cross Entropy: %7.5f; SoftMax Error: %7.5f\n", i, rms, cross_entropy, sme);
            }
            network_train(net, tdl, LEARNING_RATE, ERROR_FUNCTION, UPDATE_TIME, FALSE);
        }
        save_weights(net, weight_prefix, j);
        network_tell_destroy(net);
    }
    training_set_free(tdl);
}

int main(int argc, char **argv)
{
    int i;
    long t0 = (long) time(NULL);

    srand((int) t0);
    fprintf(stdout, "BEFORE: User time: %f; System time: %f\n", usertime()*0.001, systime()*0.001);
    for (i = 0; i < 1; i++) {
        run_and_save(i, TRAINING_SET, "weights_bp_marc");
    }
    fprintf(stdout, "AFTER:  User time: %f; System time: %f\n", usertime()*0.001, systime()*0.001);
    exit(0);
}

/******************************************************************************/

