#ifndef __ERR_HANDLER__
#define __ERR_HANDLER__

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>

#include "argparse.h"
#include "conn_mgnt.h"
#include "logger.h"

// handle errno
int poll_err_handler(void *obj);

#endif 
