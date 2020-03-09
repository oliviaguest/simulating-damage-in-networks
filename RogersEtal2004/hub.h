#ifndef _hub_h_

#define _hub_h_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lib_error.h"

#define NUM_NAME      40
#define NUM_VERBAL   112
#define NUM_VISUAL    64
#define NUM_SEMANTIC  64

#define NUM_IO (NUM_NAME + NUM_VERBAL + NUM_VISUAL)

typedef enum {FALSE, TRUE} Boolean;

#include "utils_hub.h"

#endif
