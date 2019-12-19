#ifndef __ERR_HANDLER__
#define __ERR_HANDLER__

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>

// handle errno
int sock_sent_err_handler();
int sock_recv_err_handler();

#endif 
