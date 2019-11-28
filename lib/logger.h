#ifndef __LOGGER__
#define __LOGGER__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include "utils.h"

/** Record any activities from http-goon,
 * - design:
 *      - syslog: single thread / psyslog: multi threads
 *      - file location: `/tmp/http_requester_<thread-id>.log`
 *          - thread_id will be 0 if using single thread.
 * - format: 
 *      [@TIME][@ACTION_TYPE][FUNC] Description.
 */

static char *log_dir="/tmp/";
static char *log_filename="http_request";
static char *log_ext=".log";

int logger(char *info_args, ...);
int syslog(const char *action_type, const char *func_name, char *info_args, ...);
int syslog_simple(const char *action_type, const char *func_name, char *info_arg);
// int psyslog(int thread_id, char *info_args, ...);

#endif