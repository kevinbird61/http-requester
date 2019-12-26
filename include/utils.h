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

void *get_in_addr(struct sockaddr *sa);
u8 get_digits(u64 input);
u8 *itoa(u64 number);
u8  to_lowercase(u8 *str);
u64 gettime(void);
u8 *getdate(void);
char *copy_str_n_times(char *ori, int n_times);
unsigned int get_cpufreq();

static inline unsigned long long read_tsc(void)
{
    unsigned low, high;
    asm volatile("rdtsc":"=a"(low),"=d"(high));
    return ((low)|((unsigned long long)(high)<<32));
}

#endif
