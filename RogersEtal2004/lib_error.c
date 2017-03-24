/*******************************************************************************

    File:       lib_error.c
    Contents:   Library for reporting errors of various types
    Author:     Rick Cooper
    Copyright (c) 2016 Richard P. Cooper

    Public procedures:
        lib_error_report(ErrorType et, const char *message)
        TODO(int priority, const char *message)

Note: The above are actually symbols defined in lib_error.h. The symbols
ensure that the corresponding functions are called with correct location
information so that errors etc. report the line/function/file where the errors
was detected.

*******************************************************************************/
/******** Include files: ******************************************************/

#include <stdio.h>
#include <glib.h>
#include "lib_error.h"

#ifdef MALLOC_CHECK
#include "../lib/zmalloc.h"
#endif

void _lib_error_report5(ErrorType err, const char *when, const char *where_file, const char *where_func, int where_line)
{
    char warning[64];

    switch (err) {
        case ERROR_NONE: {
            warning[0] = '\0';
            break;
        }
        case ERROR_DUMMY: {
            g_snprintf(warning, 64, "Dummy error");
            break;
        }
        case ERROR_MALLOC_FAILED: {
            g_snprintf(warning, 64, "Memory allocation failed");
            break;
        }
        case ERROR_INTERNAL: {
            g_snprintf(warning, 64, "Internal error");
            break;
        }
        case ERROR_UNREGISTERED_HANDLE: {
            g_snprintf(warning, 64, "Unregistered model type");
            break;
        }
        case ERROR_NO_READ_ACCESS: {
            g_snprintf(warning, 64, "No read access");
            break;
        }
        case ERROR_NO_WRITE_ACCESS: {
            g_snprintf(warning, 64, "No write access");
            break;
        }
        case ERROR_FILE_OPEN_FAILED: {
            g_snprintf(warning, 64, "Cannot open file");
            break;
        }
        case ERROR_FILE_CLOSE_FAILED: {
            g_snprintf(warning, 64, "Cannot close file");
            break;
        }
        case ERROR_FILE_COPY_FAILED: {
            g_snprintf(warning, 64, "File copy failed");
            break;
        }
        case ERROR_FILE_SAVE_FAILED: {
            g_snprintf(warning, 64, "File save failed");
            break;
        }
        case ERROR_FILE_DELETE_FAILED: {
            g_snprintf(warning, 64, "File delete failed");
            break;
        }
        case ERROR_ILLFORMED_DATA: {
            g_snprintf(warning, 64, "Illformed data");
            break;
        }
        case ERROR_OPERATION_FAILED: {
            g_snprintf(warning, 64, "Operation cancelled");
            break;
        }
        case ERROR_OPERATION_CANCELLED: {
            g_snprintf(warning, 64, "Operation cancelled");
            break;
        }
        case ERROR_INSUFFICIENT_STRING_SPACE: {
            g_snprintf(warning, 64, "Insufficient pre-allocated string space");
            break;
        }
        case ERROR_UNSPECIFIED: {
            g_snprintf(warning, 64, "Unspecified error");
            break;
        }
    }
    
    if (warning[0] != '\0') {
        char message[512];

        g_snprintf(message, 512, "Warning: %s\n%s\n\nFile: %s\nFunction: %s\nLine: %d", warning, when, where_file, where_func, where_line);
        fprintf(stdout, "%s", message);
    }
}

void _TODO5(int priority, const char *message, const char *where_file, const char *where_func, int where_line)
{
    if (priority < 5) {
        fprintf(stdout, "PRIORITY %d (%s:%s:%d): %s\n", priority, where_file, where_func, where_line, message);
    }
}

