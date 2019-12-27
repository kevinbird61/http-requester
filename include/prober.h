#ifndef __PROBER__
#define __PROBER__

/** Send probe requests to examine the available resource of target server
*/
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

extern char *common_uri[]; // popular, common uri

// probe management
typedef struct _probe_mgnt_t {
    parsed_args_t*      args;
    state_machine_t*    state_m;
    int                 probe_fd;
    char *              found_uri;
    u8                  thrd_num;
} probe_mgnt_t;

// int prepare_dict(); // prepare pre-defined resource list (dictionary)
probe_mgnt_t *create_probe_mgnt(parsed_args_t *args);
int probe_mgnt_run(probe_mgnt_t *this);

#endif