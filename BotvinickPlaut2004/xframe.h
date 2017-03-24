/******************************************************************************/

#include "bp.h"
#include "gtkx.h"
#include <string.h>

/******************************************************************************/

typedef struct x_globals {
    GtkWidget        *frame;
    GtkWidget        *training_set_label;
    GtkWidget        *weight_history_label;
    GtkWidget        *weight_history_cycles;
    Network          *net;
    TrainingDataList *first;
    TrainingDataList *next;
    int               cycle;
    int               step;
    double            conflict[100];
    double            output[100][OUT_WIDTH];
    Boolean           pause;
    Boolean           colour;
} XGlobals;

typedef struct parameters {
    double           lr;
    ErrorFunction    ef;
    WeightUpdateTime wut;
    Boolean          penalty;
    Boolean          sugar_closed;
    Boolean          mug_with_coffee;
    Boolean          mug_with_tea;
    Boolean          mug_with_cream;
    Boolean          mug_with_sugar;
} Parameters;

extern XGlobals xg;

extern Parameters pars;

#define OUTPUT_FOLDER "OUTPUT/"

/******************************************************************************/
