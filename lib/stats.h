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

typedef struct _statistics_t {
    /* status code (1xx ~ 5xx) */
    int status_code[5];
    int status_code_detail[STATUS_CODE_MAXIMUM];
    /* connection status */
    struct _conn_t *sockets;
    /* response number */
    u64 pkt_byte_cnt; // bytes counts
    u64 hdr_size;
    u64 body_size;
    u64 resp_cnt; // pkt counts (response)
} stat_t;

/* main statistics (single thread) */
extern stat_t statistics;

// status code
void stats_inc_code(unsigned char code_enum);
// pkt, byte counts
void stats_inc_pkt_bytes(u64 size);
void stats_inc_hdr_size(u64 size);
void stats_inc_body_size(u64 size);
void stats_inc_resp_cnt(u64 resp_num);
// response time

// retransmit (connection fail)

// all connections statistics - need to call stats_init_sockets first.
void stats_set_conn(conn_mgnt_t* cm);  // only available in conn_mgnt class

// dump all statistics
void stats_dump();

// stats for status code
#define STATS_INC_CODE(status_code)         stats_inc_code(status_code)
#define STATS_INC_PKT_BYTES(size)           stats_inc_pkt_bytes(size)
#define STATS_INC_HDR_BYTES(size)           stats_inc_hdr_size(size)
#define STATS_INC_BODY_BYTES(size)          stats_inc_body_size(size)
#define STATS_INC_RESP_NUM(cnt)             stats_inc_resp_cnt(cnt)

#define STATS_DUMP()                        stats_dump()


#endif
