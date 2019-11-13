#include <stdio.h>
#include <string.h>
#include "http.h"

int main(void)
{
    http_t dummy_h;
    printf("Sizeof(http_t): %ld\n", sizeof(dummy_h));
    printf("Sizeof enum: %ld\n", sizeof(dummy_h.type));
    printf("Sizeof(union, struct _req_t): %ld\n", sizeof(dummy_h.req));
    printf("Sizeof(union, struct _res_t): %ld\n", sizeof(dummy_h.res));
    printf("Sizeof(http_header_t*): %ld\n", sizeof(dummy_h.headers));
    printf("Sizeof(u8*): %ld\n", sizeof(dummy_h.msg_body));

    return 0;
}