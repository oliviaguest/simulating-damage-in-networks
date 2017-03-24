/******************************************************************************

Contents:
    Record the actions in a list and analyse that list
Created:
    Richard Cooper, Sat Aug 15 15:54:44 1998
Public procedures:
    void action_log_initialise();
    void action_log_record(ActionType act, int cycle, char *arg1, char *arg2)
    void action_log_print();
    void ActAnalyseList(FILE *fp)

*******************************************************************************/

#include "bp.h"
#include <glib.h>
#include <string.h>
#include "lib_string.h"

#undef LOG_ERRORS

typedef enum a3_label {ANOMOLOUS_A3, PREPARE_COFFEE, PREPARE_TEA} A3Label;
typedef enum a2_label {ANOMOLOUS_A2, ADD_COFFEE, STEEP_TEA, ADD_SUGAR, ADD_MILK, STIR_DRINK, DRINK_BEVERAGE, A2_BREAK, A2_DOUBLE_BREAK} A2Label;

typedef struct action_list {
    ActionType          act;
    int                 cycle;
    char               *arg1;
    char               *arg2;
    A2Label             subtask;
    A3Label             task;
    Boolean             a2_open;
    Boolean             a2_close;
    Boolean             a3_open;
    Boolean             a3_close;
    Boolean             crux;
    Boolean             independent;
    Boolean             correct;
    struct action_list *next;
    struct action_list *prev;
} ActionList;

static ActionList *first_action = NULL;
static ActionList *last_action  = NULL;

extern void action_log_report(char *string);

static Boolean task_complete;

/******************************************************************************/
/********* Constructing the action/event list: ********************************/

void action_log_initialise()
{
    while (first_action != NULL) {
        ActionList *tmp = first_action;
        first_action = first_action->next;
        free(tmp);
    }
    last_action = NULL;
}

void action_log_record(ActionType act, int cycle, char *arg1, char *arg2)
{
    ActionList *new;

    if ((new = (ActionList *)malloc(sizeof(ActionList))) == NULL) {
        fprintf(stderr, "WARNING: Memory allocation failed in action_log_record()\n");
    }
    else {
        new->act = act;
        new->cycle = cycle;
        new->arg1 = arg1;
        new->arg2 = arg2;
        new->subtask = ANOMOLOUS_A2;
        new->task = ANOMOLOUS_A3;
        new->a2_open = FALSE;
        new->a2_close = FALSE;
        new->a3_open = FALSE;
        new->a3_close = FALSE;
        new->crux = FALSE;
        new->independent = FALSE;
        new->correct = FALSE;
        new->next = NULL;
        new->prev = last_action;
    }
    if (last_action == NULL) {
        first_action = new;
        last_action = new;
    }
    else {
        last_action->next = new;
        last_action = new;
    }
}

/******************************************************************************/
/********* Printing the action/event list: ************************************/

static char acs_code(ActionList *action)
{
    switch (action->subtask) {
        case ANOMOLOUS_A2: {
            return(action->crux?'X':'x');
        }
        case ADD_COFFEE: {
            return(action->crux?'G':'g');
        }
        case STEEP_TEA: {
            return(action->crux?'T':'t');
        }
        case ADD_SUGAR: {
            return(action->crux?'S':'s');
        }
        case ADD_MILK: {
            return(action->crux?'M':'m');
        }
        case STIR_DRINK: {
            return(action->crux?'R':'r');
        }
        case DRINK_BEVERAGE: {
            return(action->crux?'D':'d');
        }
        case A2_BREAK: {
            return('/');
	}
        case A2_DOUBLE_BREAK: {
            return('"');
        }
        default: {
            return('?');
        }
    }
}

static void print_action(ActionList *action)
{
    char buffer[128];
    char buffer2[128];

    if (action->a2_open) { action_log_report("("); }

    switch (action->act) {
        case ACTION_PICK_UP: {
            g_snprintf(buffer, 128, "pick_up(%s)", action->arg1);
            break;
        }
        case ACTION_PUT_DOWN: {
            g_snprintf(buffer, 128, "put_down(%s)", action->arg1);
            break;
        }
        case ACTION_POUR: {
            g_snprintf(buffer, 128, "pour(%s, %s)", action->arg1, action->arg2);
            break;
        }
        case ACTION_PEEL_OPEN: {
            g_snprintf(buffer, 128, "peel_open(%s)", action->arg1);
            break;
        }
        case ACTION_TEAR_OPEN: {
            g_snprintf(buffer, 128, "tear_open(%s)", action->arg1);
            break;
        }
        case ACTION_PULL_OPEN: {
            g_snprintf(buffer, 128, "pull_open(%s)", action->arg1);
            break;
        }
        case ACTION_PULL_OFF: {
            g_snprintf(buffer, 128, "pull_off(%s, %s)", action->arg1, action->arg2);
            break;
        }
        case ACTION_SCOOP: {
            g_snprintf(buffer, 128, "scoop(%s, %s)", action->arg1, action->arg2);
            break;
        }
        case ACTION_SIP: {
            g_snprintf(buffer, 128, "sip(%s)", action->arg1);
            break;
        }
        case ACTION_STIR: {
            g_snprintf(buffer, 128, "stir(%s, %s)", action->arg1, action->arg2);
            break;
        }
        case ACTION_DIP: {
            g_snprintf(buffer, 128, "dip(%s, %s)", action->arg1, action->arg2);
            break;
        }
        case ACTION_SAY_DONE: {
            g_snprintf(buffer, 128, "done");
            break;
        }
        default: {
            g_snprintf(buffer, 128, "default(%d, %p, %p)", (int) action->act, action->arg1, action->arg2);
            break;
        }
    }

    g_snprintf(buffer2, 128, "%c%c %4d: %c %s", (action->correct?' ':'*'), (action->independent?'I':' '), action->cycle, acs_code(action), buffer);

    action_log_report(buffer2);

    if (action->a2_close) { action_log_report(")"); }
}

void action_log_print()
{
    ActionList *tmp;

    for (tmp = first_action; tmp != NULL; tmp = tmp->next) {
        print_action(tmp);
    }
}

/*----------------------------------------------------------------------------*/

static void print_action_to_file(FILE *fp, ActionList *action)
{
    switch (action->act) {
        case ACTION_PICK_UP: {
            fprintf(fp, "pick_up(%s)\n", action->arg1);
            break;
        }
        case ACTION_PUT_DOWN: {
            fprintf(fp, "put_down(%s)\n", action->arg1);
            break;
        }
        case ACTION_POUR: {
            fprintf(fp, "pour(%s, %s)\n", action->arg1, action->arg2);
            break;
        }
        case ACTION_PEEL_OPEN: {
            fprintf(fp, "peel_open(%s)\n", action->arg1);
            break;
        }
        case ACTION_TEAR_OPEN: {
            fprintf(fp, "tear_open(%s)\n", action->arg1);
            break;
        }
        case ACTION_PULL_OPEN: {
            fprintf(fp, "pull_open(%s)\n", action->arg1);
            break;
        }
        case ACTION_PULL_OFF: {
            fprintf(fp, "pull_off(%s, %s)\n", action->arg1, action->arg2);
            break;
        }
        case ACTION_SCOOP: {
            fprintf(fp, "scoop(%s, %s)\n", action->arg1, action->arg2);
            break;
        }
        case ACTION_SIP: {
            fprintf(fp, "sip(%s)\n", action->arg1);
            break;
        }
        case ACTION_STIR: {
            fprintf(fp, "stir(%s, %s)\n", action->arg1, action->arg2);
            break;
        }
        case ACTION_DIP: {
            fprintf(fp, "dip(%s, %s)\n", action->arg1, action->arg2);
            break;
        }
        case ACTION_SAY_DONE: {
            fprintf(fp, "done\n");
            break;
        }
        default: {
            fprintf(fp, "default(%d, %p, %p)\n", (int) action->act, action->arg1, action->arg2);
            break;
        }
    }
}

