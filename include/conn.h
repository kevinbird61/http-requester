#ifndef __CONN__
#define __CONN__

#include <netinet/tcp.h> // TCP_*
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>

/* create connections */
#include "logger.h"
#include "utils.h"

extern char *tcpi_state_str[];

/* create socket and return its descriptor */
int create_tcp_conn(const char *target, const char *port); 
int create_tcp_conn_non_blocking(const char *target, const char *port); 
int create_tcp_keepalive_conn(const char *target, const char *port, int keepcnt, int keepidle, int keepintvl);

/* check socket status */
int check_tcp_conn_stat(int sockfd);
int get_tcp_conn_stat(int sockfd);

#endif
