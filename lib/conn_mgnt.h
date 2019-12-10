#ifndef __CONN_MGNT__
#define __CONN_MGNT__

#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>
#include "argparse.h"
#include "types.h"
#include "conn.h"
#include "http.h"

#define MAX_RETRY       (5)
#define NUM_GAP         (500)

struct _conn_t {
    int             sockfd; 
    int             unsent_req; 
    int             sent_req;
    int             rcvd_res;
    int             retry;
};  

typedef struct _conn_mgnt_t {
    parsed_args_t*  args;       // user's arguments
    u8              pipe;       // ON/OFF, enable pipeline or not (default is 0, not enable)
    u16             num_gap;    // number of gaps between "sent" and "rcvd"
    u32             total_req;  // total # of reqs
    u32             sent_req;   // # of requests have been sent
    u32             rcvd_res;   // # of responses have been received
    struct _conn_t* sockets;
} conn_mgnt_t;

// create 
conn_mgnt_t *create_conn_mgnt(parsed_args_t *args);
/** main function: 
 *  - run with configuration (manage all connections)
 *  - handle all connections (with different cases)
 *  - retry the unanswer one
 */
int conn_mgnt_run(conn_mgnt_t *this);
// update args
int conn_mgnt_update_args(parsed_args_t *args);
// update num_gap
int conn_mgnt_update_num_gap(u32 num_gap);

#endif
