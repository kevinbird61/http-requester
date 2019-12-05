#ifndef __CONN__
#define __CONN__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/tcp.h> // TCP_*

/* create connections */
#include "utils.h"

// return socket descriptor
int create_tcp_conn(const char *target, const char *port); 
int create_tcp_keepalive_conn(const char *target, const char *port);

#endif
