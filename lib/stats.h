#ifndef __STATS__
#define __STATS__

/** statistics/counters, provide sysadmin to trace all activities. 
 * - using several results to categorize statistics:
 *      - return code
 *      - status code
 * 
 * - print format:
 *  - connection fail
 *      - [invalid_xxx]: argument error
 *      - ...
 *  - connection establish
 *      - [1xx] international
 *      - [2xx] success
 *      - [3xx] redirect
 *      - [4xx] client error
 *      - [5xx] server error
 */

#include <stdio.h>
// essential deps
#include "http_enum.h"

typedef struct _statistics_t {
    // status code (1xx ~ 5xx)
    int status_code[5];
} stat_t;

extern stat_t statistics;

void stats_inc_code(unsigned char code_enum);
void stats_dump();

// stats for status code
#define STATS_INC_CODE(status_code) stats_inc_code(status_code)
#define STATS_DUMP()                stats_dump()


#endif
