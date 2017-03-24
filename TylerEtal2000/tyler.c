/******************************************************************************/

// Run to criterion, writing the error to standard output every
// ERR_WRITE_CYCLES cycles

#include "tyler.h"

#include "lib_maths.h"

#include <glib.h>
#include <time.h>
#include <sys/stat.h>

//    NetworkParameters pars: NT, WN, LR, Momentum, Decay, Error, Update, Epochs, Criterion
NetworkParameters pars = 
// {NT_FEEDFORWARD, 0.010, 0.250, 0.100, 0.000, SUM_SQUARE_ERROR, UPDATE_BY_EPOCH, 1000, 0.01};
// {NT_RECURRENT, 0.010, 0.025, 0.100, 0.000, SUM_SQUARE_ERROR, UPDATE_BY_EPOCH, 1000, 0.01};
{NT_RECURRENT, 0.010, 0.025, 0.100, 0.000, SUM_SQUARE_ERROR, UPDATE_BY_EPOCH, 5000, 0.01};

#define ERR_WRITE_CYCLES      1000

#define ATTRACTOR_MAX 1<<12

typedef struct at_tab_entry {
  double vector[HIDDEN_WIDTH];
  int count;
} AtTabEntry;

static AtTabEntry attractor_table[ATTRACTOR_MAX];
static int attractor_count;

#define DAMAGE_LEVELS 21
#define LESION_WEIGHTS TRUE
#define MAX_DAMAGE 1.00
#define MAX_NOISE 3.00
#define LESION_RECORD_FILE "damage_lesion_results.dat"

/******************************************************************************/



static void save_weights(Network *net, char *weight_prefix, int i)
{
    char filename[64];
    FILE *fp;

    g_snprintf(filename, 64, "%s_%03d.dat", weight_prefix, i);

    if ((fp = fopen(filename, "w")) == NULL) {
        fprintf(stderr, "ERROR: Cannot write to %s\n", filename);
    }
    else if (!network_dump_weights(net, fp)) {
        fprintf(stderr, "ERROR: Problem dumping weights ... weights not saved\n");
        fclose(fp);
        remove(filename);
    }
    else {
        fprintf(stderr, "Weights successfully saved to %s\n", filename);
        fclose(fp);
    }
}

void train_and_save(int j, char *training_set_file, char *weight_prefix)
{
    double rms, cross_entropy, sme;
    PatternList *training_set;
    Network *net;
    int i;

    net = network_initialise(pars.nt, IO_WIDTH, HIDDEN_WIDTH, IO_WIDTH);
    network_initialise_weights(net, pars.wn);
    training_set = training_set_read(training_set_file, IO_WIDTH, IO_WIDTH);

//    fprintf(stdout, "Patterns: \n");
//    pattern_list_print(training_set, IO_WIDTH, IO_WIDTH, stdout);

    for (i = 0; i < pars.epochs; i++) {
        if (i == (ERR_WRITE_CYCLES * (i / ERR_WRITE_CYCLES))) {
            rms = sqrt(network_test(net, training_set, SUM_SQUARE_ERROR));
            cross_entropy = network_test(net, training_set, CROSS_ENTROPY);
            sme = 0.0; // network_test(net, training_set, SOFT_MAX_ERROR);
            fprintf(stdout, "%4d: RMS Error: %7.5f; Cross Entropy: %7.5f; SM Error: %7.5f\n", i, rms, cross_entropy, sme);
        }
        network_train(net, &pars, training_set);
    }
    save_weights(net, weight_prefix, j);

    training_set_free(training_set);
    network_destroy(net);
}

static void build_pattern(double vector_in[IO_WIDTH], int i)
{
    int j;

    for (j = 0; j < IO_WIDTH; j++) {
        if (i % 2 == 1) {
            vector_in[j] = 1;
        }
        else {
            vector_in[j] = 0;
        }
        i = i / 2;
    }
}

static void save_attractor(Network *net)
{
    double vector_hidden[HIDDEN_WIDTH];
    int i = 0, j;

    network_ask_hidden(net, vector_hidden);

    while ((i < attractor_count) && (vector_sum_square_difference(HIDDEN_WIDTH, vector_hidden, attractor_table[i].vector) > NET_SETTLING_THRESHOLD)) {
      i++;
    }

    if (i == ATTRACTOR_MAX) {
      fprintf(stdout, "WARNING: TABLE FULL ... TOO MANY ATTRACTORS!\n");
    }
    else if (i == attractor_count) {
        for (j = 0; j < HIDDEN_WIDTH; j++) {
            attractor_table[i].vector[j] = vector_hidden[j];
	}
        attractor_table[i].count = 1;
        attractor_count++;
    }
    else {
        // Found a duplicate attractor!
        attractor_table[i].count++;
    }
}

