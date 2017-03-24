#include "bp.h"
#include "utils_maths.h"
#include <stdlib.h>
#include <string.h>
#include <glib.h>

extern void action_log_initialise();
extern void action_log_record(ActionType act, int cycle, char *arg1, char *arg2);

#define NAME_LENGTH 20

typedef enum object_type    {OBJECT_CUP, OBJECT_TEABAG, OBJECT_COFFEE_PACKET, OBJECT_SUGAR_PACKET, OBJECT_SPOON, OBJECT_CARTON, OBJECT_SUGAR_BOWL, OBJECT_SUGAR_BOWL_LID} ObjectType;
typedef enum access_state   {ACCESS_NONE, ACCESS_OPEN, ACCESS_CLOSED} AccessState;

#define CONTAINS_NOTHING     0
#define CONTAINS_TEA         1
#define CONTAINS_COFFEE      2
#define CONTAINS_MILK1       4
#define CONTAINS_MILK2       8
#define CONTAINS_MILK3      16
#define CONTAINS_SUGAR1     32
#define CONTAINS_SUGAR2     64
#define CONTAINS_SUGAR3    128
#define CONTAINS_WATER1    256
#define CONTAINS_WATER2    512
#define CONTAINS_WATER3   1024
#define CONTAINS_WATER4   2048

typedef struct object {
    char name[NAME_LENGTH];
    int           contents;
    Boolean       mixed;
    Boolean       infused;
    AccessState   at;
    ObjectType    ot;
} Object;

typedef struct object_list {
    Object             *head;
    struct object_list *tail;
} ObjectList;

typedef struct current_state {
    Object     *fixated;
    Object     *held;
    ObjectList *ol;
    Boolean     coffee_instruction;
    Boolean     tea_instruction;
} CurrentState;

static CurrentState cs;

char *task_name[TASK_MAX];

char world_error_string[WES_LENGTH];

double object_empty_mug[18]              = {1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
double object_non_empty_mug1[18]         = {1.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
double object_non_empty_mug2[18]         = {1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
double object_non_empty_mug3[18]         = {1.0, 1.0, 0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
double object_empty_spoon[18]            = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0};
double object_spoon_with_sugar[18]       = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0};
double object_spoon_with_liquid1[18]     = {0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0};
double object_spoon_with_liquid2[18]     = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0};
double object_spoon_with_liquid3[18]     = {0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0};
double object_sugar_bowl_lid[18]         = {0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
double object_sugar_bowl_without_lid[18] = {1.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0};
double object_sugar_bowl_with_lid[18]    = {1.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
double object_cream_carton_open[18]       = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
double object_cream_carton_closed[18]     = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
double object_coffee_pack_torn[18]       = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0};
double object_coffee_pack_untorn[18]     = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0};
double object_sugar_pack_torn[18]        = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0};
double object_sugar_pack_untorn[18]      = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0, 0.0};
double object_teabag[18]                 = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0};

/******************************************************************************/

#if DEBUG
static void show_content(char *when, Object *object)
{
    if (object == NULL) {
        fprintf(stderr, "contents of NULL (%s)\n", when); return;
    }

    fprintf(stderr, "contents of %s (%s): ", object->name, when);
    if (object->contents & CONTAINS_TEA) {
        fprintf(stderr, "tea, ");
    }
    if (object->contents & CONTAINS_COFFEE) {
        fprintf(stderr, "coffee, ");
    }
    if (object->contents & CONTAINS_MILK1) {
        fprintf(stderr, "cream1, ");
    }
    if (object->contents & CONTAINS_MILK2) {
        fprintf(stderr, "cream2, ");
    }
    if (object->contents & CONTAINS_MILK3) {
        fprintf(stderr, "cream3, ");
    }
    if (object->contents & CONTAINS_SUGAR1) {
        fprintf(stderr, "sugar1, ");
    }
    if (object->contents & CONTAINS_SUGAR2) {
        fprintf(stderr, "sugar2, ");
    }
    if (object->contents & CONTAINS_SUGAR3) {
        fprintf(stderr, "sugar3, ");
    }
    if (object->contents & CONTAINS_WATER1) {
        fprintf(stderr, "water1, ");
    }
    if (object->contents & CONTAINS_WATER2) {
        fprintf(stderr, "water2, ");
    }
    if (object->contents & CONTAINS_WATER3) {
        fprintf(stderr, "water3, ");
    }
    if (object->contents & CONTAINS_WATER4) {
        fprintf(stderr, "water4, ");
    }
    fprintf(stderr, "\n");
}
#endif

static Object *create_object(char *name, ObjectType ot, AccessState at, int contents)
{
    ObjectList *new = (ObjectList *)malloc(sizeof(ObjectList));
    Object *node = (Object *)malloc(sizeof(Object));

    if ((new != NULL) && (node != NULL)) {
        strncpy(node->name, name, NAME_LENGTH);
        node->contents = contents;
        node->mixed = FALSE;
        node->infused = FALSE;
        node->ot = ot;
        node->at = at;
        new->head = node;
        new->tail = cs.ol;
        cs.ol = new;
    }
    return(node);
}

