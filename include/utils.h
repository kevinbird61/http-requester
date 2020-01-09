#ifndef __UTILS__
#define __UTILS__

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

#include "types.h"

#define USEC    1000000
#define SAVE_ALLOC(ptr, num, type)      \
    do {                                \
        ptr = malloc(num*sizeof(type)); \
        if(ptr==NULL){                          \
            perror("Cannot allocate memory");   \
            exit(1);                            \
        }                               \
        memset(ptr, 0x00, num);         \
    } while(0)

static inline void *
get_in_addr(struct sockaddr *sa)
{
    if(sa->sa_family==AF_INET){
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

static inline u8 
get_digits(u64 input)
{
    int num_digits=0;
    while(input>0){
        input/=10;
        num_digits++;
    }
    return num_digits;
}

static inline u8 *
itoa(u64 number)
{
    char *str=malloc((get_digits(number)+1)*sizeof(char));
    if(str==NULL){
        perror("Cannot allocate memory for thread information.");
        exit(1);
    }
    sprintf(str, "%lld", number);
    return str;
}

static inline u8 
to_lowercase(u8 *str)
{
    for(int i=0; i<strlen(str); i++){
        if((str[i]>=65 && str[i]<=90) || (str[i]>=97 && str[i]<=122)){
            str[i]=(str[i]<=90? str[i]+32 : str[i]);
        }
    }
}

static inline u64 
gettime(void)
{
    struct timezone tz;
    struct timeval tvstart;
    gettimeofday(&tvstart, &tz);
    return ((tvstart.tv_sec)*USEC)+(tvstart.tv_usec);
}

static inline u8 *
getdate(void)
{
    char *date=malloc(32*sizeof(char));
    if(date==NULL){
        perror("Cannot allocate memory for thread information.");
        exit(1);
    }
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(date, "%04d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1,tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    return date;
}

static inline char *
copy_str_n_times(char *ori, int n_times)
{
    char *total=malloc((n_times+1)*(strlen(ori)));
    if(total==NULL){
        perror("Cannot allocate memory for thread information.");
        exit(1);
    }
    sprintf(total, "%s", ori);
    for(int i=1;i<n_times;i++){
        sprintf(total+strlen(total), "%s", ori);
    }
    return total;
}

static inline unsigned long long read_tsc(void)
{
    unsigned low, high;
    asm volatile("rdtsc":"=a"(low),"=d"(high));
    return ((low)|((unsigned long long)(high)<<32));
}

static inline unsigned int 
get_cpufreq()
{
    struct timezone tz;
    struct timeval tvstart, tvstop;
    unsigned long long int cycles[2];
    unsigned long microseconds;
    unsigned int mhz;
    memset(&tz, 0, sizeof(tz));
    cycles[0]=read_tsc();
    gettimeofday(&tvstart, &tz);
    usleep(25000);
    cycles[1]=read_tsc();
    gettimeofday(&tvstop, &tz);                                             
    microseconds = ((tvstop.tv_sec-tvstart.tv_sec)*USEC) + (tvstop.tv_usec-tvstart.tv_usec);                                                 
    mhz = (unsigned int) (cycles[1]-cycles[0]) / (microseconds);
    return mhz*USEC;
}

#endif
