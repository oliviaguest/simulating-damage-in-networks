/*******************************************************************************

    File:       lib_string.c
    Contents:   Miscellaneous string manipulation functions.
    Author:     Rick Cooper
    Copyright (c) 2004 Richard P. Cooper

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

    Public procedures:
        char *string_new(int l);
        char *string_copy(const char *s);
        char *string_copy_substring(const char *s, int l);
        void string_trim_trailing_blanks(char *string);
        int string_split(char *fullstring);
        int string_is_positive_integer(const char *text, long *i);
        int string_is_real_number(const char *text, double *i);

*******************************************************************************/
/******** Include files: ******************************************************/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lib_string.h"

#ifdef MALLOC_CHECK
#include "zmalloc.h"
#endif

/******************************************************************************/

char *string_new(int l)
{
    if (l > 0) {
        return((char *)malloc(l * sizeof(char)));
    }
    else {
        return(NULL);
    }
}

/*----------------------------------------------------------------------------*/

void string_free(char *string)
{
    if (string != NULL) {
        free(string);
    }
}

/*----------------------------------------------------------------------------*/

char *string_copy(const char *s)
{
    char *copy;

    if (s == NULL) {
        return(NULL);
    }
    else if ((copy = string_new(strlen(s)+1)) == NULL) {
        return(NULL);
    }
    else {
        strcpy(copy, s);
        return(copy);
    }
}

/*----------------------------------------------------------------------------*/

char *string_copy_substring(const char *s, int l)
{
    char *copy;

    if (s == NULL) {
        return(NULL);
    }
    else if ((copy = string_new(l+1)) == NULL) {
        return(NULL);
    }
    else {
        strncpy(copy, s, l);
        copy[l] = '\0';
        return(copy);
    }
}

/*----------------------------------------------------------------------------*/

void string_trim_trailing_blanks(char *string)
{
    int l = 0;
    while (string[l] != '\0') {
        l++;
    }
    while ((l-- > 0) && (string[l] == ' ')) {
        string[l] = '\0';
    }
}

/*----------------------------------------------------------------------------*/

void string_delete_ctrl_chars(char *string)
{
    int l = 0;

    while (string[l] != '\0') {
        if (iscntrl(string[l])) {
            string[l] = ' ';
        }
        l++;
    }
}

/*----------------------------------------------------------------------------*/

int string_split(char *fullstring)
{
    int l = strlen(fullstring);
    int i = l / 2, j = l / 2;

    /* Find the first space character in fullstring before fullstring[l/2]:   */

    while ((i > 0) && (fullstring[i] != ' ')) {
        i--;
    }

    /* Find the first space character in fullstring after fullstring[l/2]:    */

    while ((j < l) && (fullstring[j] != ' ')) {
        j++;
    }

    /* Choose i or j, whichever is closest to l/2                             */

    if ((i > 0) || (j < l)) {
        return(((l/2 - i) > (j - l/2)) ? j : i);
    }
    else {
        return(0);
    }
}

/*----------------------------------------------------------------------------*/

int string_is_positive_integer(const char *text, long *i)
{
    int c = 0;

    *i = 0;

    while (text[c] == ' ') {
        c++;
    }
    while (isdigit((int) text[c])) {
        *i = *i * 10 + (text[c++] - '0');
    }
    return(text[c] == '\0');
}

/*----------------------------------------------------------------------------*/

int string_is_real_number(const char *text, double *i)
{
    int sign = 1;
    int c = 0;

    *i = 0.0;

    while (text[c] == ' ') {
        c++;
    }
    if (text[c] == '-') {
        c++; sign = -1;
    }

    while (isdigit((int) text[c])) {
        *i = *i * 10 + (text[c++] - '0');
    }
    if (text[c++] == '.') {
        double denom = 10.0;
        while (isdigit((int) text[c])) {
            *i = *i + ((text[c++] - '0') / denom);
            denom = denom * 10;
        }
    }
    *i = *i * sign;
    return(text[c] == '\0');
}

/******************************************************************************/