void world_initialise(TaskType *task)
{
    Object *mug;

    action_log_initialise();

    cs.ol = NULL;

    /* The mug (initially empty ... actually it contains hot water): */
    mug = create_object("mug", OBJECT_CUP, ACCESS_OPEN, CONTAINS_WATER1 | CONTAINS_WATER2 | CONTAINS_WATER3 | CONTAINS_WATER4);
    if (task->initial_state.mug_contains_coffee) {
        mug->contents = mug->contents | CONTAINS_COFFEE;
    }
    if (task->initial_state.mug_contains_tea) {
        mug->contents = mug->contents | CONTAINS_TEA;
    }
    if (task->initial_state.mug_contains_cream) {
        mug->contents = mug->contents | CONTAINS_MILK1;
    }
    if (task->initial_state.mug_contains_sugar) {
        mug->contents = mug->contents | CONTAINS_SUGAR1;
    }
    /* The spoon (initially empty): */
    create_object("spoon", OBJECT_SPOON, ACCESS_NONE, CONTAINS_NOTHING);
    /* The sugar bowl (initially closed): */
    if (task->initial_state.bowl_closed) {
        create_object("sugar bowl", OBJECT_SUGAR_BOWL, ACCESS_CLOSED, CONTAINS_SUGAR1 | CONTAINS_SUGAR2 | CONTAINS_SUGAR3);
    }
    else {
        create_object("sugar bowl", OBJECT_SUGAR_BOWL, ACCESS_OPEN, CONTAINS_SUGAR1 | CONTAINS_SUGAR2 | CONTAINS_SUGAR3);
    }
    /* The sugar bowl lid: */
    create_object("lid", OBJECT_SUGAR_BOWL_LID, ACCESS_NONE, CONTAINS_NOTHING);
    /* The cream carton (initially closed): */
    create_object("cream carton", OBJECT_CARTON, ACCESS_CLOSED, CONTAINS_MILK1 | CONTAINS_MILK2 | CONTAINS_MILK3);
    /* The coffee packet (initially closed): */
    create_object("coffee packet", OBJECT_COFFEE_PACKET, ACCESS_CLOSED, CONTAINS_COFFEE);
    /* The sugar pack (initially closed): */
    create_object("sugar packet", OBJECT_SUGAR_PACKET, ACCESS_CLOSED, CONTAINS_SUGAR1);
    /* The teabag: */
    create_object("teabag", OBJECT_TEABAG, ACCESS_CLOSED, CONTAINS_TEA);
    /* The instructions (zero in first 18 features): */
    cs.coffee_instruction = (task->base == TASK_COFFEE);
    cs.tea_instruction = (task->base == TASK_TEA);

    cs.held = NULL;
    cs.fixated = mug;

    task_name[TASK_NONE] = "None";
    task_name[TASK_COFFEE] = "Coffee";
    task_name[TASK_TEA] = "Tea";
}

/******************************************************************************/

static Boolean object_contains_water(Object *object)
{
    return(object->contents & (CONTAINS_WATER1 | CONTAINS_WATER2 | CONTAINS_WATER3 | CONTAINS_WATER4));
}

static Boolean object_contains_cream(Object *object)
{
    return(object->contents & (CONTAINS_MILK1 | CONTAINS_MILK2 | CONTAINS_MILK3));
}

static Boolean object_contains_sugar(Object *object)
{
    return(object->contents & (CONTAINS_SUGAR1 | CONTAINS_SUGAR2 | CONTAINS_SUGAR3));
}

static void build_object_vector(double *vector, Object *object)
{
    // Set the first 18 features of vector according to object and its state:
    // Cup
    vector[0] = (object->ot == OBJECT_CUP) || (object->ot == OBJECT_SUGAR_BOWL);
    // One handle
    vector[1] = (object->ot == OBJECT_CUP);
    // Two handles
    vector[2] = (object->ot == OBJECT_SUGAR_BOWL);
    // Lid
    vector[3] = ((object->ot == OBJECT_SUGAR_BOWL) && (object->at == ACCESS_CLOSED)) || (object->ot == OBJECT_SUGAR_BOWL_LID);
    // Clear liquid
    vector[4] = object_contains_water(object) && !object->infused && !(object->contents & CONTAINS_COFFEE) && !object_contains_cream(object);
    // Light (liquid)
    vector[5] = object_contains_water(object) && ((object->contents & CONTAINS_COFFEE) || object->infused) && object_contains_cream(object);
    // Brown liquid
    vector[6] = object_contains_water(object) && ((object->contents & CONTAINS_COFFEE) || object->infused);
    // Carton
    vector[7] = (object->ot == OBJECT_CARTON);
    // Open
    vector[8] = (object->at == ACCESS_OPEN) && (object->ot  == OBJECT_CARTON);
    // Closed
    vector[9] = (object->at == ACCESS_CLOSED) && (object->ot  == OBJECT_CARTON);
    // Packet
    vector[10] = ((object->ot == OBJECT_COFFEE_PACKET) || (object->ot == OBJECT_SUGAR_PACKET));
    // Foil
    vector[11] = (object->ot == OBJECT_COFFEE_PACKET);
    // Paper
    vector[12] = (object->ot == OBJECT_SUGAR_PACKET);
    // Torn
    vector[13] = (((object->ot == OBJECT_COFFEE_PACKET) || (object->ot == OBJECT_SUGAR_PACKET)) && (object->at == ACCESS_OPEN));
    // Untorn
    vector[14] = (((object->ot == OBJECT_COFFEE_PACKET) || (object->ot == OBJECT_SUGAR_PACKET)) && (object->at == ACCESS_CLOSED));
    // Spoon
    vector[15] = (object->ot == OBJECT_SPOON);
    // Teabag
    vector[16] = (object->ot == OBJECT_TEABAG);
    // Sugar
    vector[17] = ((object->ot == OBJECT_SUGAR_BOWL) && (object->at != ACCESS_CLOSED)) || ((object->ot == OBJECT_SPOON) && object_contains_sugar(object));
}

/******************************************************************************/

static void world_set_fixation_vector(double *vector)
{
    // Fixate vector: 18 features from object, plus tea instruction or coffee instruction

    int i;

    if (cs.fixated != NULL) {
        build_object_vector(vector, cs.fixated);
    }
    else {
        for (i = 0; i < 18; i++) {
            vector[i] = 0.0;
        }
    }
    vector[18] = (cs.coffee_instruction ? 1.0 : 0.0);
    vector[19] = (cs.tea_instruction ? 1.0 : 0.0);
}

