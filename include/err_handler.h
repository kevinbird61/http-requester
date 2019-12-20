#ifndef __ERR_HANDLER__
#define __ERR_HANDLER__

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include "conn_mgnt.h"

// handle errno
int sock_sent_err_handler(void *obj);
int sock_recv_err_handler();
int poll_err_handler(void *obj);

#endif 