void print_actions_to_file(FILE *fp)
{
    ActionList *tmp;

    fprintf(fp, "     ----------------------------------------------------------------------\n");
    for (tmp = first_action; tmp != NULL; tmp = tmp->next) {
        print_action_to_file(fp, tmp);
    }
}

/******************************************************************************/
/********* Analysing the action/event list: ***********************************/

static Boolean is_sugar(char *string)
{
    return((string != NULL) && strcmp(string, "sugar") == 0);
}

static Boolean is_coffee(char *string)
{
    return((string != NULL) && strcmp(string, "coffee") == 0);
}

static Boolean is_cream(char *string)
{
    return((string != NULL) && strcmp(string, "cream") == 0);
}

static Boolean is_teabag(char *string)
{
    return((string != NULL) && strcmp(string, "teabag") == 0);
}

static Boolean is_cream_carton(char *string)
{
    return((string != NULL) && strcmp(string, "cream carton") == 0);
}

static Boolean is_sugar_bowl(char *string)
{
    return((string != NULL) && strcmp(string, "sugar bowl") == 0);
}

static Boolean is_sugar_packet(char *string)
{
    return((string != NULL) && strcmp(string, "sugar packet") == 0);
}

static Boolean is_coffee_packet(char *string)
{
    return((string != NULL) && strcmp(string, "coffee packet") == 0);
}

static Boolean is_spoon(char *string)
{
    return((string != NULL) && strcmp(string, "spoon") == 0);
}

static Boolean is_mug(char *string)
{
    return((string != NULL) && strcmp(string, "mug") == 0);
}

static Boolean is_lid(char *string)
{
    return((string != NULL) && strcmp(string, "lid") == 0);
}

static A2Label guess_subtask_from_object(char *string)
{
    if (string == NULL) {
        return(ANOMOLOUS_A2);
    }
    else if (strcmp(string, "sugar") == 0) {
        return(ADD_SUGAR);
    }
    else if (strcmp(string, "sugar bowl") == 0) {
        return(ADD_SUGAR);
    }
    else if (strcmp(string, "sugar packet") == 0) {
        return(ADD_SUGAR);
    }
    else if (strcmp(string, "coffee") == 0) {
        return(ADD_COFFEE);
    }
    else if (strcmp(string, "coffee packet") == 0) {
        return(ADD_COFFEE);
    }
    else if (strcmp(string, "teabag") == 0) {
        return(STEEP_TEA);
    }
    else if (strcmp(string, "cream") == 0) {
        return(ADD_MILK);
    }
    else if (strcmp(string, "cream carton") == 0) {
        return(ADD_MILK);
    }
    else {
        return(ANOMOLOUS_A2);
    }
}

static Boolean is_error(ActionList *action)
{
    switch (action->act) {
        case ACTION_PICK_UP: case ACTION_PUT_DOWN: {
            return((action->subtask != ADD_MILK) && (action->subtask != ADD_SUGAR) && (action->subtask != ADD_COFFEE) && (action->subtask != STEEP_TEA) && (action->subtask != STIR_DRINK) && (action->subtask != DRINK_BEVERAGE));
        }
        case ACTION_POUR: {
            return((strcmp(action->arg2, "mug") != 0) || ((action->subtask != ADD_MILK) && (action->subtask != ADD_SUGAR) && (action->subtask != ADD_COFFEE)));
        }
        case ACTION_STIR: {
            return((action->subtask != STIR_DRINK) && ((strcmp(action->arg2, "mug") != 0) || (strcmp(action->arg1, "spoon") != 0)));
        }
        case ACTION_DIP: {
            return(action->subtask != STEEP_TEA);
        }
        case ACTION_TEAR_OPEN: case ACTION_PULL_OFF: case ACTION_SCOOP: {
            return(action->subtask != ADD_SUGAR);
        }
        case ACTION_PULL_OPEN: {
            return(action->subtask != ADD_COFFEE);
        }
        case ACTION_PEEL_OPEN: {
            return(action->subtask != ADD_MILK);
        }
        case ACTION_SIP: {
            return(action->subtask != DRINK_BEVERAGE);
        }
        default: {
            return(TRUE);
        }
    }
}

static char *get_spoon_contents(ActionList *action)
{
    while (action->prev != NULL) {
        action = action->prev;
        if (action->act == ACTION_SCOOP) {
            if (is_spoon(action->arg2)) {
                return(action->arg1);
            }
        }
        else if (action->act == ACTION_POUR) {
            if (is_spoon(action->arg1)) {
                return(NULL);
            }
        }
    }
    return(NULL);
}

static A2Label get_subtask(ActionList *action)
{
    switch (action->act) {
        case ACTION_PICK_UP: {
            if (is_spoon(action->arg1) && (action->next != NULL)) {
                if ((action->next->act == ACTION_PUT_DOWN) && is_spoon(action->next->arg1)) {
                    return(A2_BREAK);
                }
                else {
                    return(get_subtask(action->next));
                }
            }
            else if (is_mug(action->arg1) && (action->next != NULL)) {
                if (action->next->act == ACTION_SIP) {
                    return(DRINK_BEVERAGE);
                }
                else if (action->next->act == ACTION_POUR) {
                    return(get_subtask(action->next));
                }
                else {
                    return(guess_subtask_from_object(action->arg1));
                }
            }
            else {
                return(guess_subtask_from_object(action->arg1));
            }
        }
        case ACTION_PUT_DOWN: {
            if (is_spoon(action->arg1) && (action->prev != NULL)) {
                if ((action->prev->act == ACTION_PUT_DOWN) && is_spoon(action->prev->arg1)) {
                    return(A2_BREAK); 
                }
                else {
                    return(get_subtask(action->prev));
                }
            }
            else if (is_lid(action->arg1)) {
                if ((action->prev != NULL) && (action->prev->act == ACTION_PULL_OFF)) {
                    return(action->prev->subtask);
                }
                else if (action->next != NULL) {
                    return(get_subtask(action->next));
                }
                else {
                    return(ADD_SUGAR);
                }
            }
            else {
                return(guess_subtask_from_object(action->arg1));
            }
        }
        case ACTION_TEAR_OPEN: case ACTION_PEEL_OPEN: case ACTION_PULL_OPEN: case ACTION_PULL_OFF: {
            return(guess_subtask_from_object(action->arg1));
        }
        case ACTION_SCOOP: {
            return(guess_subtask_from_object(action->arg1));
        }
        case ACTION_STIR: {
            return(STIR_DRINK);
        }
        case ACTION_DIP: {
            return(guess_subtask_from_object(action->arg1));
        }
        case ACTION_POUR: {
            if (is_spoon(action->arg1)) {
                char *contents = get_spoon_contents(action);
                return(guess_subtask_from_object(contents));
            }
            else {
                return(guess_subtask_from_object(action->arg1));
            }
        }
        case ACTION_SIP: {
            return(DRINK_BEVERAGE);
        }
        default: {
            return(A2_BREAK);
        }
    }
}

