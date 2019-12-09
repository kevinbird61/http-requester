#include "stats.h"

stat_t statistics;

void 
stats_inc_code(unsigned char code_enum)
{
    switch(code_enum){
        case _100_CONTINUE ... _101_SWITCHING_PROTO:
            statistics.status_code[0]++;
            break;
        case _200_OK ... _205_RESET_CONTENT:
            statistics.status_code[1]++;
            break;
        case _300_MULTI_CHOICES ... _307_TEMP_REDIRECT:
            statistics.status_code[2]++;
            break;
        case _400_BAD_REQUEST ... _426_UPGRADE_REQUIRED:
            statistics.status_code[3]++;
            break;
        case _500_INTERNAL_SERV_ERR ... _505_HTTP_VER_NOT_SUPPORTED:
            statistics.status_code[4]++;
            break;
    }
}

void 
stats_dump()
{
    printf("Statistics==============================================================\n");
    // status code
    printf("└> Status Code:\n");
    for(int i=0; i<5; i++){
        printf("    [%dxx]: %d\n", i+1, statistics.status_code[i]);
    }
    printf("========================================================================\n");
}
