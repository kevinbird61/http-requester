#ifndef __LOGGER__
#define __LOGGER__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include "utils.h"

#define LOG_LEVEL_STR log_level_str

// log filename
static char *log_dir="/tmp/";
static char *log_filename="http_request";
static char *log_ext=".log";
// log level string
extern char *log_level_str[];
// log enable (global)
extern unsigned char log_visible;

/** Record any activities,
 * - design:
 *      - file location: `/tmp/http_requester_<thread-id>.log`
 * - format: 
 *      [@TIME][@LOG_LEVEL][FUNC] Description.
 */
#define LOG(loglevel, format, args...)              \
    do {                                            \
        char *userlog=parse_valist(format, ##args); \
        syslog(loglevel,                            \
            "[%s][%-10s][%s] %s\n",                 \
            getdate(), LOG_LEVEL_STR[loglevel],     \
            __func__,                               \
            userlog);                               \
    } while(0)



typedef enum _log_level {
    /* only display on the screen */
    NORMAL=0,
    INFO,
    SHOWONLY,
    /* log + show */
    WARNING,
    DANGER,
    ERROR,
    /* reserved for log-only enum */
    LOGONLY,
    DEBUG,
    LOG_LEVEL_MAXIMUM /* maximum */
} log_level;


// write the log
int syslog(u8 loglevel, char *info_args, ...);
// turn va_list into char array
char *parse_valist(char *info_args, ...);

#endif