static int count_attractors_by_damage_level(Network *net)
{
  // Start by just counting the attractors and writing the results to stdout
  // If that isn't too slow, do it for, say, 21 levels of damage and save the results
  // Note: This is only sensible for the unclamped SRN, so make sure clamps are set to 4 in utils_network.c

    int i, in_max;
    double vector_in[IO_WIDTH];

    in_max = 1<<IO_WIDTH;
    attractor_count = 0;

    for (i = 0; i < in_max; i++) {
      // Construct the pattern:
      build_pattern(vector_in, i);
      // set input to pattern i;
      network_tell_input(net, vector_in);
      // run the network until it settles (or 100 cycles);
      network_tell_randomise_hidden(net);
      network_tell_propagate_full(net);
      // add the hidden unit state to the table of state counts
      save_attractor(net);
      // And let the use know what's happening:
      if ((i % (1<<16)) == 0) {
        fprintf(stdout, "%10d inputs (%d %%); %d attractor states\n", i+1, (i+1)*100/in_max, attractor_count);
      }
    }
    fprintf(stdout, "%10d inputs (%d %%); %d attractor states\n", i+1, (i+1)*100/in_max, attractor_count);
    return(attractor_count);
}

void train_and_count_attractors(int j, char *training_set_file)
{
    double rms, cross_entropy, sme;
    PatternList *training_set;
    Network *net;
    FILE *fp;
    int i;

    net = network_initialise(pars.nt, IO_WIDTH, HIDDEN_WIDTH, IO_WIDTH);
    network_initialise_weights(net, pars.wn);
    training_set = training_set_read(training_set_file, IO_WIDTH, IO_WIDTH);

    for (i = 0; i < pars.epochs; i++) {
        if (i == (ERR_WRITE_CYCLES * (i / ERR_WRITE_CYCLES))) {
            rms = sqrt(network_test(net, training_set, SUM_SQUARE_ERROR));
            cross_entropy = network_test(net, training_set, CROSS_ENTROPY);
            sme = 0.0; // network_test(net, training_set, SOFT_MAX_ERROR);
            fprintf(stdout, "%4d: RMS Error: %7.5f; Cross Entropy: %7.5f; SM Error: %7.5f\n", i, rms, cross_entropy, sme);
        }
        network_train(net, &pars, training_set);
    }

    for (i = 0; i < DAMAGE_LEVELS; i++) {
        Network *tmp = network_copy(net);
        double ll;
	int n;

        if (LESION_WEIGHTS) {
            ll = 100 * i * MAX_DAMAGE / (double) (DAMAGE_LEVELS - 1);
            network_lesion_weights(tmp, ll / 100.0);
        }
        else {
            ll = i * MAX_NOISE / (double) (DAMAGE_LEVELS - 1);
            network_perturb_weights(tmp, ll);
	}
	n = count_attractors_by_damage_level(tmp);
	fp = fopen(LESION_RECORD_FILE, "a");
	if (i > 0) {
            fprintf(fp, "\t");
	}
	fprintf(fp, "%d", n);
	fclose(fp);
        network_destroy(tmp);
    }
    fp = fopen(LESION_RECORD_FILE, "a");
    fprintf(fp, "\n");
    fclose(fp);

    training_set_free(training_set);
    network_destroy(net);
}

int main(int argc, char **argv)
{
    FILE *fp;
    int i;
    long t0 = (long) time(NULL);

    srand((int) t0);

    fp = fopen(LESION_RECORD_FILE, "w");
    for (i = 0; i < DAMAGE_LEVELS; i++) {
        double ll;

        if (LESION_WEIGHTS) {
            ll = 100 * i * MAX_DAMAGE / (double) (DAMAGE_LEVELS - 1);
        }
        else {
            ll = i * MAX_NOISE / (double) (DAMAGE_LEVELS - 1);
        }
	if (i > 0) {
            fprintf(fp, "\t");
	}
	fprintf(fp, "%5.1f", ll);
    }
    fprintf(fp, "\n");
    fclose(fp);

    for (i = 0; i < 10; i++) {
        train_and_count_attractors(i, TRAINING_PATTERNS);
        // train_and_save(i, TRAINING_PATTERNS, "DATA/tyler_etal_2000");
    }
    exit(0);
}

/******************************************************************************/

