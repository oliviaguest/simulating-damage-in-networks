#ifndef _lib_string_h_

#define  _lib_string_h_

#include <stdlib.h>

extern char *string_new(int l);
extern void  string_free(char *s);
extern char *string_copy(const char *s);
extern char *string_copy_substring(const char *s, int l);
extern void  string_delete_ctrl_chars(char *string);
extern void  string_trim_trailing_blanks(char *string);
extern int   string_split(char *fullstring);
extern int   string_is_positive_integer(const char *text, long *i);
extern int   string_is_real_number(const char *text, double *i);

#endif
