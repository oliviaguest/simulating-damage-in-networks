/******************************************************************************/

typedef enum boolean {
    FALSE, TRUE}
Boolean;

#include "utils_network.h"
#include "lib_string.h"
#include <glib.h>
#include <ctype.h>

/******************************************************************************/

static void gfree(void *ptr)
{
    if (ptr != NULL) {
        free(ptr);
    }
}            

static double read_double(FILE *fp)
{
    double r = 0;
    int neg = 1;
    int c;

    if ((c = getc(fp)) == '-') {
        neg = -1;
    }
    else {
        ungetc(c, fp);
    }

    while (isdigit(c = fgetc(fp))) {
        r = r * 10.0 + (c - '0');
    }
    if (c == '.') {
        double denom = 1.0;
        while (isdigit(c = fgetc(fp))) {
            denom = denom * 10.0;
            r = r + ((c - '0') / denom);
        }
    }
    ungetc(c, fp);
    return(r * neg);
}

static Boolean read_training_data_item(FILE *fp, int n, double *vector_in, int m, double *vector_out)
{
    char c;
    int i;

    for (i = 0; i < n; i++) {
        while ((c = fgetc(fp)) == ' ') { };
        if ((c == '\n') || (c == EOF)) {
            return(FALSE);
        }
        else {
            ungetc(c, fp);
            vector_in[i] = read_double(fp);
        }
    }
    while ((c = fgetc(fp)) == ' ') { };
    if (c != '>') {
        return(FALSE);
    }
    for (i = 0; i < m; i++) {
        while ((c = fgetc(fp)) == ' ') { };
        if ((c == '\n') || (c == EOF)) {
            return(FALSE);
        }
        else {
            ungetc(c, fp);
            vector_out[i] = read_double(fp);
        }
    }
    while ((c = fgetc(fp)) == ' ') { };
    return(c == '\n');
}

static PatternList *pattern_list_allocate(int in_width, int out_width)
{
    PatternList *new;

    if ((new = (PatternList *)malloc(sizeof(PatternList))) == NULL) {
        return(NULL);
    }
    else if ((new->vector_in = (double *)malloc(in_width * sizeof(double))) == NULL) {
        free(new);
        return(NULL);
    }
    else if ((new->vector_out = (double *)malloc(out_width * sizeof(double))) == NULL) {
        free(new);
        free(new->vector_in);
        return(NULL);
    }
    else {
        new->name = NULL;
        return(new);
    }
}

PatternList *read_training_patterns(char *filename, int in_width, int out_width)
{
    /* Read training data from a file: Each line should represent one case */

    FILE *fp;

    if ((fp = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "WARNING: Cannot read %s (training patterns)\n", filename);
        return(NULL);
    }
    else {
        PatternList dummy = {NULL, NULL, NULL};
        PatternList *last = &dummy, *new;
        Boolean ok = TRUE;
        char buffer[16];
        int i = 0;

        do {
            if ((new = pattern_list_allocate(in_width, out_width)) == NULL) {
                ok = FALSE;
            }
            else if (read_training_data_item(fp, in_width, new->vector_in, out_width, new->vector_out)) {
                last->next = new;
                last = last->next;
                g_snprintf(buffer, 16, "Pattern %d", ++i);
                new->name = string_copy(buffer);
            }
            else {
                last->next = NULL;
                ok = FALSE;
            }
        } while (ok);
        fclose(fp);
        return(dummy.next);
    }
}

/******************************************************************************/

int pattern_list_length(PatternList *patterns)
{
    int n = 0;
    while (patterns != NULL) {
        n++;
        patterns = patterns->next;
    }
    return(n);
}

void pattern_list_print(PatternList *patterns, int in_width, int out_width, FILE *fp)
{
    int i = 0;
    while (patterns != NULL) {
        fprintf(fp, "%2d: ", ++i);
        print_vector(fp, in_width, patterns->vector_in);
        fprintf(fp, " => ");
        print_vector(fp, out_width, patterns->vector_out);
        fprintf(fp, "\n");
        patterns = patterns->next;
    }
}

void pattern_list_free(PatternList *patterns)
{
    PatternList *tmp;
    while (patterns != NULL) {
        tmp = patterns->next;
        gfree(patterns->name);
        gfree(patterns->vector_in);
        gfree(patterns->vector_out);
        free(patterns);
        patterns = tmp;
    }
}

/******************************************************************************/

void print_vector(FILE *fp, int n, double *vector_in)
{
    int i;

    for (i = 0; i < n; i++) {
        fprintf(fp, "%6.4f", vector_in[i]);
        if (i != n-1) {
            fputc(' ', fp);
        }
    }
}


void print_string(FILE *fp, int width, char *string)
{
    Boolean instring = TRUE;
    int i = 0;

    while (i < width) {
        if (!instring) {
            fputc(' ', fp);
        }
        else if (string[i] == '\0') {
            fputc(' ', fp);
            instring = FALSE;
        }
        else {
            fputc(string[i], fp);
        }
        i++;
    }
}

/******************************************************************************/
/******************************************************************************/

#if FALSE
void pattern_set_free(Patterns *list)
{
    Patterns *tmp;
    while (list != NULL) {
        tmp = list->next;
        free(list->name);
        free(list);
        list = tmp;
    }
}

static Boolean read_pattern_item(FILE *fp, int width, Patterns *new)
{
    char name[80];
    double r;
    int state = 0;
    int i, c, j;

    i = 0; j = 0;
    while ((state < 2) && ((c = getc(fp)) != EOF)) {
        if ((state == 0) && (c != '\n')) {
            if (i < 79) {
                name[i++] = c;
                name[i] = '\0';
            }
        }
        else if ((state == 0) && (c == '\n')) {
            state = 1;
        }
        else if ((state == 1) && isdigit(c)) {
            ungetc(c, fp);
            r = read_double(fp);
            if (j < width) {
                new->vector[j++] = r;
            }
        }
        else if ((state == 1) && isblank(c)) {
            // Blank space between values - ignore
        }
        else if ((state == 1) && (c == '\n')) {
            state = 2;
        }
    }
    new->name = string_copy(name);
    return(state == 2);
}

Patterns *pattern_set_read(char *filename, int width)
{
    FILE *fp;

    if ((fp = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "WARNING: Cannot read %s (training patterns)\n", filename);
        return(NULL);
    }
    else {
        Patterns  dummy = {NULL, {}, NULL};
        Patterns *last = &dummy, *new;
        Boolean ok = TRUE;

        do {
            if ((new = (Patterns *)malloc(sizeof(Patterns))) == NULL) {
                ok = FALSE;
            }
            else if (read_pattern_item(fp, width, new)) {
                last->next = new;
                last = last->next;
            }
            else {
                last->next = NULL;
                ok = FALSE;
            }
        } while (ok);
        fclose(fp);
        return(dummy.next);
    }
}

#endif