static void world_set_held_vector(double *vector)
{
    // Held vector: 18 features from object, plus the "nothing" feature

    int i;

    if (cs.held == NULL) {
        for (i = 0; i < 18; i++) {
            vector[i] = 0.0;
        }
        vector[18] = 1.0;
    }
    else {
        // Set the first 18 features of vector:
        build_object_vector(vector, cs.held);
        vector[18] = 0.0;
    }
}

void world_set_network_input_vector(double *vector)
{
    // vector should be 39 units long

    world_set_fixation_vector(vector);    /* First 20 units */
    world_set_held_vector(&vector[20]);   /* Next 19 units  */
}

/******************************************************************************/

static void world_print_object_state(FILE *fp, Object *obj)
{
    if (obj == NULL) {
        print_string(fp, 20, "nothing");
    }
    else {
        double vector[18];

        print_string(fp, 20, obj->name);
        fprintf(fp, ": ");
        build_object_vector(vector, obj);

        if (vector[0] > 0) { fprintf(fp, "cup "); }
        if (vector[1] > 0) { fprintf(fp, "1-handle "); }
        if (vector[2] > 0) { fprintf(fp, "2-handles "); }
        if (vector[3] > 0) { fprintf(fp, "lid "); }
        if (vector[4] > 0) { fprintf(fp, "clear-liquid "); }
        if (vector[5] > 0) { fprintf(fp, "light "); }
        if (vector[6] > 0) { fprintf(fp, "brown-liquid "); }
        if (vector[7] > 0) { fprintf(fp, "carton "); }
        if (vector[8] > 0) { fprintf(fp, "open "); }
        if (vector[9] > 0) { fprintf(fp, "closed "); }
        if (vector[10] > 0) { fprintf(fp, "packet "); }
        if (vector[11] > 0) { fprintf(fp, "foil "); }
        if (vector[12] > 0) { fprintf(fp, "paper "); }
        if (vector[13] > 0) { fprintf(fp, "torn "); }
        if (vector[14] > 0) { fprintf(fp, "untorn "); }
        if (vector[15] > 0) { fprintf(fp, "spoon "); }
        if (vector[16] > 0) { fprintf(fp, "teabag "); }
        if (vector[17] > 0) { fprintf(fp, "sugar "); }
    }
    fprintf(fp, "\n");
}

void world_print_state(FILE *fp)
{
    ObjectList *os;

    fprintf(fp, "FIXATED:\n");
    fprintf(fp, "  ");
    world_print_object_state(fp, cs.fixated);
    fprintf(fp, "HELD:\n");
    fprintf(fp, "  ");
    world_print_object_state(fp, cs.held);
    fprintf(fp, "INSTRUCTIONS:");
    if (cs.coffee_instruction) {
        fprintf(fp, " coffee");
    }
    if (cs.tea_instruction) {
        fprintf(fp, " tea");
    }
    fprintf(fp, "\nSTATE:\n");
    for (os = cs.ol; os != NULL; os = os->tail) {
        fprintf(fp, "  ");
        world_print_object_state(fp, os->head);
    }
    fprintf(fp, "\n");
}

/******************************************************************************/

static Object *locate_object(char *name)
{
    ObjectList *os = cs.ol;
    while (os != NULL) {
        if (strncmp(os->head->name, name, NAME_LENGTH) == 0) {
            return(os->head);
        }
        os = os->tail;
    }
    return(NULL);
}

/*----------------------------------------------------------------------------*/

static void set_cream_content(Object *obj, int i)
{
    if (i > 3) {
        obj->contents = obj->contents | CONTAINS_MILK3;
        i -= 4;
    }
    else if (obj->contents & CONTAINS_MILK3) {
        obj->contents = obj->contents - CONTAINS_MILK3;
    }
    if (i > 1) {
        obj->contents = obj->contents | CONTAINS_MILK2;
        i -= 2;
    }
    else if (obj->contents & CONTAINS_MILK2) {
        obj->contents = obj->contents - CONTAINS_MILK2;
    }
    if (i > 0) {
        obj->contents = obj->contents | CONTAINS_MILK1;
        i -= 1;
    }
    else if (obj->contents & CONTAINS_MILK1) {
        obj->contents = obj->contents - CONTAINS_MILK1;
    }
}

static void set_sugar_content(Object *obj, int i)
{
    if (i > 3) {
        obj->contents = obj->contents | CONTAINS_SUGAR3;
        i -= 4;
    }
    else if (obj->contents & CONTAINS_SUGAR3) {
        obj->contents = obj->contents - CONTAINS_SUGAR3;
    }
    if (i > 1) {
        obj->contents = obj->contents | CONTAINS_SUGAR2;
        i -= 2;
    }
    else if (obj->contents & CONTAINS_SUGAR2) {
        obj->contents = obj->contents - CONTAINS_SUGAR2;
    }
    if (i > 0) {
        obj->contents = obj->contents | CONTAINS_SUGAR1;
        i -= 1;
    }
    else if (obj->contents & CONTAINS_SUGAR1) {
        obj->contents = obj->contents - CONTAINS_SUGAR1;
    }
}

static void set_water_content(Object *obj, int i)
{
    if (i > 7) {
        obj->contents = obj->contents | CONTAINS_WATER4;
        i -= 8;
    }
    else if (obj->contents & CONTAINS_WATER4) {
        obj->contents = obj->contents - CONTAINS_WATER4;
    }
    if (i > 3) {
        obj->contents = obj->contents | CONTAINS_WATER3;
        i -= 4;
    }
    else if (obj->contents & CONTAINS_WATER3) {
        obj->contents = obj->contents - CONTAINS_WATER3;
    }
    if (i > 1) {
        obj->contents = obj->contents | CONTAINS_WATER2;
        i -= 2;
    }
    else if (obj->contents & CONTAINS_WATER2) {
        obj->contents = obj->contents - CONTAINS_WATER2;
    }
    if (i > 0) {
        obj->contents = obj->contents | CONTAINS_WATER1;
        i -= 1;
    }
    else if (obj->contents & CONTAINS_WATER1) {
        obj->contents = obj->contents - CONTAINS_WATER1;
    }
}

