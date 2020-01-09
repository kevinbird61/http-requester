#ifndef __SYSCONFIG__
#define __SYSCONFIG__

#include <sys/resource.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

/* fetch user's configuration, and calculate the limitation of total connections, or other kb's configuration */
static inline int
get_max_num_open_fds(struct rlimit *limit)
{
    // limit.rlim_cur: soft limit
    // limit.rlim_max: hard limit (max. value on this machine)
    if(getrlimit(RLIMIT_NOFILE, limit) != 0) {
        printf("getrlimit() failed with errno = %d\n", errno);
        return -1;
    }

    return 0;
}

static inline int
set_max_num_open_fds(struct rlimit *limit)
{
    if(setrlimit(RLIMIT_NOFILE, limit) != 0) {
        printf("setrlimit() failed with errno = %d\n", errno);
        return -1;
    }
    return 0;
}


#endif 