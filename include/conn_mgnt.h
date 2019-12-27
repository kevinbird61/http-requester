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
    u8                      type; // type of this thread
    struct _conn_mgnt_t     *mgnt;
    // FIXME: shared info
};

// per connection
struct _conn_t {
    state_machine_t *state_m;
    int             sockfd;         // socket fd
    int             unsent_req;     // unfinished req
    int             sent_req;       // unanswered req
    u16             num_gap;        // number of gaps between "sent" and "rcvd"
    int             leftover;
    int             rcvd_res;       // received resp
    int             retry_conn_num; // retry connection number (create a new sockfd)
    int             retry;          // (reserved)
};  

typedef struct _conn_mgnt_t {
    parsed_args_t*  args;       // user's arguments
    u8              thrd_num;   // belong to which thread.
    u8              pipe;       // ON/OFF, enable pipeline or not (default is 0, not enable)
    u16             num_gap;    // number of gaps between "sent" and "rcvd"
    u8              is_wait;    // 
    u32             total_req;  // total # of reqs
    u32             sent_req;   // # of requests have been sent
    u32             rcvd_res;   // # of responses have been received
    struct _conn_t* sockets;
} conn_mgnt_t;

// create 
conn_mgnt_t *create_conn_mgnt(parsed_args_t *args);
conn_mgnt_t *create_conn_mgnt_non_blocking(parsed_args_t *args);
/** main function: 
 *  - run with configuration (manage all connections)
 *  - handle all connections (with different cases)
 *  - retry the unanswer one
 */
// single thread, multiple socket (non-blocking)
int conn_mgnt_run(conn_mgnt_t *this);
int conn_mgnt_run_non_blocking(conn_mgnt_t *this);
// single thread, single socket
int conn_mgnt_run_blocking(conn_mgnt_t *this);
// update args
int conn_mgnt_update_args(parsed_args_t *args);
// update num_gap
int conn_mgnt_update_num_gap(u32 num_gap);

#endif