static int get_cream_content(Object *obj)
{
    int tmp = 0;
    if (obj->contents & CONTAINS_MILK3) {
        tmp += 4;
    }
    if (obj->contents & CONTAINS_MILK2) {
        tmp += 2;
    }
    if (obj->contents & CONTAINS_MILK1) {
        tmp += 1;
    }
    return(tmp);
}

static int get_sugar_content(Object *obj)
{
    int tmp = 0;
    if (obj->contents & CONTAINS_SUGAR3) {
        tmp += 4;
    }
    if (obj->contents & CONTAINS_SUGAR2) {
        tmp += 2;
    }
    if (obj->contents & CONTAINS_SUGAR1) {
        tmp += 1;
    }
    return(tmp);
}

static int get_water_content(Object *obj)
{
    int tmp = 0;
    if (obj->contents & CONTAINS_WATER4) {
        tmp += 8;
    }
    if (obj->contents & CONTAINS_WATER3) {
        tmp += 4;
    }
    if (obj->contents & CONTAINS_WATER2) {
        tmp += 2;
    }
    if (obj->contents & CONTAINS_WATER1) {
        tmp += 1;
    }
    return(tmp);
}

static void transfer_content_some(Object *source, Object *target)
{
    int tmp;

#if DEBUG
show_content("Source before", source);
show_content("Target before", target);
#endif

    if (get_water_content(source) == 0) {
        if (source->contents & CONTAINS_COFFEE) {
            if (target != NULL) {
                target->contents = target->contents | CONTAINS_COFFEE;
            }
            source->contents = source->contents - CONTAINS_COFFEE;
        }
        if (source->contents & CONTAINS_TEA) {
            if (target != NULL) {
                target->contents = target->contents | CONTAINS_TEA;
            }
            source->contents = source->contents - CONTAINS_TEA;
        }

        if ((tmp = get_sugar_content(source)) > 4) {
            set_sugar_content(source, 3);
            if (target != NULL) {
                set_sugar_content(target, tmp - 3);
            }
        }
        else if (tmp > 0) {
            set_sugar_content(source, 0);
            if (target != NULL) {
                set_sugar_content(target, tmp);
            }
        }

        if ((tmp = get_cream_content(source)) > 4) {
            set_cream_content(source, 3);
            if (target != NULL) {
                set_cream_content(target, tmp - 3);
            }
        }
        else if (tmp > 0) {
            set_cream_content(source, 0);
            if (target != NULL) {
                set_cream_content(target, tmp);
            }
        }
    }

    else if ((tmp = get_water_content(source)) > 8) {
        set_water_content(source, 7);
        if (target != NULL) {
            set_water_content(target, tmp - 7);
        }
    }
    else {
        tmp = get_water_content(source);
        set_water_content(source, 0);
        if (target != NULL) {
            set_water_content(target, tmp);
        }

        if (source->contents & CONTAINS_COFFEE) {
            if (target != NULL) {
                target->contents = target->contents | CONTAINS_COFFEE;
            }
            source->contents = source->contents - CONTAINS_COFFEE;
        }
        if (source->contents & CONTAINS_TEA) {
            if (target != NULL) {
                target->contents = target->contents | CONTAINS_TEA;
            }
            source->contents = source->contents - CONTAINS_TEA;
        }

        if ((tmp = get_sugar_content(source)) > 4) {
            set_sugar_content(source, 3);
            if (target != NULL) {
                set_sugar_content(target, tmp - 3);
            }
        }
        else if (tmp > 0) {
            set_sugar_content(source, 0);
            if (target != NULL) {
                set_sugar_content(target, tmp);
            }
        }

        if ((tmp = get_cream_content(source)) > 4) {
            set_cream_content(source, 3);
            if (target != NULL) {
                set_cream_content(target, tmp - 3);
            }
        }
        else if (tmp > 0) {
            set_cream_content(source, 0);
            if (target != NULL) {
                set_cream_content(target, tmp);
            }
        }

    }
#if DEBUG
show_content("Source after", source);
show_content("Target after", target);
#endif
}

static void transfer_content_all(Object *source, Object *target)
{
    while (source->contents != CONTAINS_NOTHING) {
        transfer_content_some(source, target);
    }
}

static void sip_content(Object *source)
{
#if DEBUG
show_content("Source before sipping", source);
#endif
    transfer_content_some(source, NULL);
#if DEBUG
show_content("Source after sipping", source);
#endif
}

/*----------------------------------------------------------------------------*/
/* Return the action corresponding to the maximum output node. If the first   */
/* argument is non-null then also print the output vector and its distance    */
/* from the closest unit vector.                                              */

ActionType world_get_network_output_action(FILE *fp, double *vector)
{
    double activity = 0.0;
    int i = 0, j = 0;

    if (fp != NULL) {
        fprintf(fp, "Output vector:\n  ");
        print_vector(fp, OUT_WIDTH, vector);
        fprintf(fp, "\n");
    }

    for (i = 0; i < OUT_WIDTH; i++) {
        if (vector[i] > activity) {
            activity = vector[i];
            j = i;
        }
    }

    if (fp != NULL) {
        double sse = 0.0;
        for (i = 0; i < OUT_WIDTH; i++) {
            sse += (i == j) ? (1.0 - vector[i]) * (1.0 - vector[i]) : vector[i] * vector[i];
        }
        fprintf(fp, "RMS error: %f\n", sqrt(sse / OUT_WIDTH));
    }

    return((ActionType) j);
}

