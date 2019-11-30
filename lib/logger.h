#ifndef __LOGGER__
#define __LOGGER__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include "utils.h"

#define LOG_LEVEL_STR log_level_str

#define LOG(loglevel, format, args...)                  \
    do {                                                \
        char *userlog=parse_valist(format, ##args);     \
        syslog("[%s][%-10s][%s] %s\n",                  \
            getdate(), LOG_LEVEL_STR[loglevel],         \
            __func__,                                   \
            userlog);                                   \
    } while(0)

// log filename
static char *log_dir="/tmp/";
static char *log_filename="http_request";
static char *log_ext=".log";
// log level string
extern char *log_level_str[];

typedef enum _log_level {
    NORMAL=0,
    DEBUG,
    INFO,
    WARNING,
    DANGER,
    ERROR,
    LOG_LEVEL_MAXIMUM
} log_level;

/** Record any activities,
 * - design:
 *      - file location: `/tmp/http_requester_<thread-id>.log`
 * - format: 
 *      [@TIME][@LOG_LEVEL][FUNC] Description.
 */
// write the log
int syslog(char *info_args, ...);
// turn va_list into char array
char *parse_valist(char *info_args, ...);

#endif