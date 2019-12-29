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
#include "conn_mgnt.h"
#include "argparse.h"
#include "global.h"
#include "http.h"

#define STATS       statistics
#define PRIV_STATS  (priv_statistics)

struct _resp_intvl_t {
    int resp_intvl;
    struct _resp_intvl_t *next;
};

typedef struct _statistics_t {
    /* status code (1xx ~ 5xx) */
    int status_code_detail[STATUS_CODE_MAXIMUM];            // >37 (38)=38*4=152 bytes
    int retry_conn_num;
    int status_code[5];                                     // 5*4=20 bytes
    /* request number */
    u64 sent_bytes;
    u64 sent_reqs;
    /* response number */
    u64 recv_bytes;
    u64 recv_resps;
    u64 hdr_size;
    u64 body_size;        
    /* time */
    u64 total_time;                                         // program execution time
    u64 process_time;                                       // handle response (from "recv" to "finish parsing") 
    /* state */
    struct {
        u8  is_fin: 1,
            reserved: 7;
    };
    /* connection status */
    struct _conn_t *sockets;                                // 8 bytes
    u32 thrd_cnt;                                           // 4 bytes
    int conn_num;                                   
    int workload;                                           
    int max_req_size;
    /* interval */
    u64 resp_intvl_cnt;
    u64 total_resp_intvl_time;
    u64 resp_intvl_max;
    u64 resp_intvl_min;
    u64 resp_intvl_median;
    float avg_resp_intvl_time;
} __attribute__((packed)) stat_t;

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

u8 stats_progress(u32 total_workload);
// dump all statistics
void stats_dump();

/** stats API:
 */
// init 
#define STATS_INIT()                                    stats_init()
// time measurement
#define STATS_TIME_START()                              (STATS.total_time=read_tsc())
#define STATS_TIME_END()                                (STATS.total_time=(read_tsc()-STATS.total_time))
#define STATS_THR_INIT(thrd_num, thrd_id)               (PRIV_STATS[thrd_num].thrd_cnt=thrd_id) /* using thrd_cnt to record thrd_id */
#define STATS_THR_TIME_START(thrd_num)                  (PRIV_STATS[thrd_num].total_time=read_tsc())
#define STATS_THR_TIME_END(thrd_num)                    \
    do {                                                \
        PRIV_STATS[thrd_num].total_time=(read_tsc()-PRIV_STATS[thrd_num].total_time);   \
        PRIV_STATS[thrd_num].is_fin=1;                                                  \
    } while(0)
#define STATS_THR_FIN(thrd_num)                         (PRIV_STATS[thrd_num].is_fin=1;)
#define STATS_PUSH_RESP_INTVL(thrd_num, intvl)          stats_push_resp_intvl(thrd_num, intvl)
#define STATS_INC_PROCESS_TIME(thrd_num, time)          (PRIV_STATS[thrd_num].process_time+=time)
// req
#define STATS_INC_SENT_BYTES(thrd_num, bytes)           (PRIV_STATS[thrd_num].sent_bytes+=bytes)
#define STATS_INC_SENT_REQS(thrd_num, reqs)             (PRIV_STATS[thrd_num].sent_reqs+=reqs)
// resp
#define STATS_INC_CODE(thrd_num, status_code)           stats_inc_code(thrd_num, status_code)
#define STATS_INC_PKT_BYTES(thrd_num, size)             (PRIV_STATS[thrd_num].recv_bytes+=size)
#define STATS_INC_HDR_BYTES(thrd_num, size)             (PRIV_STATS[thrd_num].hdr_size+=size)
#define STATS_INC_BODY_BYTES(thrd_num, size)            (PRIV_STATS[thrd_num].body_size+=size)
#define STATS_INC_RESP_NUM(thrd_num, cnt)               (PRIV_STATS[thrd_num].recv_resps+=cnt)
// sync connection status
#define STATS_CONN(conn_mgnt)                           (stats_conn(conn_mgnt))
// print func
#define STATS_PROGRESS(total_workload)                  stats_progress(total_workload)
#define STATS_DUMP()                                    stats_dump()

#endif
