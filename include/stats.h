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
#include "argparse.h"
#include "conn_mgnt.h"
#include "http.h"

#define STATS       statistics
#define PRIV_STATS  (priv_statistics)

typedef struct _statistics_t {
    /* status code (1xx ~ 5xx) */
    int status_code[5];
    int status_code_detail[STATUS_CODE_MAXIMUM];
    /* connection status */
    struct _conn_t *sockets;
    u64 conn_num;
    u64 retry_conn_num;
    /* time */
    u64 process_time;  // handle response (from "recv" to "finish parsing") 
    /* response number */
    u64 pkt_byte_cnt; // bytes counts
    u64 hdr_size;
    u64 body_size;
    u64 resp_cnt; // pkt counts (response)
} stat_t;

/* main statistics (single thread) */
extern stat_t statistics;
extern stat_t priv_statistics[];

// reset/init
void stats_init();
// status code
void stats_inc_code(u8 thrd_num, unsigned char code_enum);
// response time

// all connections statistics - need to call stats_init_sockets first.
void stats_conn(void* cm);  // only available in conn_mgnt class

// dump all statistics
void stats_dump();

// stats call
#define STATS_INIT()                                    stats_init()
#define STATS_INC_CODE(thrd_num, status_code)           stats_inc_code(thrd_num, status_code)
#define STATS_INC_PKT_BYTES(thrd_num, size)             (PRIV_STATS[thrd_num].pkt_byte_cnt+=size)
#define STATS_INC_HDR_BYTES(thrd_num, size)             (PRIV_STATS[thrd_num].hdr_size+=size)
#define STATS_INC_BODY_BYTES(thrd_num, size)            (PRIV_STATS[thrd_num].body_size+=size)
#define STATS_INC_RESP_NUM(thrd_num, cnt)               (PRIV_STATS[thrd_num].resp_cnt+=cnt)
#define STATS_CONN(conn_mgnt)                           (stats_conn(conn_mgnt))
#define STATS_DUMP()                                    stats_dump()


#endif