Boolean is_crux(ActionType act)
{
    switch (act) {
        case ACTION_STIR: case ACTION_DIP: case ACTION_POUR: case ACTION_SIP: {
            return(TRUE);
        }
        default: {
            return(FALSE);
        }
    }
}

static int count_actions()
{
    ActionList *tmp;
    int count = 0;

    for (tmp = first_action; tmp != NULL; tmp = tmp->next) {
        count++;
    }
    return(count);
}

static int count_independents()
{
    ActionList *tmp;
    int count = 0;

    for (tmp = first_action; tmp != NULL; tmp = tmp->next) {
        if (tmp->independent) {
            count++;
        }
    }
    return(count);
}

static int count_crux_errors()
{
    ActionList *tmp;
    int count = 0;

    for (tmp = first_action; tmp != NULL; tmp = tmp->next) {
        if ((tmp->crux) && (!tmp->correct)) {
            count++;
        }
    }
    return(count);
}

static int count_non_crux_errors()
{
    ActionList *tmp;
    int count = 0;

    for (tmp = first_action; tmp != NULL; tmp = tmp->next) {
        if ((!tmp->crux) && (!tmp->correct)) {
            count++;
        }
    }
    return(count);
}

/*----------------------------------------------------------------------------*/

static void categorise_a1s()
{
    ActionList *tmp;
    Boolean prev_break;

    /* Initialise and score basic actions: */
    for (tmp = first_action; tmp != NULL; tmp = tmp->next) {
        tmp->a2_open = FALSE;
        tmp->a2_close = FALSE;
        tmp->subtask = get_subtask(tmp);
        tmp->crux = is_crux(tmp->act);
        tmp->independent = FALSE;
        tmp->correct = TRUE;
    }
    /* Ignore the last "say done", if it is present: */
    if ((last_action != NULL) && (last_action->act == ACTION_SAY_DONE)) {
        if (first_action == last_action) {
            free(last_action);
            first_action = NULL;
            last_action = NULL;
        }
        else {
            last_action = last_action->prev;
            free(last_action->next);
            last_action->next = NULL;
        }
        task_complete = TRUE;
    }
    /* Score double breaks: */
    for (tmp = first_action, prev_break = FALSE; tmp != NULL; tmp = tmp->next) {
        if ((prev_break) && (tmp->subtask == A2_BREAK)) {
            tmp->prev->subtask = A2_DOUBLE_BREAK;
            tmp->subtask = A2_DOUBLE_BREAK;
        }
        else {
            prev_break = (tmp->subtask == A2_BREAK);
        }
    }
}

static void extend_bracketing_minimally(ActionList *action)
{
    ActionList *tmp;

    for (tmp = action->prev; ((tmp != NULL) && (tmp->subtask == action->subtask)); tmp = tmp->prev) {
        tmp->next->a2_open = FALSE;
        tmp->a2_open = TRUE;
    }

    for (tmp = action->next; ((tmp != NULL) && (tmp->subtask == action->subtask)); tmp = tmp->next) {
        tmp->prev->a2_close = FALSE;
        tmp->a2_close = TRUE;
    }
}

static void extend_bracketing_to_prior(ActionList *action)
{
    /* Look to the left. Is there another action from the current subtask */
    /* before the beginning of the previous fully bracketed subtask?      */

    ActionList *tmp = action;
    Boolean crosses_a2 = FALSE;

    while ((tmp != NULL) && (tmp->subtask == action->subtask)) {
        tmp = tmp->prev;
    }

    while ((tmp != NULL) && (tmp->subtask != action->subtask)) {

        /* DON'T BRACKET OVER A DOUBLE A2_BREAK */

        if (tmp->subtask == A2_DOUBLE_BREAK) {
            return;
        }

        /* BE SURE WE DON'T CROSS ANOTHER COMPLETE A2 */
        /* IF WE DO, THEN TERMINATE THE BRACKETING    */ 

        if (!crosses_a2 && tmp->a2_close) {
            crosses_a2 = TRUE;
        }
        else if (crosses_a2 && tmp->a2_open) {
            return;
        }

        tmp = tmp->prev;
    }

    while ((tmp != NULL) && (tmp->subtask == action->subtask)) {
        tmp = tmp->prev;
    }

    if ((tmp == NULL) && (first_action->subtask == action->subtask)) {

    /* The open bracket should be moved to the start of the sequence */

        action->a2_open = FALSE;
        first_action->a2_open = TRUE;
    }
    else if (tmp != NULL) {

    /* We've found a prior action in the same subtask,  */
    /* so shift the open bracket                        */

        action->a2_open = FALSE;
        tmp->next->a2_open = TRUE;
        extend_bracketing_to_prior(tmp);
    }
}

static void extend_bracketing_to_post(ActionList *action)
{
    /* Look to the right. Is there another action from the current subtask */
    /* before the end of the following fully bracketed subtask?            */

    ActionList *tmp = action;
    Boolean crosses_a2 = FALSE;

    while ((tmp != NULL) && (tmp->subtask == action->subtask)) {
        tmp = tmp->next;
    }

    while ((tmp != NULL) && (tmp->subtask != action->subtask)) {

        /* DON'T BRACKET OVER A DOUBLE A2_BREAK */

        if (tmp->subtask == A2_DOUBLE_BREAK) {
            return;
        }

        /* BE SURE WE DON'T CROSS ANOTHER COMPLETE A2 */
        /* IF WE DO, THEN TERMINATE THE BRACKETING    */ 

        if (!crosses_a2 && tmp->a2_open) {
            crosses_a2 = TRUE;
        }
        else if (crosses_a2 && tmp->a2_close) {
            return;
        }

        tmp = tmp->next;
    }

    while ((tmp != NULL) && (tmp->subtask == action->subtask)) {
        tmp = tmp->next;
    }

    if ((tmp == NULL) && (last_action->subtask == action->subtask)) {

    /* The close bracket should be moved to the end of the sequence */

        action->a2_close = FALSE;
        last_action->a2_close = TRUE;
    }
    else if (tmp != NULL) {

    /* We've found a post action in the same subtask,   */
    /* so shift the close bracket                       */

        action->a2_close = FALSE;
        tmp->prev->a2_close = TRUE;
        extend_bracketing_to_post(tmp);
    }
}

static void bracket_a1s()
{
    ActionList *tmp;

    /* First run through the list finding cruxes. Extend them to left/right   */
    /* so that the brackets extend over connected regions of one subtask.     */

    for (tmp = first_action; tmp != NULL; tmp = tmp->next) {
        if ((tmp->crux) && (tmp->subtask != A2_BREAK)) {
            tmp->a2_open = TRUE;
            tmp->a2_close = TRUE;
            extend_bracketing_minimally(tmp);
        }
    }

    for (tmp = first_action; tmp != NULL; tmp = tmp->next) {
        if (tmp->a2_open) {
            extend_bracketing_to_prior(tmp);
        }
        if (tmp->a2_close) {
            extend_bracketing_to_post(tmp);
        }
    }
}

static void categorise_indepedents()
{
    ActionList *tmp;
    int level = 0;

    for (tmp = first_action; tmp != NULL; tmp = tmp->next) {
        if (tmp->a2_open) {
            level++;
        }
        tmp->independent = (level == 0);
        if (tmp->a2_close) {
            level--;
        }
    }
}

