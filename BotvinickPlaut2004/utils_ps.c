
#include "utils_ps.h"
#include <sys/stat.h>
#include <time.h>
#include <pwd.h>
#include <string.h>

extern int    getuid();

static void get_host_name(char *buffer, int l)
{
    extern int gethostname(char *buffer, int length);

    if (gethostname(buffer, l) < 0) {
        strncpy(buffer, "unknown-host", l);
    }
}

void ps_print_string(FILE *fp, const char *string)
{
    int c, i = 0;

    while ((c = string[i++]) != '\0') {
        if ((c == '\\') || (c == '(') || (c == ')') || (c == '\t')) {
            putc('\\', fp); putc(c, fp);
        }
        else {
            putc(c, fp);
        }
    }
}

void ps_text_centre(FILE *fp, int x, int y, const char *string)
{
    fprintf(fp, "%d %d m (", x, y);
    ps_print_string(fp, string);
    fprintf(fp, ") dup stringwidth pop 2 div neg 0 rm show\n");
}

void ps_text_right_justify(FILE *fp, int x, int y, const char *string)
{
    fprintf(fp, "%d %d m (", x, y);
    ps_print_string(fp, string);
    fprintf(fp, ") dup stringwidth pop neg 0 rm show\n");
}

void ps_text_left_justify(FILE *fp, int x, int y, const char *string)
{
    fprintf(fp, "%d %d m (", x, y);
    ps_print_string(fp, string);
    fprintf(fp, ") show\n");
}

void ps_vector_out(FILE *fp, double *vector, int width, int x, int y)
{
    int j;

    for (j = 0; j < width; j++) {
        double c = 1.0 - vector[j];
        fprintf(fp, "%5.3f %5.3f %5.3f setrgbcolor\n", c, c, c);
        fprintf(fp, "%d %d m 12 0 rl 0 12 rl -12 0 rl 0 -12 rl fill stroke\n", x+12*j, y);
        fprintf(fp, "0.0 0.0 0.0 setrgbcolor\n");
        fprintf(fp, "%d %d m 12 0 rl 0 12 rl -12 0 rl 0 -12 rl stroke\n", x+12*j, y);
    }
}

static void fprint_ps_colour_defs_etc(FILE *fp)
{
    fprintf(fp, "    /l {lineto} bind def\n");
    fprintf(fp, "    /rl {rlineto} bind def\n");
    fprintf(fp, "    /m {moveto} bind def\n");
    fprintf(fp, "    /rm {rmoveto} bind def\n");
    fprintf(fp, "    /s {stroke} bind def\n");
    fprintf(fp, "    /gs {gsave} bind def\n");
    fprintf(fp, "    /gr {grestore} bind def\n");
    fprintf(fp, "    /nl {gsave show grestore 0 %d neg rm} bind def\n", 13);
    fprintf(fp, "end\n");
}

void initialise_eps_file(FILE *fp, int w, int h)
{
    struct passwd  *who;
    time_t          when;
    char            host[256];

    who = getpwuid(getuid());
    get_host_name(host, 256);
    time(&when);

    fprintf(fp, "%%!PS-Adobe-2.0\n");
    fprintf(fp, "%%%%Title: EPS figure\n");
    fprintf(fp, "%%%%Creator: %s\n", "XBP");
    fprintf(fp, "%%%%CreationDate: %s", ctime(&when));
    if (who != NULL) {
        fprintf(fp, "%%%%For: %s@%s (%s)\n", who->pw_name, host, who->pw_gecos);
    }
    
    fprintf(fp, "%%%%BoundingBox: 0 0 %d %d\n", w, h);
    fprintf(fp, "%%%%EndComments\n");

    fprintf(fp, "/PmDict 200 dict def\n");
    fprintf(fp, "PmDict begin\n");
    fprintf(fp, "    PmDict /mtrx matrix put\n");
    fprint_ps_colour_defs_etc(fp);
    fprintf(fp, "%%%%EndProlog\n");
    fprintf(fp, "PmDict begin /EnteredState save def\n");
    fprintf(fp, "1 setlinecap 0 setlinejoin gs\n");
    fprintf(fp, "%%%%Page: 1\n");
}

/*----------------------------------------------------------------------------*/

void terminate_eps_file(FILE *fp)
{
    fprintf(fp, "showpage\n");
    fprintf(fp, "%%%%Trailer\n");
    fprintf(fp, "EnteredState restore end\n");
}

/******************************************************************************/
