#ifndef __UTILS__
#define __UTILS__

#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "types.h"

#define USEC    1000000

static inline void *get_in_addr(struct sockaddr *sa)
{
    if(sa->sa_family==AF_INET){
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
    
static inline u64 gettime(void)
{
    struct timezone tz;
    struct timeval tvstart;
    gettimeofday(&tvstart, &tz);
    return ((tvstart.tv_sec)*USEC)+(tvstart.tv_usec);
}

static inline u8 *getdate(void)
{
    char *date=malloc(32*sizeof(char));
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(date, "%4d-%2d-%2d %2d:%2d:%2d", tm.tm_year + 1900, tm.tm_mon + 1,tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    return date;
}

#endif