#ifndef __CONN_MGNT__
#define __CONN_MGNT__

#include "types.h"

#define NUM_GAP 5

typedef struct _conn_mgnt_t {
    parsed_args_t*  args;       // user's arguments
    u16             num_gap;    // number of gaps between "sent" and "rcvd"
    u32             sent_req;   // # of requests have been sent
    u32             rcvd_res;   // # of responses have been received
} conn_mgnt_t;

// create 
conn_mgnt_t *create_conn_mgnt(parsed_args_t *args);
// run

// update args

// update num_gap

#endif