static void categorise_errors()
{
    ActionList *tmp;

    for (tmp = first_action; tmp != NULL; tmp = tmp->next) {
        tmp->correct = !is_error(tmp);
    }
}

/*----------------------------------------------------------------------------*/

// NOTE: task_complete is recorded in a very unsatisfactory way! We use a
// global variable to record whether SAY_DONE occurs using ACS1, but for other
// reasons delete SAY_DONE if it does occur at the end of the list. ACS2 then
// checks this global variable. Thus, ACS2 must be called after ACS1 on the same
// actions if this is to be accurate

static void analyse_actions_code_with_acs1()
{
    task_complete = FALSE;
    if (first_action != NULL) {
        categorise_a1s();
        bracket_a1s();
        categorise_indepedents();
        categorise_errors();
    }
}

void analyse_actions_with_acs1(TaskType *task, ACS1 *results)
{
    analyse_actions_code_with_acs1();

    results->actions = count_actions();
    results->independents = count_independents();
    results->errors_crux = count_crux_errors();
    results->errors_non_crux = count_non_crux_errors();
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/* We use a state transition approach, whereby the action list is processed   */
/* from start to finish with errors recorded along the way or at the end.     */
/* This isn't the real ACS2 of Schwartz et al., but it is more accurate.      */

/* Global variables to record what has been done:                             */

static Boolean added_coffee, added_tea, added_cream, added_sugar, stirred_coffee, stirred_sugar, stirred_cream;
static Boolean cream_carton_is_open, coffee_packet_is_open, sugar_bowl_is_open, sugar_packet_is_open;
static int sip_count;
static char *spoon_contents, *lid_contents;

/*----------------------------------------------------------------------------*/

static Boolean action_compare(ActionList *this, ActionType act, char *arg1, char *arg2)
{
    if (this == NULL) {
        return(FALSE);
    }
    else if (this->act != act) {
        return(FALSE);
    }
    else if ((arg1 != NULL) && ((this->arg1 == NULL) || strcmp(this->arg1, arg1) != 0)) {
        return(FALSE);
    }
    else if ((arg2 != NULL) && ((this->arg2 == NULL) || strcmp(this->arg2, arg2) != 0)) {
        return(FALSE);
    }
    else {
        return(TRUE);
    }
}

static Boolean all_ingredients_have_been_stirred_in()
{
    if (added_coffee && !stirred_coffee) {
        return(FALSE);
    }
    else if (added_sugar && !stirred_sugar) {
        return(FALSE);
    }
    else if (added_cream && !stirred_cream) {
        return(FALSE);
    }
    else {
        return(TRUE);
    }
}

static Boolean all_ingredients_added(TaskType *task)
{
    if (task->base == TASK_COFFEE) {
        return(added_coffee && added_sugar && added_cream);
    }
    else if (task->base == TASK_TEA) {
        return(added_tea && added_sugar);
    }
    else {
        return(TRUE);
    }
}

static Boolean ingredients_added_after_current_action(ActionList *action)
{
    while (action != NULL) {
        if (action_compare(action, ACTION_POUR, NULL, "mug")) {
            return(TRUE);
        }
        action = action->next;
    }
    return(FALSE);
}


static Boolean next_steps_add_but_dont_stir_ingredient(ActionList *action)
{
    // Return TRUE if the next step is to add cream/sugar/coffee and that
    // ingredient is not immediately stirred in. As an approximation, return
    // TRUE either if we have exactly one pour(X, mug) between now and the
    // end, or more than one consecutive pour(X, mug) without an intervening
    // stir(mug, Y)

    int pour_count = 0;

    while (action != NULL) {
        if (action_compare(action, ACTION_POUR, NULL, "mug")) {
            pour_count++;
        }
        else if (action_compare(action, ACTION_STIR, "mug", NULL)) {
            return(pour_count > 1);
        }
        action = action->next;
    }
    return(pour_count == 1);
}

static Boolean sugar_bowl_lid_is_poured_into_mug(ActionList *action)
{
    // Return TRUE if the sugar bowl lid is eventually poured into the coffee 

    while (action != NULL) {
        if (action_compare(action, ACTION_POUR, "lid", "mug")) {
            return(TRUE);
        }
        else if (action_compare(action, ACTION_POUR, "lid", NULL)) {
            return(FALSE);
        }
        else {
            action = action->next;
        }
    }
    return(FALSE);
}

/*----------------------------------------------------------------------------*/

static void initialise_state()
{
    added_coffee = FALSE;
    added_tea = FALSE;
    added_cream = FALSE;
    added_sugar = FALSE;
    stirred_coffee = FALSE;
    stirred_sugar = FALSE;
    stirred_cream = FALSE;
    cream_carton_is_open = FALSE;
    coffee_packet_is_open = FALSE;
    sugar_bowl_is_open = FALSE;
    sugar_packet_is_open = FALSE;
    sip_count = 0;
    spoon_contents = NULL;
    lid_contents = NULL;
}

static void initialise_counts(ACS2 *results)
{
    results->object_sub = 0;
    results->action_addition = 0;
    results->anticipations = 0;
    results->perseverations = 0;
    results->reversals = 0;
    results->gesture_sub = 0;
    results->quality = 0;
    results->tool_omission = 0;
    results->omissions = 0;
    results->bizarre = 0;
    results->accomplished = 0;
}

static GList *append_error(GList *errors, char *content)
{
    return(g_list_append(errors, string_copy(content)));
}

static GList *process_all_actions(GList *errors, TaskType *task, ACS2 *results)
{
    A2Label previous_subtask = ANOMOLOUS_A2;
    char buffer[128];
    ActionList *action;

    for (action = first_action; action != NULL; action = action->next) {
        switch (action->act) {
            case ACTION_PICK_UP: {
                break;
            }
            case ACTION_PUT_DOWN: {
                break;
            }
            case ACTION_POUR: {
                if ((action->arg1 == NULL) || (action->arg2 == NULL)) {
                    errors = append_error(errors, "Bizarre: NULL object in pour!");
                    results->bizarre++;
		}
                else if (strcmp(action->arg1, action->arg2) == 0) {
                    errors = append_error(errors, "Bizarre: Pouring into self!");
                    results->bizarre++;
                }
                else if (is_spoon(action->arg2)) {
                    // Pour target is spoon
                    if (is_coffee_packet(action->arg1)) {
                        if (!coffee_packet_is_open) {
                            errors = append_error(errors, "Anticipation: Pouring coffee into spoon without opening packet");
                            results->anticipations++;
                        }
                        else {
                            errors = append_error(errors, "Object substitution: Pouring coffee into spoon");
                            results->object_sub++;
                            spoon_contents = "coffee";
                        }
                    }
                    else if (is_sugar_packet(action->arg1)) {
                        if (!sugar_packet_is_open) {
                            errors = append_error(errors, "Anticipation: Pouring sugar packet into spoon but sugar packet isn't open");
                            results->anticipations++;
                        }
                        else {
                            errors = append_error(errors, "Object substitution: Pouring sugar packet into spoon");
                            results->object_sub++;
                            spoon_contents = "sugar";
                        }
                    }
                    else if (is_sugar_bowl(action->arg1)) {
                        if (!sugar_bowl_is_open) {
                            errors = append_error(errors, "Anticipation: Pouring sugar bowl into spoon but sugar bowl is not open");
                            results->anticipations++;
                        }
                        else {
                            errors = append_error(errors, "Object substitution: Pouring sugar bowl into spoon");
                            results->object_sub++;
                            spoon_contents = "sugar";
                        }
                    }
                    else if (is_lid(action->arg1)) {
                        if (lid_contents == NULL) {
                            errors = append_error(errors, "Anticipation: Pouring lid into spoon but lid is empty");
                            results->anticipations++;
                        }
                        else {
                            errors = append_error(errors, "Action addition: Pouring sugar bowl lid into spoon");
                            results->action_addition++;
                            spoon_contents = lid_contents;
                            lid_contents = NULL;
                        }
                    }
                    else if (is_cream_carton(action->arg1)) {
                        if (!cream_carton_is_open) {
                            errors = append_error(errors, "Anticipation: Pouring cream carton into spoon but cream carton is not open");
                            results->anticipations++;
                        }
                        else {
                            errors = append_error(errors, "Object substitution: Pouring cream carton into spoon");
                            results->object_sub++;
                            spoon_contents = "cream";
                        }
                    }
                    else {
                        g_snprintf(buffer, 128, "Gesture substitution: Pouring with %s (not a container)", action->arg1);
                        errors = append_error(errors, buffer);
                        results->gesture_sub++;
                    }
                }
                else if (is_sugar_bowl(action->arg2)) {
                    // Pour target is sugar bowl
                    if (is_coffee_packet(action->arg1)) {
                        if (!coffee_packet_is_open) {
                            errors = append_error(errors, "Anticipation: Pouring coffee packet into sugar bowl but coffee packet is closed");
                            results->anticipations++;
                        }
                        else {
                            errors = append_error(errors, "Object substitution: Pouring coffee into sugar bowl");
                            results->object_sub++;
                        }
                    }
                    else if (is_sugar_packet(action->arg1)) {
                        if (!sugar_packet_is_open) {
                            errors = append_error(errors, "Anticipation: Pouring sugar packet into sugar bowl but sugar packet is not open");
                            results->anticipations++;
                        }
                        else {
                            errors = append_error(errors, "Object substitution: Pouring sugar packet into sugar bowl");
                            results->object_sub++;
                        }
                    }
                    else if (is_spoon(action->arg1)) {
                        if (spoon_contents == NULL) {
                            errors = append_error(errors, "Bizarre: Pouring empty spoon into sugar bowl");
                            results->bizarre++;
                        }
                        else {
                            errors = append_error(errors, "Object substitution: Pouring full spoon into sugar bowl");
                            results->object_sub++;
                            spoon_contents = NULL;
                        }
                    }
                    else if (is_lid(action->arg1)) {
                        if (lid_contents == NULL) {
                            errors = append_error(errors, "Anticipation: Pouring empty lid into sugar bowl");
                            results->anticipations++;
                        }
                        else {
                            errors = append_error(errors, "Action addition: Pouring sugar bowl lid into sugar bowl");
                            results->action_addition++;
                            lid_contents = NULL;
                        }
                    }
                    else if (is_cream_carton(action->arg1)) {
                        if (!cream_carton_is_open) {
                            errors = append_error(errors, "Anticipation: Pouring cream carton into sugar bowl but cream carton is not open");
                            results->anticipations++;
                        }
                        else {
                            errors = append_error(errors, "Object substitution: Pouring cream carton into sugar bowl");
                            results->object_sub++;
                        }
                    }
                    else {
                        g_snprintf(buffer, 128, "Gesture substitution: Pouring with %s (not a container)", action->arg1);
                        errors = append_error(errors, buffer);
                        results->gesture_sub++;
                    }
                }
                else if (is_lid(action->arg2)) {
                    // Pour target is sugar bowl lid
                    if (is_coffee_packet(action->arg1)) {
                        if (!coffee_packet_is_open) {
                            errors = append_error(errors, "Anticipation: Pouring from closed coffee packet");
                            results->anticipations++;
                        }
                        else {
                            errors = append_error(errors, "Object substitution: Pouring coffee into sugar bowl lid");
                            results->object_sub++;
                            spoon_contents = "coffee";
                        }
                    }
                    else if (is_sugar_packet(action->arg1)) {
                        if (!sugar_packet_is_open) {
                            errors = append_error(errors, "Anticipation: Pouring from closed sugar packet");
                            results->anticipations++;
                        }
                        else {
                            errors = append_error(errors, "Object substitution: Pouring sugar packet into sugar bowl lid");
                            results->object_sub++;
                            spoon_contents = "sugar";
                        }
                    }
                    else if (is_spoon(action->arg1)) {
                        errors = append_error(errors, "Action addition: Pouring spoon into sugar bowl lid");
                        results->action_addition++;
                        lid_contents = spoon_contents;
                        spoon_contents = NULL;
                    }
                    else if (is_sugar_bowl(action->arg1)) {
                        if (!sugar_bowl_is_open) {
                            errors = append_error(errors, "Anticipation: Pouring unopened bowl into sugar bowl lid");
                            results->anticipations++;
                        }
                        else {
                            errors = append_error(errors, "Object substitution: Pouring sugar bowl into sugar bowl lid");
                            results->object_sub++;
                            lid_contents = "sugar";
                        }
                    }
                    else if (is_cream_carton(action->arg1)) {
                        if (!cream_carton_is_open) {
                            errors = append_error(errors, "Anticipation: Pouring from closed cream carton");
                            results->anticipations++;
                        }
                        else {
                            errors = append_error(errors, "Object substitution: Pouring cream carton into sugar bowl lid");
                            results->object_sub++;
                        }
                    }
                    else {
                        errors = append_error(errors, "Gesture substitution: Pouring with non-container");
                        results->gesture_sub++;
                    }
                }
                else if (!is_mug(action->arg2)) {
                    // Pour target is not mug
                    g_snprintf(buffer, 128, "Object substitution: Pouring into %s (not a container)", action->arg2);
                    errors = append_error(errors, buffer);
                    results->object_sub++;
                }
                // Pour target is mug
                else if (is_coffee_packet(action->arg1)) {
                    if (!coffee_packet_is_open) {
                        errors = append_error(errors, "Anticipation: Pouring from closed coffee packet");
                        results->anticipations++;
                    }
                    else if (added_coffee) {
                        if (previous_subtask == ADD_COFFEE) {
                            errors = append_error(errors, "Quality: Continuous perseveration of pouring coffee");
                            results->quality++;
                        }
                        else {
                            errors = append_error(errors, "Perseveration: Recurrent adding of coffee");
                            results->perseverations++;
                        }
                    }
                    else {
                        added_coffee = TRUE;
                    }
                    previous_subtask = ADD_COFFEE;
                }
                else if (is_cream_carton(action->arg1)) {
                    if (!cream_carton_is_open) {
                        errors = append_error(errors, "Anticipation: Pouring from closed cream carton");
                        results->anticipations++;
                    }
                    else if (added_cream) {
                        if (previous_subtask == ADD_MILK) {
                            errors = append_error(errors, "Quality: Continuous perseveration of pouring of cream");
                            results->quality++;
                        }
                        else {
                            errors = append_error(errors, "Perseveration: Recurrent adding of cream");
                            results->perseverations++;
                        }
                    }
                    else {
                        added_cream = TRUE;
                    }
                    previous_subtask = ADD_MILK;
                }
                else if (is_sugar_packet(action->arg1)) {
                    if (!sugar_packet_is_open) {
                        errors = append_error(errors, "Anticipation: Pouring from closed sugar packet");
                        results->anticipations++;
                    }
                    else if (added_sugar) {
                        if (previous_subtask == ADD_SUGAR) {
                            errors = append_error(errors, "Quality: Continuous perseveration of pouring sugar packet");
                            results->quality++;
                        }
                        else {
                            errors = append_error(errors, "Perseveration: Recurrent adding of sugar");
                            results->perseverations++;
                        }
                    }
                    else {
                        added_sugar = TRUE;
                    }
                    previous_subtask = ADD_SUGAR;
                }
                else if (is_spoon(action->arg1)) {
                    if (spoon_contents == NULL) {
                        if ((previous_subtask == ADD_COFFEE) || (previous_subtask == ADD_SUGAR) || (previous_subtask == ADD_MILK)) {
                            errors = append_error(errors, "Gesture subtitution: Pouring from empty spoon when we should be stirring");
                            results->gesture_sub++;
                        }
                        else {
                            errors = append_error(errors, "Anticipation: Pouring from empty spoon");
                            results->anticipations++;
                        }
                    }
                    else if (is_sugar(spoon_contents)) {
                        if (added_sugar) {
                            errors = append_error(errors, "Perseveration: Recurrent adding of sugar");
                            results->perseverations++;
                        }
                        else {
                            added_sugar = TRUE;
                        }
                        previous_subtask = ADD_SUGAR;
                    }
                    else if (is_coffee(spoon_contents)) {
                        if (added_coffee) {
                            errors = append_error(errors, "Perseveration: Recurrent adding of coffee");
                            results->perseverations++;
                        }
                        else {
                            added_coffee = TRUE;
                        }
                        previous_subtask = ADD_COFFEE;
                    }
                    else if (is_cream(spoon_contents)) {
                        if (added_cream) {
                            errors = append_error(errors, "Perseveration: Recurrent adding of cream");
                            results->perseverations++;
                        }
                        else {
                            added_cream = TRUE;
                        }
                        previous_subtask = ADD_MILK;
                    }
                    else {
                        g_snprintf(buffer, 128, "Object substitution: Spooning %s into mug", spoon_contents);
                        errors = append_error(errors, buffer);
                        results->object_sub++;
                    }
                    spoon_contents = NULL;
                }
                else if (is_sugar_bowl(action->arg1)) {
                    if (!sugar_bowl_is_open) {
                        errors = append_error(errors, "Anticipation: Pouring from closed sugar bowl");
                        results->anticipations++;
                    }
                    else {
                        errors = append_error(errors, "Tool omission: Pouring open sugar bowl into mug");
                        results->tool_omission++;
                        if (added_sugar) {
                            if (previous_subtask == ADD_SUGAR) {
                                errors = append_error(errors, "Quality: Continuous perseveration of pouring of sugar bowl");
                                results->quality++;
                            }
                            else {
                                errors = append_error(errors, "Perseveration: Recurrent adding of sugar");
                                results->perseverations++;
                            }
                        }
                        else {
                            added_sugar = TRUE;
                        }
                    }
                    previous_subtask = ADD_SUGAR;
                }
                else if (is_lid(action->arg1)) {
                    if (lid_contents == NULL) {
                        errors = append_error(errors, "Anticipation: Pouring from empty lid");
                        results->anticipations++;
                    }
                    else if (is_sugar(lid_contents)) {
                        if (added_sugar) {
                            errors = append_error(errors, "Perseveration: Recurrent adding of sugar");
                            results->perseverations++;
                        }
                        else {
                            added_sugar = TRUE;
                        }
                        previous_subtask = ADD_SUGAR;
                    }
                    else if (is_coffee(lid_contents)) {
                        if (added_coffee) {
                            errors = append_error(errors, "Perseveration: Recurrent adding of coffee");
                            results->perseverations++;
                        }
                        else {
                            added_coffee = TRUE;
                        }
                        previous_subtask = ADD_COFFEE;
                    }
                    else if (is_cream(lid_contents)) {
                        if (added_cream) {
                            errors = append_error(errors, "Perseveration: Recurrent adding of cream");
                            results->perseverations++;
                        }
                        else {
                            added_cream = TRUE;
                        }
                        previous_subtask = ADD_MILK;
                    }
                    else {
                        g_snprintf(buffer, 128, "Object substitution: Spooning %s with lid into mug", lid_contents);
                        errors = append_error(errors, buffer);
                        results->object_sub++;
                    }
                    lid_contents = NULL;
		}
		else {
		    g_snprintf(buffer, 128, "Object substitution: Pouring %s into mug", action->arg1);
                    errors = append_error(errors, buffer);
                    results->object_sub++;
		}
                break;
            }
            case ACTION_PEEL_OPEN: {
                if (action->arg1 == NULL) {
                    errors = append_error(errors, "Bizarre: NULL object in peeling open!");
                    results->bizarre++;
		}
                else if (!is_cream_carton(action->arg1)) {
                    g_snprintf(buffer, 128, "Object substitution (%s) on peeling open", action->arg1);
                    errors = append_error(errors, buffer);
                    results->object_sub++;
                }
                else if (!cream_carton_is_open) {
                    cream_carton_is_open = TRUE;
                }
                else if ((action->prev != NULL) && (action->prev->act == ACTION_PEEL_OPEN)) {
                    errors = append_error(errors, "Quality: Continuous perseveration on opening of cream carton");
                    results->quality++;
                }
                else {
                    errors = append_error(errors, "Perseveration: Recurrent opening of cream carton");
                    results->perseverations++;
                }
                break;
            }
            case ACTION_TEAR_OPEN: {
                if (action->arg1 == NULL) {
                    errors = append_error(errors, "Bizarre: NULL object in tearing open!");
                    results->bizarre++;
		}
                else if (!is_sugar_packet(action->arg1)) {
                    g_snprintf(buffer, 128, "Object substitution (%s) on tearing open", action->arg1);
                    errors = append_error(errors, buffer);
                    results->object_sub++;
                }
                else if (!sugar_packet_is_open) {
                    sugar_packet_is_open = TRUE;
                }
                else if ((action->prev != NULL) && (action->prev->act == ACTION_TEAR_OPEN)) {
                    errors = append_error(errors, "Quality: Continuous perseveration on opening sugar packet");
                    results->quality++;
                }
                else {
                    errors = append_error(errors, "Perseveration: Recurrent opening of sugar packet");
                    results->perseverations++;
                }
                break;
            }
            case ACTION_PULL_OPEN: {
                if (action->arg1 == NULL) {
                    errors = append_error(errors, "Bizarre: NULL object in pulling open!");
                    results->bizarre++;
		}
                else if (!is_coffee_packet(action->arg1)) {
                    g_snprintf(buffer, 128, "Object substitution (%s) on pulling open", action->arg1);
                    errors = append_error(errors, buffer);
                    results->object_sub++;
                }
                else if (!coffee_packet_is_open) {
                    coffee_packet_is_open = TRUE;
                }
                else if ((action->prev != NULL) && (action->prev->act == ACTION_PULL_OPEN)) {
                    errors = append_error(errors, "Quality: Continuous perseveration on opening coffee packet");
                    results->quality++;
                }
                else {
                    errors = append_error(errors, "Perseveration: Recurrent opening of coffee packet");
                    results->perseverations++;
                }
                break;
            }
            case ACTION_PULL_OFF: {
                if (action->arg1 == NULL) {
                    errors = append_error(errors, "Bizarre: NULL object in pulling off!");
                    results->bizarre++;
		}
                else if (!is_sugar_bowl(action->arg1)) {
                    g_snprintf(buffer, 128, "Object substitution (%s) on pulling off", action->arg1);
                    errors = append_error(errors, buffer);
                    results->object_sub++;
                }
                else if (!sugar_bowl_is_open) {
                    sugar_bowl_is_open = TRUE;
                }
                else if ((action->prev != NULL) && (action->prev->act == ACTION_PULL_OPEN)) {
                    errors = append_error(errors, "Quality: Continuous perseveration on opening sugar bowl");
                    results->quality++;
                }
                else {
                    errors = append_error(errors, "Perseveration: Recurrent opening of sugar bowl");
                    results->perseverations++;
                }
                break;
            }
            case ACTION_SCOOP: {
                if (action->arg2 == NULL) {
                    errors = append_error(errors, "Tool omission: Scooping without an implement");
                    results->tool_omission++;
                }
                else if (is_spoon(action->arg2)) {
                    if (spoon_contents != NULL) {
                        if (action_compare(action->prev, ACTION_SCOOP, action->arg1, "spoon")) {
                            errors = append_error(errors, "Quality: Perseverative scooping with full spoon");
                            results->quality++;
                        }
                        else if (action_compare(action->next, ACTION_POUR, "spoon", NULL)) {
                            errors = append_error(errors, "Reversal: Scooping with full spoon");
                            results->reversals++;
                        }
                        else {
                            errors = append_error(errors, "Perseveration: Recurrent scooping with full spoon");
                            results->perseverations++;
                        }
                    }
                    else if (is_mug(action->arg1)) {
                        errors = append_error(errors, "Object substitution: Scooping from mug");
                        results->object_sub++;
                    }
                    else if (is_cream_carton(action->arg1) && !cream_carton_is_open) {
                        errors = append_error(errors, "Geature substitution: Scooping from closed cream carton");
                        results->gesture_sub++;
                    }
                    else if (is_sugar_packet(action->arg1) && !sugar_packet_is_open) {
                        errors = append_error(errors, "Geature substitution: Scooping from closed sugar packet");
                        results->gesture_sub++;
                    }
                    else if (is_coffee_packet(action->arg1) && !coffee_packet_is_open) {
                        errors = append_error(errors, "Geature substitution: Scooping from closed coffee packet");
                        results->gesture_sub++;
                    }
                    else if (!is_sugar_bowl(action->arg1)) {
                        g_snprintf(buffer, 128, "Object substitution: Scooping from %s (should be sugar bowl)", action->arg1);
                        errors = append_error(errors, buffer);
                        results->object_sub++;
                    }
                    else if (!sugar_bowl_is_open) {
                        errors = append_error(errors, "Anticipation: Scooping from closed sugar bowl");
                        results->anticipations++;
                    }
                    else {
                        spoon_contents = "sugar";
                    }
                }
                else if (is_lid(action->arg2)) {
                    if (lid_contents != NULL) {
                        if (action_compare(action->prev, ACTION_SCOOP, action->arg1, "lid")) {
                            errors = append_error(errors, "Quality: Continuous perseveration of scooping with full lid");
                            results->quality++;
                        }
                        else if (action_compare(action->next, ACTION_POUR, "lid", NULL)) {
                            errors = append_error(errors, "Reversal: Scooping with full lid");
                            results->reversals++;
                        }
                        else {
                            errors = append_error(errors, "Perseveration: Recurrent scooping with full lid");
                            results->perseverations++;
                        }
                    }
                    else if (!is_sugar_bowl(action->arg1)) {
                        g_snprintf(buffer, 128, "Action addition: Scooping from non sugar bowl (%s)", action->arg1);
                        errors = append_error(errors, buffer);
                        results->action_addition++;
                    }
                    else if (!sugar_bowl_is_open) {
                        errors = append_error(errors, "Anticipation: Scooping from closed sugar bowl");
                        results->anticipations++;
                    }
                    else {
                        if (sugar_bowl_lid_is_poured_into_mug(action->next)) {
                            errors = append_error(errors, "Object substitution: Scooping with lid");
                            results->object_sub++;
			}
			else {
                            errors = append_error(errors, "Action addition: Scooping with lid");
                            results->action_addition++;
			}
                        lid_contents = "sugar";
                    }
                }
                else if (is_sugar_bowl(action->arg2)) {
                    errors = append_error(errors, "Object substitution: Scooping with sugar bowl");
                    results->object_sub++;
                }
                else if (is_mug(action->arg2)) {
                    errors = append_error(errors, "Object substitution: Scooping with mug");
                    results->object_sub++;
                }
                else {
                    g_snprintf(buffer, 128, "Gesture substitution: Scooping with %s", action->arg2);
                    errors = append_error(errors, buffer);
                    results->gesture_sub++;
                }
                break;
            }
            case ACTION_SIP: {
                if (action->arg1 == NULL) {
                    errors = append_error(errors, "Bizarre: NULL object in sipping!");
                    results->bizarre++;
                }
                else if (is_sugar_bowl(action->arg1)) {
                    if (sugar_bowl_is_open) {
                        errors = append_error(errors, "Action addition: Sipping from the open sugar bowl");
                        results->action_addition++;
                    }
                    else {
                        errors = append_error(errors, "Anticipation: Sipping from the closed sugar bowl");
                        results->anticipations++;
                    }
                    previous_subtask = A2_BREAK;
                }
                else if (is_sugar_packet(action->arg1)) {
                    if (sugar_packet_is_open) {
                        errors = append_error(errors, "Action addition: Sipping from the open sugar packet");
                        results->action_addition++;
                    }
                    else {
                        errors = append_error(errors, "Anticipation: Sipping from closed sugar packet");
                        results->anticipations++;
                    }
                    previous_subtask = A2_BREAK;
                }
                else if (is_coffee_packet(action->arg1)) {
                    if (coffee_packet_is_open) {
                        errors = append_error(errors, "Action addition: Sipping from the open coffee packet");
                        results->action_addition++;
                    }
                    else {
                        errors = append_error(errors, "Anticipation: Sipping from closed coffee packet");
                        results->anticipations++;
                    }
                    previous_subtask = A2_BREAK;
                }
                else if (is_cream_carton(action->arg1)) {
                    if (cream_carton_is_open) {
                        errors = append_error(errors, "Action addition: Sipping from the open cream carton");
                        results->action_addition++;
                    }
                    else {
                        errors = append_error(errors, "Anticipation: Sipping from closed cream carton");
                        results->anticipations++;
                    }
                    previous_subtask = A2_BREAK;
                }
                else if (is_spoon(action->arg1)) {
                    if (spoon_contents == NULL) {
                        errors = append_error(errors, "Gesture substitution: Sipping from empty spoon");
                        results->gesture_sub++;
                    }
                    else {
                        errors = append_error(errors, "Action addition: Sipping from non-empty spoon");
                        results->action_addition++;
                        previous_subtask = A2_BREAK;
                        spoon_contents = NULL;
                    }
                }
                else if (!is_mug(action->arg1)) {
                    g_snprintf(buffer, 128, "Gesture substitution: Sipping from %s", action->arg1);
                    errors = append_error(errors, buffer);
                    results->gesture_sub++;
                    previous_subtask = A2_BREAK;
                }
                else {
                    sip_count++;
                    if (sip_count > 2) {
                        if (previous_subtask == DRINK_BEVERAGE) {
                            errors = append_error(errors, "Perseveration: Continuous sipping");
                            results->perseverations++;
                        }
                        else {
                            errors = append_error(errors, "Perseveration: Recurrent sipping (after a pause)");
                            results->perseverations++;
                        }
                    }
                    if (!all_ingredients_added(task) && ingredients_added_after_current_action(action)) {
                        errors = append_error(errors, "Anticipation: Sipping before adding further ingredients");
                        results->anticipations++;
                    }
                    previous_subtask = DRINK_BEVERAGE;
                }
                break;
            }
            case ACTION_STIR: {
                if (action->arg2 == NULL) {
                    errors = append_error(errors, "Tool omission on stirring");
                    results->tool_omission++;
                }
                else if (!is_spoon(action->arg2)) {
                    g_snprintf(buffer, 128, "Object substitution: Stirring with %s", action->arg1);
                    errors = append_error(errors, buffer);
                    results->object_sub++;
                }
                else if (is_spoon(action->arg1)) {
                    errors = append_error(errors, "Bizarre: Stirring spoon with spoon!");
                    results->bizarre++;
                }
                else if (!is_mug(action->arg1)) {
                    g_snprintf(buffer, 128, "Object substitution: Stirring the %s with spoon", action->arg1);
                    errors = append_error(errors, buffer);
                    results->object_sub++;
                }
                else if (previous_subtask == STIR_DRINK) {
                    errors = append_error(errors, "Perseveration: Continuous stirring");
                    results->perseverations++;
                }
                else if (previous_subtask == ADD_COFFEE) {
                    stirred_coffee = TRUE;
                }                
                else if (previous_subtask == ADD_SUGAR) {
                    stirred_sugar = TRUE;
                }                
                else if (previous_subtask == ADD_MILK) {
                    stirred_cream = TRUE;
                }
                else if (!added_cream && !added_sugar && !added_coffee) {
                    if (next_steps_add_but_dont_stir_ingredient(action)) {
                        errors = append_error(errors, "Reversal: Stirring before adding");
                        results->anticipations++;
                    }
                    else {
                        errors = append_error(errors, "Anticipation: Stirring before adding anything");
                        results->anticipations++;
                    }
                }
                else if (all_ingredients_have_been_stirred_in()) {
                    errors = append_error(errors, "Perseveration: Recurrent stirring");
                    results->perseverations++;
                }
                else {
                    if (added_coffee) { stirred_coffee = TRUE; }
                    if (added_sugar)  { stirred_sugar = TRUE; }
                    if (added_cream)   { stirred_cream = TRUE; }
                }
                previous_subtask = STIR_DRINK;
                break;
            }
            case ACTION_DIP: {
                if (action->arg1 == NULL) {
                    errors = append_error(errors, "Tool omission on steeping");
                    results->tool_omission++;
                }
                else if (!is_teabag(action->arg1)) {
                    g_snprintf(buffer, 128, "Object substitution (%s) on steeping", action->arg1);
                    errors = append_error(errors, buffer);
                    results->object_sub++;
		}
                else if (!is_mug(action->arg2)) {
                    g_snprintf(buffer, 128, "Object substitution: Steeping in %s", action->arg2);
                    errors = append_error(errors, buffer);
                    results->object_sub++;
                }
                else if (!added_tea) {
                    added_tea = TRUE;
                }
                else if (previous_subtask == STEEP_TEA) {
                    errors = append_error(errors, "Perseveration: Continuous steeping of the tea");
                    results->perseverations++;
                }
                else {
                    errors = append_error(errors, "Perseveration: Recurrent tea steeping (after a pause)");
                    results->perseverations++;
                }
                previous_subtask = STEEP_TEA;
                break;
            }
            case ACTION_SAY_DONE: {
                previous_subtask = A2_BREAK;
                break;
            }
            default: {
                fprintf(stderr, "Unknown action: %d\n", (int) action->act);
                break;
            }
        }
    }
    return(errors);
}

static GList *check_for_additions_and_omissions(GList *errors, TaskType *task, ACS2 *results)
{
    switch (task->base) {
        case TASK_TEA: {
            if (added_coffee) {
                errors = append_error(errors, "Action addition: Coffee added when making tea");
                results->action_addition++;
            }
            if (!added_tea) {
                errors = append_error(errors, "Omission: Tea omitted when making tea");
                results->omissions++;
            }
            if (added_cream) {
                errors = append_error(errors, "Action addition: Milk added when making tea");
                results->action_addition++;
            }
            if (!added_sugar) {
                errors = append_error(errors, "Omission: Sugar not added");
                results->omissions++;
            }
            else if (!stirred_sugar) {
                errors = append_error(errors, "Omission: Sugar added but not stirred in");
                results->omissions++;
            }
            if (sip_count == 0) {
                errors = append_error(errors, "Omission: Drink not drunk");
                results->omissions++;
            }
            else if (sip_count == 1) {
                errors = append_error(errors, "Omission: Only one sip");
                results->omissions++;
            }

            if (added_tea && added_sugar && stirred_sugar && (sip_count == 2) && task_complete) {
                results->accomplished = 1;
            }
            break;
        }
        case TASK_COFFEE: {
            if (!added_coffee) {
                errors = append_error(errors, "Omission: Coffee omitted when making coffee");
                results->omissions++;
            }
            else if (!stirred_coffee) {
                errors = append_error(errors, "Omission: Coffee added but not stirred in");
                results->omissions++;
            }
            if (added_tea) {
                errors = append_error(errors, "Action addition: Tea added when making coffee");
                results->action_addition++;
            }
            if (!added_cream) {
                errors = append_error(errors, "Omission: Milk not added when making coffee");
                results->omissions++;
            }
            else if (!stirred_cream) {
                errors = append_error(errors, "Omission: Milk added but not stirred in");
                results->omissions++;
            }
            if (!added_sugar) {
                errors = append_error(errors, "Omission: Sugar not added");
                results->omissions++;
            }
            else if (!stirred_sugar) {
                errors = append_error(errors, "Omission: Sugar added but not stirred in");
                results->omissions++;
            }
            if (sip_count == 0) {
                errors = append_error(errors, "Omission: Drink not drunk");
                results->omissions++;
            }
            else if (sip_count == 1) {
                errors = append_error(errors, "Omission: Only one sip");
                results->omissions++;
            }

            if (added_coffee && added_cream && stirred_cream && added_sugar && stirred_sugar && (sip_count == 2) && task_complete) {
                results->accomplished = 1;
            }

            break;
        }
        default: {
            break;
        }
    }
    if (!task_complete) {
        errors = append_error(errors, "Capture: Task not completed");
    }
    return(errors);
}

#if LOG_ERRORS
static void log_actions_to_file(char *file)
{
    FILE *fp = fopen(file, "a");
    print_actions_to_file(fp);
    fclose(fp);
}
#endif

GList *analyse_actions_with_acs2(TaskType *task, ACS2 *results)
{
    GList *errors = NULL;

#if LOG_ERRORS
    log_actions_to_file("ERROR_LOG");
#endif
    initialise_state();
    initialise_counts(results);
    errors = process_all_actions(errors, task, results);
    errors = check_for_additions_and_omissions(errors, task, results);
    return(errors);
}

void analyse_actions_code_free_error_list(GList *errors)
{
    while (errors != NULL) {
        free(errors->data);
        errors = g_list_delete_link(errors, errors);
    }
}

/******************************************************************************/