Boolean world_perform_action(ActionType action)
{
    Boolean error = TRUE;
    int cycle = 0;
    Object *tmp;

    /* Unset the instruction units: */
    cs.coffee_instruction = FALSE;
    cs.tea_instruction = FALSE;

    switch (action) {
        case ACTION_PICK_UP: {
            if (cs.held != NULL) {
                g_snprintf(world_error_string, WES_LENGTH, "pick up failed: already holding %s", cs.held->name);
            }
            else if (cs.fixated == NULL) {
                g_snprintf(world_error_string, WES_LENGTH, "pick up failed: Not fixated on anything");
            }
            else {
                g_snprintf(world_error_string, WES_LENGTH, "pick up %s", cs.fixated->name);
                action_log_record(ACTION_PICK_UP, cycle, cs.fixated->name, NULL);
                cs.held = cs.fixated;
                error = FALSE;
            }
            break;
        }
        case ACTION_PUT_DOWN: {
            if (cs.held == NULL) {
                g_snprintf(world_error_string, WES_LENGTH, "put down failed: not holding anything");
            }
            else {
                g_snprintf(world_error_string, WES_LENGTH, "put down %s", cs.held->name);
                action_log_record(ACTION_PUT_DOWN, cycle, cs.held->name, NULL);
                cs.held = NULL;
                error = FALSE;
            }
            break;
        }
        case ACTION_POUR: {
            if (cs.held == NULL) {
                g_snprintf(world_error_string, WES_LENGTH, "pour failed: not holding anything");
            }
            else if (cs.held->contents == CONTAINS_NOTHING) {
                g_snprintf(world_error_string, WES_LENGTH, "pour failed: source (%s) is empty", cs.held->name);
            }
            else if (cs.held->at == ACCESS_CLOSED) {
                g_snprintf(world_error_string, WES_LENGTH, "pour failed: source (%s) is closed", cs.held->name);
            }
            else if (cs.fixated == NULL) {
                g_snprintf(world_error_string, WES_LENGTH, "pour failed: No target");
            }
            else if (cs.fixated->at == ACCESS_CLOSED) {
                g_snprintf(world_error_string, WES_LENGTH, "pour failed: target (%s) is closed", cs.fixated->name);
            }
            else if ((cs.fixated->ot != OBJECT_CUP) && (cs.fixated->ot != OBJECT_SUGAR_BOWL) && (cs.fixated->ot != OBJECT_CARTON)) {
                g_snprintf(world_error_string, WES_LENGTH, "pour failed: target (%s) is not valid", cs.fixated->name);
            }
            else {
                transfer_content_some(cs.held, cs.fixated);
                g_snprintf(world_error_string, WES_LENGTH, "pour %s into %s", cs.held->name, cs.fixated->name);
                action_log_record(ACTION_POUR, cycle, cs.held->name, cs.fixated->name);
                error = FALSE;
            }
            break;
        }
        case ACTION_PULL_OPEN: {
            /* Peel open the coffee packet */
            if (cs.held == NULL) {
                g_snprintf(world_error_string, WES_LENGTH, "Cannot pull open: Hand is empty");
            }
            else if (cs.held->ot != OBJECT_COFFEE_PACKET) {
                g_snprintf(world_error_string, WES_LENGTH, "pull open failed: target (%s) is not coffee packet", cs.fixated->name);
            }
            else if (cs.held->at != ACCESS_CLOSED) {
                g_snprintf(world_error_string, WES_LENGTH, "pull open failed: target (%s) is not closed", cs.fixated->name);
            }
            else {
                g_snprintf(world_error_string, WES_LENGTH, "pull open %s", cs.held->name);
                cs.held->at = ACCESS_OPEN;
                action_log_record(ACTION_PULL_OPEN, cycle, cs.held->name, NULL);
                error = FALSE;
            }
            break;
        }
        case ACTION_TEAR_OPEN: {
            /* Tear open the sugar packet */
            if (cs.held == NULL) {
                g_snprintf(world_error_string, WES_LENGTH, "Cannot tear open: Hand is empty");
            }
            else if (cs.held->ot != OBJECT_SUGAR_PACKET) {
                g_snprintf(world_error_string, WES_LENGTH, "tear open failed: target (%s) is not sugar packet", cs.fixated->name);
            }
            else if (cs.held->at != ACCESS_CLOSED) {
                g_snprintf(world_error_string, WES_LENGTH, "tear open failed: target (%s) is not closed", cs.fixated->name);
            }
            else {
                g_snprintf(world_error_string, WES_LENGTH, "tear open %s", cs.held->name);
                cs.held->at = ACCESS_OPEN;
                action_log_record(ACTION_TEAR_OPEN, cycle, cs.held->name, NULL);
                error = FALSE;
            }
            break;
        }
        case ACTION_PEEL_OPEN: {
            /* Pull open the cream carton */
            if (cs.held == NULL) {
                g_snprintf(world_error_string, WES_LENGTH, "Cannot peel open: Hand is empty");
            }
            else if (cs.held->ot != OBJECT_CARTON) {
                g_snprintf(world_error_string, WES_LENGTH, "peel open failed: target (%s) is not carton", cs.fixated->name);
            }
            else if (cs.held->at != ACCESS_CLOSED) {
                g_snprintf(world_error_string, WES_LENGTH, "peel open failed: target (%s) is not closed", cs.fixated->name);
            }
            else {
                g_snprintf(world_error_string, WES_LENGTH, "peel open %s", cs.held->name);
                cs.held->at = ACCESS_OPEN;
                action_log_record(ACTION_PEEL_OPEN, cycle, cs.held->name, NULL);
                error = FALSE;
            }
            break;
        }
        case ACTION_PULL_OFF: {
            /* Pull the lid off the sugar bowl */
            if (cs.held != NULL) {
                g_snprintf(world_error_string, WES_LENGTH, "Cannot pull off: Hand is being used");
            }
            else if (cs.fixated->ot != OBJECT_SUGAR_BOWL) {
                g_snprintf(world_error_string, WES_LENGTH, "pull off failed: target (%s) is not sugar bowl", cs.fixated->name);
            }
            else if (cs.fixated->at != ACCESS_CLOSED) {
                g_snprintf(world_error_string, WES_LENGTH, "pull off failed: target (%s) is not closed", cs.fixated->name);
            }
            else if ((tmp = locate_object("lid")) == NULL) {
                g_snprintf(world_error_string, WES_LENGTH, "pull off failed: cannot locate sugar bowl lid");
            }
            else {
                g_snprintf(world_error_string, WES_LENGTH, "pull off %s from %s", tmp->name, cs.fixated->name);
                cs.held = tmp;
                cs.fixated->at = ACCESS_OPEN;
                action_log_record(ACTION_PULL_OFF, cycle, cs.fixated->name, cs.held->name);
                error = FALSE;
            }
            break;
        }
        case ACTION_SCOOP: {
            if (cs.held == NULL) {
                g_snprintf(world_error_string, WES_LENGTH, "scoop failed: not holding anything");
            }
            else if (cs.held->ot != OBJECT_SPOON) {
                g_snprintf(world_error_string, WES_LENGTH, "scoop failed: held object (%s) is not spoon", cs.held->name);
            }
            else if (cs.held->contents != CONTAINS_NOTHING) {
                g_snprintf(world_error_string, WES_LENGTH, "scoop failed: spoon is not empty");
            }
            else if (cs.fixated == NULL) {
                g_snprintf(world_error_string, WES_LENGTH, "scoop failed: No target");
            }
            else if (cs.fixated->at == ACCESS_CLOSED) {
                g_snprintf(world_error_string, WES_LENGTH, "scoop failed: target (%s) is closed", cs.fixated->name);
            }
            else if (cs.fixated->contents == CONTAINS_NOTHING) {
                g_snprintf(world_error_string, WES_LENGTH, "scoop failed: target (%s) is empty", cs.fixated->name);
            }
            else if ((cs.fixated->ot != OBJECT_CUP) && (cs.fixated->ot != OBJECT_SUGAR_BOWL) && (cs.fixated->ot != OBJECT_CARTON)) {
                g_snprintf(world_error_string, WES_LENGTH, "scoop failed: target (%s) is not valid", cs.fixated->name);
            }
            else {
                transfer_content_some(cs.fixated, cs.held);
                g_snprintf(world_error_string, WES_LENGTH, "scoop with %s from %s", cs.held->name, cs.fixated->name);
                action_log_record(ACTION_SCOOP, cycle, cs.fixated->name, cs.held->name);
                error = FALSE;
            }
            break;
        }
        case ACTION_SIP: {
            if (cs.held == NULL) {
                g_snprintf(world_error_string, WES_LENGTH, "sip failed: Hand is empty");
            }
            else if (cs.held->at == ACCESS_CLOSED) {
                g_snprintf(world_error_string, WES_LENGTH, "sip failed: Held object (%s) is closed", cs.held->name);
            }
            else if (cs.held->contents == CONTAINS_NOTHING) {
                g_snprintf(world_error_string, WES_LENGTH, "sip failed: Held object (%s) is empty", cs.held->name);
            }
            else {
                sip_content(cs.held);
                g_snprintf(world_error_string, WES_LENGTH, "sip from %s", cs.held->name);
                action_log_record(ACTION_SIP, cycle, cs.held->name, NULL);
                error = FALSE;
            }
            break;
        }
        case ACTION_STIR: {
            // Dip stir the contents of a container ... hopefully the mug with the spoon
            if (cs.held == NULL) {
                g_snprintf(world_error_string, WES_LENGTH, "stir failed: Hand is empty");
            }
            else if (cs.held->ot != OBJECT_SPOON) {
                g_snprintf(world_error_string, WES_LENGTH, "stir failed: Held object (%s) is not spoon", cs.held->name);
            }
            else if (cs.fixated == NULL) {
                g_snprintf(world_error_string, WES_LENGTH, "stir failed: No fixated target");
            }
            else if (cs.fixated->at != ACCESS_OPEN) {
                g_snprintf(world_error_string, WES_LENGTH, "stir failed: Target (%s) is not open", cs.fixated->name);
            }
            else if (cs.fixated->contents == CONTAINS_NOTHING) {
                g_snprintf(world_error_string, WES_LENGTH, "stir failed: Target (%s) is empty", cs.fixated->name);
            }
            else if ((cs.fixated->ot != OBJECT_CUP) && (cs.fixated->ot != OBJECT_SUGAR_BOWL) && (cs.fixated->ot != OBJECT_CARTON)) {
                g_snprintf(world_error_string, WES_LENGTH, "stir failed: Target (%s) is not sensible", cs.fixated->name);
            }
            else {
                if (cs.held->contents != CONTAINS_NOTHING) {
                    transfer_content_all(cs.held, cs.fixated);
                }
                g_snprintf(world_error_string, WES_LENGTH, "stir %s with %s", cs.fixated->name, cs.held->name);
                cs.fixated->mixed = TRUE;
                action_log_record(ACTION_STIR, cycle, cs.fixated->name, cs.held->name);
                error = FALSE;
            }
            break;
        }
        case ACTION_DIP: {
            // Dip the teabag into a container ... hopefully the mug
            if (cs.held == NULL) {
                g_snprintf(world_error_string, WES_LENGTH, "dip failed: Hand is empty");
            }
            else if (cs.held->ot != OBJECT_TEABAG) {
                g_snprintf(world_error_string, WES_LENGTH, "dip failed: Held object (%s) is not teabag", cs.held->name);
            }
            else if (cs.fixated->at != ACCESS_OPEN) {
                g_snprintf(world_error_string, WES_LENGTH, "dip failed: Target (%s) is not open", cs.fixated->name);
            }
            else if ((cs.fixated->ot != OBJECT_CUP) && (cs.fixated->ot != OBJECT_SUGAR_BOWL) && (cs.fixated->ot != OBJECT_CARTON)) {
                g_snprintf(world_error_string, WES_LENGTH, "dip failed: Target (%s) is not sensible", cs.fixated->name);
            }
            else {
                if (cs.fixated->contents & (CONTAINS_WATER1 | CONTAINS_WATER2 | CONTAINS_WATER3 | CONTAINS_WATER4)) {
                    cs.fixated->infused = TRUE;
                }
                g_snprintf(world_error_string, WES_LENGTH, "dip %s into %s", cs.held->name, cs.fixated->name);
                action_log_record(ACTION_DIP, cycle, cs.held->name, cs.fixated->name);
                error = FALSE;
            }
            break;
        }
        case ACTION_SAY_DONE: {
            g_snprintf(world_error_string, WES_LENGTH, "Done!");
            action_log_record(ACTION_SAY_DONE, cycle, NULL, NULL);
            error = FALSE;
            break;
        }
        case ACTION_FIXATE_CUP: {
            if ((tmp = locate_object("mug")) == NULL) {
                g_snprintf(world_error_string, WES_LENGTH, "fixate_cup failed: cannot locate target");
            }
            else {
                g_snprintf(world_error_string, WES_LENGTH, "fixate_cup (target is %s)", tmp->name);
                cs.fixated = tmp;
                error = FALSE;
	    }
            break;
        }
        case ACTION_FIXATE_TEABAG: {
            if ((tmp = locate_object("teabag")) == NULL) {
                g_snprintf(world_error_string, WES_LENGTH, "fixate_teabag failed: cannot locate target");
            }
            else {
                g_snprintf(world_error_string, WES_LENGTH, "fixate_teabag (target is %s)", tmp->name);
                cs.fixated = tmp;
                error = FALSE;
	    }
            break;
        }
        case ACTION_FIXATE_COFFEE_PACKET: {
            if ((tmp = locate_object("coffee packet")) == NULL) {
                g_snprintf(world_error_string, WES_LENGTH, "fixate_coffee_pack failed: cannot locate target");
            }
            else {
                g_snprintf(world_error_string, WES_LENGTH, "fixate_coffee_pack (target is %s)", tmp->name);
                cs.fixated = tmp;
                error = FALSE;
	    }
            break;
        }
        case ACTION_FIXATE_SPOON: {
            if ((tmp = locate_object("spoon")) == NULL) {
                g_snprintf(world_error_string, WES_LENGTH, "fixate_spoon failed: cannot locate target");
            }
            else {
                g_snprintf(world_error_string, WES_LENGTH, "fixate_spoon (target is %s)", tmp->name);
                cs.fixated = tmp;
                error = FALSE;
	    }
            break;
        }
        case ACTION_FIXATE_CARTON: {
            if ((tmp = locate_object("cream carton")) == NULL) {
                g_snprintf(world_error_string, WES_LENGTH, "fixate_cream_carton failed: cannot locate target");
            }
            else {
                g_snprintf(world_error_string, WES_LENGTH, "fixate_cream_carton (target is %s)", tmp->name);
                cs.fixated = tmp;
                error = FALSE;
	    }
            break;
        }
        case ACTION_FIXATE_SUGAR_PACKET: {
#ifdef SUGAR_HACK
            tmp = locate_object((random_uniform(0.0, 1.0) > 0.5) ? "sugar packet" : "sugar bowl");
            if (tmp == NULL) {
                g_snprintf(world_error_string, WES_LENGTH, "fixate_sugar failed: cannot locate target");
            }
            else {
                g_snprintf(world_error_string, WES_LENGTH, "fixate_sugar (target is %s)", tmp->name);
                cs.fixated = tmp;
                error = FALSE;
            }
#else
            if ((tmp = locate_object("sugar packet")) == NULL) {
                g_snprintf(world_error_string, WES_LENGTH, "fixate_sugar_packet failed: cannot locate target");
            }
            else {
                g_snprintf(world_error_string, WES_LENGTH, "fixate_sugar_packet (target is %s)", tmp->name);
                cs.fixated = tmp;
                error = FALSE;
            }
#endif
            break;
        }
        case ACTION_FIXATE_SUGAR_BOWL: {
            if ((tmp = locate_object("sugar bowl")) == NULL) {
                g_snprintf(world_error_string, WES_LENGTH, "fixate_sugar_bowl failed: cannot locate target");
            }
            else {
                g_snprintf(world_error_string, WES_LENGTH, "fixate_sugar_bowl (target is %s)", tmp->name);
                cs.fixated = tmp;
                error = FALSE;
            }
            break;
        }
        default: {
            g_snprintf(world_error_string, WES_LENGTH, "Ignoring unrecognised action");
        }
    }
    return(!error);
}

