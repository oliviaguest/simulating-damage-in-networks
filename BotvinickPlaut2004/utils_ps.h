#include <stdio.h>

extern void ps_print_string(FILE *fp, const char *string);
extern void ps_text_centre(FILE *fp, int x, int y, const char *string);
extern void ps_text_left_justify(FILE *fp, int x, int y, const char *string);
extern void ps_text_right_justify(FILE *fp, int x, int y, const char *string);
extern void ps_vector_out(FILE *fp, double *vector, int width, int x, int y);
extern void initialise_eps_file(FILE *fp, int w, int h);
extern void terminate_eps_file(FILE *fp);
