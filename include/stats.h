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

struct _resp_intvl_t {
    int resp_intvl;
    struct _resp_intvl_t *next;
};

typedef struct _statistics_t {
    /* status code (1xx ~ 5xx) */
    int status_code[5];
    int status_code_detail[STATUS_CODE_MAXIMUM];
    /* connection status */
    struct _conn_t *sockets;
    int thrd_cnt; 
    int conn_num;
    int retry_conn_num;
    int workload;
    /* time */
    u64 total_time;                                         // program execution time
    u64 process_time;                                       // handle response (from "recv" to "finish parsing") 
    u64 resp_intvl_cnt;
    u64 total_resp_intvl_time;
    u64 resp_intvl_max;
    u64 resp_intvl_min;
    u64 resp_intvl_median;
    float avg_resp_intvl_time;
    /* request number */
    u64 sent_bytes;
    u64 sent_reqs;
    /* response number */
    u64 pkt_byte_cnt;                                       // bytes counts
    u64 hdr_size;
    u64 body_size;
    u64 resp_cnt;                                           // pkt counts (response)
} stat_t;

/* main statistics (single thread) */
extern struct _resp_intvl_t resp_intvl_queue[];             // record all the response interval (only available when using single connection, non-pipeline mode)
extern stat_t statistics;
extern stat_t priv_statistics[];
static unsigned int cpuFreq;

// reset/init
void stats_init();
// status code
void stats_inc_code(u8 thrd_num, unsigned char code_enum);
// response time
void stats_push_resp_intvl(u8 thrd_num, u64 intvl);
// all connections statistics - need to call stats_init_sockets first.
void stats_conn(void* cm);  // only available in conn_mgnt class

// dump all statistics
void stats_dump();

/** stats API:
 */
// init 
#define STATS_INIT()                                    stats_init()
// time measurement
#define STATS_TIME_START()                              (STATS.total_time=read_tsc())
#define STATS_TIME_END()                                (STATS.total_time=(read_tsc()-STATS.total_time))
#define STATS_PUSH_RESP_INTVL(thrd_num, intvl)          stats_push_resp_intvl(thrd_num, intvl)
#define STATS_INC_PROCESS_TIME(thrd_num, time)          (PRIV_STATS[thrd_num].process_time+=time)
// req
#define STATS_INC_SENT_BYTES(thrd_num, bytes)           (PRIV_STATS[thrd_num].sent_bytes+=bytes)
#define STATS_INC_SENT_REQS(thrd_num, reqs)             (PRIV_STATS[thrd_num].sent_reqs+=reqs)
// resp
#define STATS_INC_CODE(thrd_num, status_code)           stats_inc_code(thrd_num, status_code)
#define STATS_INC_PKT_BYTES(thrd_num, size)             (PRIV_STATS[thrd_num].pkt_byte_cnt+=size)
#define STATS_INC_HDR_BYTES(thrd_num, size)             (PRIV_STATS[thrd_num].hdr_size+=size)
#define STATS_INC_BODY_BYTES(thrd_num, size)            (PRIV_STATS[thrd_num].body_size+=size)
#define STATS_INC_RESP_NUM(thrd_num, cnt)               (PRIV_STATS[thrd_num].resp_cnt+=cnt)
// sync connection status
#define STATS_CONN(conn_mgnt)                           (stats_conn(conn_mgnt))
// print func
#define STATS_DUMP()                                    stats_dump()

#endif
