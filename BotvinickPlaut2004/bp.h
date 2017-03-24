
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define IN_WIDTH 39
#define HIDDEN_WIDTH 50
#define OUT_WIDTH 19

#define DEBUG 0
#define SUGAR_HACK

#ifdef SUGAR_HACK
#define TARGET_SEQUENCES "targets_bp.h"
#define TRAINING_SET "training_sequences_full.dat"
#else
#define TARGET_SEQUENCES "targets_cr.h"
#define TRAINING_SET "DATA_BP_WITHOUT_SUGAR_HACK/training_sequences.dat"
#endif

typedef enum {FALSE, TRUE} Boolean;
typedef enum {TASK_NONE, TASK_COFFEE, TASK_TEA, TASK_MAX} BaseTaskType;
typedef enum {DAMAGE_NONE, DAMAGE_ACTIVATION_NOISE, DAMAGE_WEIGHT_NOISE, DAMAGE_WEIGHT_LESION} DamageType;
typedef enum {ACTION_PICK_UP, ACTION_PUT_DOWN, ACTION_POUR, ACTION_PEEL_OPEN, ACTION_TEAR_OPEN, ACTION_PULL_OPEN, ACTION_PULL_OFF, ACTION_SCOOP, ACTION_SIP, ACTION_STIR, ACTION_DIP, ACTION_SAY_DONE, ACTION_FIXATE_CUP, ACTION_FIXATE_TEABAG, ACTION_FIXATE_COFFEE_PACKET, ACTION_FIXATE_SPOON, ACTION_FIXATE_CARTON, ACTION_FIXATE_SUGAR_PACKET, ACTION_FIXATE_SUGAR_BOWL, ACTION_NONE} ActionType;

extern char *task_name[TASK_MAX];

typedef struct state_type {
    Boolean   bowl_closed;
    Boolean   mug_contains_coffee;
    Boolean   mug_contains_tea;
    Boolean   mug_contains_sugar;
    Boolean   mug_contains_cream;
} StateType;

typedef struct task_type {
    BaseTaskType    base;
    DamageType      damage;
    StateType       initial_state;
} TaskType;

/* The ACS error analyses: ****************************************************/

typedef struct acs1 {
    int actions;
    int independents;
    int errors_crux;
    int errors_non_crux;
} ACS1;

typedef struct acs2 {
    int omissions;
    int anticipations;
    int perseverations;
    int reversals;
    int object_sub;
    int gesture_sub;
    int tool_omission;
    int action_addition;
    int quality;
    int bizarre;
    int accomplished;
} ACS2;

/* Defined in lib_network.c: **************************************************/

#include "lib_network.h"

/* Defined in utils_time.c: ***************************************************/

extern double usertime(); /* Return total milliseconds of user time */
extern double systime();  /* Return total milliseconds of system time */

/* Defined in world.c: ********************************************************/

#define WES_LENGTH 128
extern char world_error_string[WES_LENGTH];

extern void world_initialise(TaskType *task);
extern void world_set_network_input_vector(double *vector);
extern void world_print_state(FILE *fp);
extern ActionType world_get_network_output_action(FILE *fp, double *vector);
extern Boolean world_perform_action(ActionType action);

extern Boolean world_decode_output_vector(char *buffer, int l, double *in_vector);
extern Boolean world_decode_action(char *buffer, int l, ActionType action);
extern Boolean world_decode_held(char *buffer, int l, double *in_vector);
extern Boolean world_decode_viewed(char *buffer, int l, double *in_vector);

/******************************************************************************/
