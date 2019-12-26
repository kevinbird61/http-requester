#ifndef __LOGGER__
#define __LOGGER__

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include "global.h"
#include "utils.h"

#define LOG_LEVEL_STR g_log_level_str

/** Record any activities,
 * - design:
 *      - file location: `/tmp/http_requester_<thread-id>.log`
 * - format: 
 *      [@TIME][@LOG_LEVEL][FUNC] Description.
 *
 *  log_visible: avoid logging to have better performance
 */
typedef enum _log_level {
    /* in this level, only print this  */
    KB_DEBUG=0,     // developer only
    KB=1,           // kb's uncategorized
    KB_EH,          // error handler
    KB_CM,          // connection management
    KB_SM,          // parsing state machine
    KB_PS,          // http resp. parser
    LOG_ALL=99,     // if log_visible > LOG_ALL, then we will print all
    LOGONLY,        // reserved for log-only enum 
    LOG_LEVEL_MAXIMUM /* maximum */
} log_level;


#define LOG(loglevel, format, args...)                  \
    do {                                                \
    /* only record specified level  */                  \
    if(g_log_visible < LOG_ALL){                          \
        if(loglevel == g_log_visible){                    \
            char *userlog=parse_valist(format, ##args); \
            syslog(loglevel,                            \
                pthread_self(),                         \
                "[%s][%-15s][%s] %s\n",                 \
                getdate(),                              \
                LOG_LEVEL_STR[loglevel],                \
                __func__,                               \
                userlog);                               \
        }                                               \
    } else {                                            \
        char *userlog=parse_valist(format, ##args);     \
        syslog(loglevel,                                \
            pthread_self(),                             \
            "[%s][%-15s][%s] %s\n",                     \
            getdate(),                                  \
            LOG_LEVEL_STR[loglevel],                    \
            __func__,                                   \
            userlog);                                   \
    }                                                   \
    } while(0)

// write the log
int syslog(u8 loglevel, u64 thread_id, char *info_args, ...);
// turn va_list into char array
char *parse_valist(char *info_args, ...);

#endif
