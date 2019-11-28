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
        syslog("[%s][%-10s][%s] %s\n",                      \
            getdate(), LOG_LEVEL_STR[loglevel],         \
            __func__,                                   \
            userlog);                                   \
    } while(0)

/** Record any activities from http-goon,
 * - design:
 *      - file location: `/tmp/http_requester_<thread-id>.log`
 *          - thread_id will be 0 if using single thread.
 * - format: 
 *      [@TIME][@LOG_LEVEL][FUNC] Description.
 */

static char *log_dir="/tmp/";
static char *log_filename="http_request";
static char *log_ext=".log";

typedef enum _log_level {
    NORMAL=0,
    DEBUG,
    INFO,
    WARNING,
    DANGER,
    ERROR,
    LOG_LEVEL_MAXIMUM
} log_level;

extern char *log_level_str[];

int syslog(char *info_args, ...);
char *parse_valist(char *info_args, ...);

#endif