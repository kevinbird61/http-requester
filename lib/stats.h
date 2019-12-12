#ifndef __STATS__
#define __STATS__

/** statistics/counters, provide sysadmin to trace all activities. 
 * collect:
 *  - [ ] connection fail (record failed by which reason)
 *      - [parsing error]: data incomplete
 *      - [disconnect]: server abort the connection
 *      - ...
 *  - [ ] retry time (related to connection failure, MUST conform to RFC7230)
 *      - 
 *  - [ ] connection establish
 *      - [1xx] international
 *      - [2xx] success
 *      - [3xx] redirect
 *      - [4xx] client error
 *      - [5xx] server error
 *  - [ ] response time (like `ab`)
 *      - calculate the interval between "sent request" and "recv response"
 *          - question: how to measure the percise interval time ? (e.g. recv() can be arrived 
 *                      earlier than finish the sending process when pipelining or lots of 
 *                      connection with single thread.)
 * 
 * format: 
 *  "└> <Statistic #1 name> <...> ..."
 *  " %-20d(s) ... "
 */

#include <stdio.h>
// essential deps
#include "http_enum.h"

typedef struct _statistics_t {
    // status code (1xx ~ 5xx)
    int status_code[5];
    // connection status

} stat_t;

extern stat_t statistics;

void stats_inc_code(unsigned char code_enum);
void stats_dump();

// stats for status code
#define STATS_INC_CODE(status_code) stats_inc_code(status_code)
#define STATS_DUMP()                stats_dump()


#endif
