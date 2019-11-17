#include "utils.h"

void *get_in_addr(struct sockaddr *sa)
{
    if(sa->sa_family==AF_INET){
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

u8 get_digits(u64 input)
{
    int num_digits=0;
    while(input>0){
        input/=10;
        num_digits++;
    }
    return num_digits;
}

u8 *itoa(u64 number)
{
    char *str=malloc((get_digits(number)+1)*sizeof(char));
    sprintf(str, "%lld", number);
    return str;
}

u64 gettime(void)
{
    struct timezone tz;
    struct timeval tvstart;
    gettimeofday(&tvstart, &tz);
    return ((tvstart.tv_sec)*USEC)+(tvstart.tv_usec);
}

u8 *getdate(void)
{
    char *date=malloc(32*sizeof(char));
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(date, "%04d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1,tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    return date;
}