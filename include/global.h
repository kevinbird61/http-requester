#ifndef __GLOBAL__
#define __GLOBAL__

#include "types.h"

#define MAX_RETRY           (5)             // wait 5 sec (permit 5 retry time)
#define RETRY_WAIT_TIME     (1)             // 4xx, or other error, wait 1 sec and create a new conn to retry
#define NUM_GAP             (100)           // max-request size
#define MIN_NUM_GAP         (5)             // min-request size
#define MAX_SENT_REQ        (1000)          // how many sent_req (unanswered reqs) allow
#define DEC_RATE_NUMERAT    (9)             // decrease rate = (MAX-request size)*(DEC_RATE_NUMERAT)/(DEC_RATE_DENOMIN)
#define DEC_RATE_DENOMIN    (10)            // 
#define POLL_TIMEOUT        (1000)          // 1 sec (normal/start case)
#define POLL_MAX_TIMEOUT    (16000)         // 16 sec (will exit sending process)

#define RECV_BUFF_SCALE     (10)            // scale x chunk_size = TOTAL_RECV_BUFF 
#define CHUNK_SIZE          (8*1024)        //

#define CLEAR_SCREEN()      (printf("\e[1;1H\e[2J"))

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