/******************************************************************************/

Boolean world_decode_action(char *buffer, int l, ActionType action)
{
    Boolean ok = TRUE;

    switch (action) {
        case ACTION_PICK_UP: {
            g_snprintf(buffer, l, "Pick up");
            break;
        }
        case ACTION_PUT_DOWN: {
            g_snprintf(buffer, l, "Put down");
            break;
        }
        case ACTION_POUR: {
            g_snprintf(buffer, l, "Pour");
            break;
        }
        case ACTION_PEEL_OPEN: {
            g_snprintf(buffer, l, "Peel open");
            break;
        }
        case ACTION_TEAR_OPEN: {
            g_snprintf(buffer, l, "Tear open");
            break;
        }
        case ACTION_PULL_OPEN: {
            g_snprintf(buffer, l, "Pull open");
            break;
        }
        case ACTION_PULL_OFF: {
            g_snprintf(buffer, l, "Pull off");
            break;
        }
        case ACTION_SCOOP: {
            g_snprintf(buffer, l, "Scoop");
            break;
        }
        case ACTION_SIP: {
            g_snprintf(buffer, l, "Sip");
            break;
        }
        case ACTION_STIR: {
            g_snprintf(buffer, l, "Stir");
            break;
        }
        case ACTION_DIP: {
            g_snprintf(buffer, l, "Dip");
            break;
        }
        case ACTION_SAY_DONE: {
            g_snprintf(buffer, l, "Say done");
            break;
        }
        case ACTION_FIXATE_CUP: {
            g_snprintf(buffer, l, "Fixate cup");
            break;
        }
        case ACTION_FIXATE_TEABAG: {
            g_snprintf(buffer, l, "Fixate teabag");
            break;
        }
        case ACTION_FIXATE_COFFEE_PACKET: {
            g_snprintf(buffer, l, "Fixate coffee packet");
            break;
        }
        case ACTION_FIXATE_SPOON: {
            g_snprintf(buffer, l, "Fixate spoon");
            break;
        }
        case ACTION_FIXATE_CARTON: {
            g_snprintf(buffer, l, "Fixate carton");
            break;
        }
        case ACTION_FIXATE_SUGAR_PACKET: {
#ifdef SUGAR_HACK
            g_snprintf(buffer, l, "Fixate sugar");
#else
            g_snprintf(buffer, l, "Fixate sugar packet");
#endif
            break;
        }
        case ACTION_FIXATE_SUGAR_BOWL: {
            g_snprintf(buffer, l, "Fixate sugar bowl");
            break;
        }
        default: {
            ok = FALSE;
            break;
        }
    }
    return(ok);
}

