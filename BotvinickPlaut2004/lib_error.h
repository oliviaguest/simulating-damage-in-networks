#ifndef _lib_error_h_

#define _lib_error_h_

typedef enum error_type {
    ERROR_NONE, ERROR_DUMMY, ERROR_MALLOC_FAILED,
    ERROR_INTERNAL, ERROR_UNREGISTERED_HANDLE,
    ERROR_NO_READ_ACCESS,   ERROR_NO_WRITE_ACCESS, 
    ERROR_FILE_OPEN_FAILED, ERROR_FILE_CLOSE_FAILED,
    ERROR_FILE_COPY_FAILED, ERROR_FILE_DELETE_FAILED,
    ERROR_FILE_SAVE_FAILED, ERROR_ILLFORMED_DATA,
    ERROR_OPERATION_FAILED, ERROR_OPERATION_CANCELLED, 
    ERROR_INSUFFICIENT_STRING_SPACE, ERROR_UNSPECIFIED
} ErrorType;

#define lib_error_report(A,B) _lib_error_report5(A,B,__FILE__,__FUNCTION__,__LINE__)
#define TODO(A,B)      _TODO5(A,B,__FILE__,__FUNCTION__,__LINE__)

extern void _lib_error_report5(ErrorType err, const char *when, const char *where_file, const char *where_func, int where_line);
extern void _TODO5(int priority, const char *message, const char *where_file, const char *where_func, int where_line);

#endif
