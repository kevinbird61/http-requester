#ifndef __CONN_MGNT__
#define __CONN_MGNT__

#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/poll.h>
#include "err_handler.h"
#include "argparse.h"
#include "global.h"
#include "types.h"
#include "conn.h"
#include "http.h"

// per thread info
struct _thrd_t {
    pthread_t               tid;
    int                     num;
    struct _conn_mgnt_t     *mgnt;
};

// per connection
struct _conn_t {
    state_machine_t *state_m;
    int             sockfd;         // socket fd
    int             unsent_req;     // unfinished req
    int             sent_req;       // unanswered req
    int             rcvd_res;       // received resp
    struct {
        u32         leftover: 8,    // in non-blocking sent with pipelining, we might sent an incompleted requests, so we need to sent out the rest to prevent error of 400 bad requests
                    num_gap: 24;    // currrent allowed burst size 
        u32         retry_conn_num: 24,
                    retry: 8;
    };
} __attribute__((packed));  

typedef struct _conn_mgnt_t {
    struct _conn_t* sockets;
    parsed_args_t*  args;       // user's arguments
    struct {
        u32         thrd_num: 8,    // belong to which thread.
                    num_gap: 24;    // number of gaps between "sent" and "rcvd"
    };
    u32             total_req;  // total # of reqs
    struct {
        u8          pipe:1,     // ON/OFF, enable pipeline or not (default is 0, not enable)
                    is_wait:1,  // FIXME: enhance the waiting mechanism of retry connection
                    reserved: 6;
    };
}  __attribute__((packed)) conn_mgnt_t;


/** main function: (multiple threads, multiple sockets)
 *  - run with configuration (manage all connections)
 *  - handle all connections (with different cases)
 *  - retry the unanswer one
 */
// create 
conn_mgnt_t *create_conn_mgnt_non_blocking(parsed_args_t *args);
conn_mgnt_t *create_conn_mgnt(parsed_args_t *args);
// runner
int conn_mgnt_run_non_blocking(conn_mgnt_t *this); // full non-blocking (connect, send, recv)
int conn_mgnt_run(conn_mgnt_t *this); // hybrid mode (non-blocking recv + blocking send)

#endif
