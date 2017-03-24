
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define IO_WIDTH      24
#define HIDDEN_WIDTH  20

#define DEBUG 0

#define TRAINING_PATTERNS "DATA/tyler_etal_2000.pat"

typedef enum {FALSE, TRUE} Boolean;
typedef enum {DAMAGE_NONE, DAMAGE_NOISE, DAMAGE_THRESHOLD} DamageType;
typedef enum {F_DISTINCTIVE, F_SHARED, F_FUNCTIONAL} FeatureType;

#include "utils_network.h"

/* Defined in utils_time.c: ***************************************************/

extern int usertime(); /* Return total milliseconds of user time */
extern int systime();  /* Return total milliseconds of system time */

/* Defined in world.c: ********************************************************/

#define WES_LENGTH 128
extern char world_error_string[WES_LENGTH];

extern void world_set_network_input_vector(double *vector, PatternList *training_set, int p);
extern Boolean world_decode_output_vector(char *buffer, int l, double *vector);
extern double response_error(PatternList *input, double *result, FeatureType ft);

/******************************************************************************/

extern PatternList *world_decode_vector(PatternList *training_set, double *vector);
extern double euclidean_distance(int n, double *a, double *b);
extern Boolean pattern_is_animal(PatternList *p);
extern Boolean pattern_is_animal1(PatternList *p);
extern Boolean pattern_is_animal2(PatternList *p);
extern Boolean pattern_is_artifact(PatternList *p);
extern Boolean pattern_is_artifact1(PatternList *p);
extern Boolean pattern_is_artifact2(PatternList *p);
extern Boolean response_is_correct(PatternList *training_set, PatternList *p, double *r);
extern Boolean response_is_animal1(PatternList *training_set, double *r);
extern Boolean response_is_animal2(PatternList *training_set, double *r);
extern Boolean response_is_artifact1(PatternList *training_set, double *r);
extern Boolean response_is_artifact2(PatternList *training_set, double *r);
