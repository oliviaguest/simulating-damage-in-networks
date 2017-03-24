/*******************************************************************************

    File:       lib_print.h
    Contents:   Type definitions and function declarations for generic printing routines
    Author:     Rick Cooper
    Copyright (c) 1998 Richard P. Cooper

    This file is part of COGENT.

    COGENT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    COGENT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with COGENT; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*******************************************************************************/

#ifndef lib_print_h

#define lib_print_h

#include "pl.h"
#include "lib_file.h"
#include <gtk/gtk.h>

typedef enum {PS_PORTRAIT, PS_LANDSCAPE, PS_TWO_UP} OutputPageStyle;
typedef enum {PS_FONT_FIXED, PS_FONT_SANS, PS_FONT_SERIF} OutputFontFace;
typedef enum {PF_ASCII, PF_HTML, PF_LATEX, PF_POSTSCRIPT, PF_PDF, PF_WIN32} OutputFormat;
typedef enum {PP_PRINT_FOLDER, PP_PRINT_COMMAND, PP_PRINT_FILE, PP_PRINT_TO_FILE,
    PP_PRINT_RECURSE, PP_PRINT_CONFIRM, PP_FONT_SIZE, PP_FONT_FACE, PP_PAGE_STYLE,
    PP_OUTPUT_FORMAT} PrintPreference;

typedef struct print_preferences {
    char           *print_directory;
    char           *print_com;
    char           *print_file;
    Boolean         print_to_file;
    Boolean         print_recurse;
    Boolean         print_confirm;
    int             print_font_size;
    OutputFontFace  print_font_face;
    OutputPageStyle print_page_style;
    OutputFormat    print_output_format;
} PrintPreferences;

/* SECTION 1: Temporary print file: *******************************************/

extern char *print_get_tmp_filename();

/* SECTION 2: Send a file to lpr, and then remove it: **************************/

extern void print_and_remove(const char *print_file);

/* SECTION 3: Miscellaneous utility functions for various printing formats: ***/

extern void latex_put_char(FILE *fp, int c);
extern void latex_print_string(FILE *fp, const char *string);
extern void latex_print_clause(FILE *fp, ClauseType *clause);

extern int print_margin();
extern int print_page_width();
extern int print_page_height();
extern int print_font_size();

extern void print_page_feed(void *device, int *y);
extern void print_underline(void *device, int level, int *y);
extern void print_heading(void *device, const char *string, int level, int *y);

/* SECTION 4: Initialise/Terminate different types of print file: *************/

extern void *print_initialise_file(const char *filename, const char *title);
extern void print_terminate_file(void *device);

/* SECTION 5: Print an ascii or prolog file in a given format: ****************/

extern void print_file_ascii(void *device, char *filename, ClauseList *declarations, int indent, int *y);
extern void print_file_pl(void *device, char *filename, ClauseList *declarations, int indent, int *y);
extern void print_named_file_ascii(char *title, char *subtitle, char *file, ClauseList *declarations);
extern void print_named_file_pl(char *title, char *subtitle, char *file, ClauseList *declarations);

/* SECTION 6: Print the contents of a description text window's buffer: *******/

extern void print_textsw(void *device, GtkTextView *text, int indent, int *y);
extern void print_text_view_buffer(GtkWidget *text, char *name);

/* SECTION 7: lprint primitives (for neater coding) ***************************/

extern void lprint_string(void *device, char *string);
extern void lprint_nl(void *device, int *y);
extern void lprint_indent(void *device, int indent);
extern void lprint_clause(void *device, ClauseType *clause);

/* SECTION 8: Set/query printer preferences ***********************************/

extern void *print_get_preference(PrintPreference id);
extern void print_set_preference(PrintPreference id, void *value);
extern void print_preference_create();
extern void print_preferences_dispose();

/* SECTION 9: Printer confirmation using the option panel *********************/

extern void    print_dialog_set_subheading(GtkBox *option_panel, char *label);
extern Boolean print_confirm(GtkWidget *parent, ErrorType (*populate_function)(GtkBox *box));

/******************************************************************************/

#endif
