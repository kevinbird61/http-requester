#ifndef __UTILS__
#define __UTILS__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static inline void *get_in_addr(struct sockaddr *sa)
{
    if(sa->sa_family==AF_INET){
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
    

#endif