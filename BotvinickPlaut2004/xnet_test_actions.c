#include "xframe.h"

static GtkWidget *action_list = NULL;
static GtkWidget *transcript1 = NULL;
static GtkWidget *transcript2 = NULL;

/******************************************************************************/
/* Functions for creating/maintaining the list of attempted actions ***********/

void action_list_report(Boolean success, char *world_error_string)
{
    char cycle[6];
    char *row[3];

    g_snprintf(cycle, 6, "%d", xg.step);

    row[0] = cycle;
    row[1] = (success ? " " : "ERROR");
    row[2] = world_error_string;

    gtk_clist_append(GTK_CLIST(action_list), row);
}

void action_list_create(GtkWidget *page)
{
    char *titles[3] = {" Step ", "   OK?   ", " Action "};
    GtkWidget *tmp = gtk_scrolled_window_new(NULL, NULL);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tmp), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(page), tmp);
    gtk_widget_show(tmp);

    action_list = gtk_clist_new_with_titles(3, titles);
    gtk_container_add(GTK_CONTAINER(tmp), action_list);
    gtk_widget_show(action_list);
}

void action_list_clear()
{
    gtk_clist_clear(GTK_CLIST(action_list));
    gtk_clist_clear(GTK_CLIST(transcript1));
    gtk_clist_clear(GTK_CLIST(transcript2));
}

/******************************************************************************/

void action_log_report(char *string)
{
    gtk_clist_append(GTK_CLIST(transcript1), &string);
}

void acs1_transcript1_create(GtkWidget *page)
{
    GtkWidget *tmp = gtk_scrolled_window_new(NULL, NULL);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tmp), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(page), tmp);
    gtk_widget_show(tmp);

    transcript1 = gtk_clist_new(1);
    gtk_container_add(GTK_CONTAINER(tmp), transcript1);
    gtk_widget_show(transcript1);
}

void acs1_transcript2_create(GtkWidget *page)
{
    char *titles[4] = {" Analysis ", " Attribute                               ", " Value ", " Proportion "};

    GtkWidget *tmp = gtk_scrolled_window_new(NULL, NULL);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tmp), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(page), tmp);
    gtk_widget_show(tmp);

    transcript2 = gtk_clist_new_with_titles(4, titles);
    gtk_container_add(GTK_CONTAINER(tmp), transcript2);
    gtk_widget_show(transcript2);
}

/******************************************************************************/

static void acs1_analysis_report(char *attribute, int value, int max)
{
    char buffer1[10], buffer2[10];
    char *row[4] = {"ACS 1", attribute, buffer1, buffer2};

    g_snprintf(buffer1, 10, "%4d", value);
    g_snprintf(buffer2, 10, "%4.2f", value / (double) max);
    gtk_clist_append(GTK_CLIST(transcript2), row);
}

void print_analysis_acs1(ACS1 *results)
{
    acs1_analysis_report(" Actions", results->actions, results->actions);
    acs1_analysis_report(" Independents", results->independents, results->actions);
    acs1_analysis_report(" Crux Errors", results->errors_crux, results->actions);
    acs1_analysis_report(" Non-crux Errors", results->errors_non_crux, results->actions);
}

static void acs2_analysis_report(char *attribute, int value)
{
    char buffer[10];
    char *row[4] = {"ACS 2", attribute, buffer, ""};

    g_snprintf(buffer, 10, "%4d", value);
    gtk_clist_append(GTK_CLIST(transcript2), row);
}

static void acs2_analysis_report_error(char *attribute)
{
    char *row[4] = {"Error", attribute, "", ""};

    gtk_clist_append(GTK_CLIST(transcript2), row);
}

void print_analysis_acs2(ACS2 *results, GList *errors)
{
    acs2_analysis_report("  Omission", results->omissions);
    acs2_analysis_report("  Sequence: Anticipations", results->anticipations);
    acs2_analysis_report("  Sequence: Perseverations", results->perseverations);
    acs2_analysis_report("  Sequence: Reversals", results->reversals);
    acs2_analysis_report("  Object substitutions", results->object_sub);
    acs2_analysis_report("  Gesture subtitutions", results->gesture_sub);
    acs2_analysis_report("  Tool omissions", results->tool_omission);
    acs2_analysis_report("  Action additions", results->action_addition);
    acs2_analysis_report("  Quality", results->quality);

    while (errors != NULL) {
        acs2_analysis_report_error(errors->data);
        errors = errors->next;
    }
}

/******************************************************************************/

