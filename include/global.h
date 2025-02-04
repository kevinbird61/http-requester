#ifndef __GLOBAL__
#define __GLOBAL__

#include <stdio.h>
#include "types.h"

#define MAX_RETRY           (50000)         // wait 5 sec (permit 5 retry time)
#define MAX_THREAD          (1000)          // max thread number
#define MAX_REQS_NUM        (4294967295)    // max requests number
#define RETRY_WAIT_TIME     (1)             // 4xx, or other error, wait 1 sec and create a new conn to retry
#define MAX_NUM_GAP         (10000)
#define NUM_GAP             (100)           // max-request size
#define MIN_NUM_GAP         (1)             // min-request size
#define MAX_SENT_REQ        (1000)          // how many sent_req (unanswered reqs) allow
#define MAX_ARBIT_REQS      (10)            // argparse.c
#define DEC_RATE_NUMERAT    (9)             // decrease rate = (MAX-request size)*(DEC_RATE_NUMERAT)/(DEC_RATE_DENOMIN)
#define DEC_RATE_DENOMIN    (10)            // 
#define POLL_TIMEOUT        (1000)          // 1 sec (normal/start case)
#define POLL_MAX_TIMEOUT    (64000)         // 64 sec (after `127 sec` poll timeout, kb will exit sending process). 

#define NUM_PARAMS          (16)
#define DEFAULT_PORT        (80)
#define DEFAULT_SSL_PORT    (443)
#define __FILENAME__        (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define AGENT               "kb"

#define RECV_BUFF_SCALE     (10)            // scale x chunk_size = TOTAL_RECV_BUFF 
#define CHUNK_SIZE          (6*1024)        //

#define CLEAR_SCREEN()      (printf("\e[1;1H\e[2J"))
#define UPDATE_SCREEN()     (printf("\e[1;1H"))

#define likely(x)           __builtin_expect((x), 1)
#define unlikely(x)         __builtin_expect((x), 0)

extern u32      g_burst_length;             // max-request size
extern u8       g_fast;                     // flag for enable aggressive pipe
extern u8       g_verbose;                  // flag for verbose print
extern int      *g_thrd_id_mapping;         // thrd tid (pthread_t) & thrd num (int) mapping
extern char     *g_program;                 // program name
extern int      g_total_thrd_num;           // store number of available thrds

// log filename
extern char *g_log_dir;
extern char *g_log_filename;
extern char *g_log_ext;
extern FILE *g_logfd[MAX_THREAD];
// log level string
extern char *g_log_level_str[];
// log enable (global)
extern unsigned char g_log_visible;

// mapping from `thrd id` to `thrd num`
static inline int get_thrd_tid_from_id(int thrd_tid){
    for(int i=0; i<g_total_thrd_num; i++){ // linear search
        if(g_thrd_id_mapping[i]==thrd_tid){
            return i;
        }
    }
    return -1; // not found
}

#endif
