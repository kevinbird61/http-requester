#ifndef __UTILS__
#define __UTILS__

#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "types.h"

#define USEC    1000000

void *get_in_addr(struct sockaddr *sa);
u8 get_digits(u64 input);
u8 *itoa(u64 number);
u8  to_lowercase(u8 *str);
u64 gettime(void);
u8 *getdate(void);
char *copy_str_n_times(char *ori, int n_times);

#endif