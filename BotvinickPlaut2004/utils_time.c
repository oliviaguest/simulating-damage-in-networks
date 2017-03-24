
#include <sys/stat.h>
#include <sys/resource.h>

double usertime() /* Return total milliseconds of user time */
{
    struct rusage resource_usage;
    struct timeval *user_time = &(resource_usage.ru_utime);
    
    getrusage(RUSAGE_SELF, &resource_usage);
    return(1000.0 * user_time->tv_sec + user_time->tv_usec/1000.0);
}
 
double systime()  /* Return total milliseconds of system time */
{
    struct rusage resource_usage;
    struct timeval *sys_time = &(resource_usage.ru_stime);
    
    getrusage(RUSAGE_SELF, &resource_usage);
    return(1000.0 * sys_time->tv_sec + sys_time->tv_usec/1000.0);
}

/******************************************************************************/