Boolean world_decode_output_vector(char *buffer, int l, double *out_vector)
{
    return(world_decode_action(buffer, l, world_get_network_output_action(NULL, out_vector)));
}

static double euclidean_distance(int n, double *a, double *b)
{
    double sum = 0;
    while (n-- > 0) {
        sum = sum + (a[n] - b[n]) * (a[n] - b[n]);
    }
    return(sqrt(sum));
}

Boolean world_decode_viewed(char *buffer, int l, double *in_vector)
{
    Boolean ok = TRUE;    

    if (euclidean_distance(18, in_vector, object_empty_mug) < 0.1) {
        g_snprintf(buffer, l, "Mug (empty)");
    }
    else if (euclidean_distance(18, in_vector, object_non_empty_mug1) < 0.1) {
        g_snprintf(buffer, l, "Mug (clear liquid)");
    }
    else if (euclidean_distance(18, in_vector, object_non_empty_mug2) < 0.1) {
        g_snprintf(buffer, l, "Mug (brown liquid)");
    }
    else if (euclidean_distance(18, in_vector, object_non_empty_mug3) < 0.1) {
        g_snprintf(buffer, l, "Mug (light brown liquid)");
    }
    else if (euclidean_distance(18, in_vector, object_empty_spoon) < 0.1) {
        g_snprintf(buffer, l, "Spoon (empty)");
    }
    else if (euclidean_distance(18, in_vector, object_spoon_with_sugar) < 0.1) {
        g_snprintf(buffer, l, "Spoon (sugar)");
    }
    else if (euclidean_distance(18, in_vector, object_spoon_with_liquid1) < 0.1) {
        g_snprintf(buffer, l, "Spoon (clear liquid)");
    }
    else if (euclidean_distance(18, in_vector, object_spoon_with_liquid2) < 0.1) {
        g_snprintf(buffer, l, "Spoon (brown liquid)");
    }
    else if (euclidean_distance(18, in_vector, object_spoon_with_liquid3) < 0.1) {
        g_snprintf(buffer, l, "Spoon (light brown liquid)");
    }
    else if (euclidean_distance(18, in_vector, object_sugar_bowl_lid) < 0.1) {
        g_snprintf(buffer, l, "Sugar bowl lid");
    }
    else if (euclidean_distance(18, in_vector, object_sugar_bowl_without_lid) < 0.1) {
        g_snprintf(buffer, l, "Sugar bowl (without lid)");
    }
    else if (euclidean_distance(18, in_vector, object_sugar_bowl_with_lid) < 0.1) {
        g_snprintf(buffer, l, "Sugar bowl (with lid)");
    }
    else if (euclidean_distance(18, in_vector, object_cream_carton_open) < 0.1) {
        g_snprintf(buffer, l, "Milk carton (open)");
    }
    else if (euclidean_distance(18, in_vector, object_cream_carton_closed) < 0.1) {
        g_snprintf(buffer, l, "Milk carton (closed)");
    }
    else if (euclidean_distance(18, in_vector, object_coffee_pack_torn) < 0.1) {
        g_snprintf(buffer, l, "Coffee pack (torn)");
    }
    else if (euclidean_distance(18, in_vector, object_coffee_pack_untorn) < 0.1) {
        g_snprintf(buffer, l, "Coffee pack (untorn)");
    }
    else if (euclidean_distance(18, in_vector, object_sugar_pack_torn) < 0.1) {
        g_snprintf(buffer, l, "Sugar pack (torn)");
    }
    else if (euclidean_distance(18, in_vector, object_sugar_pack_untorn) < 0.1) {
        g_snprintf(buffer, l, "Sugar pack (untorn)");
    }
    else if (euclidean_distance(18, in_vector, object_teabag) < 0.1) {
        g_snprintf(buffer, l, "Teabag");
    }
    else {
        g_snprintf(buffer, l, "Unkown"); ok = FALSE;
    }
    return(ok);
}

Boolean world_decode_held(char *buffer, int l, double *in_vector)
{
    /* Units 21 to 38 */

    if (in_vector[38] > 0.99) {
        g_snprintf(buffer, l, "Nothing"); return(TRUE);
    }
    else {
        return(world_decode_viewed(buffer, l, &in_vector[20]));
    }
}

/******************************************************************************/
