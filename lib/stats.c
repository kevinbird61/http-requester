#include "stats.h"

stat_t statistics={ 
    .pkt_byte_cnt=0,
    .hdr_size=0,
    .body_size=0,
    .resp_cnt=0
};

void 
stats_inc_code(unsigned char code_enum)
{
    statistics.status_code_detail[code_enum-1]++; // because index start from 1
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
stats_inc_pkt_bytes(u64 size)
{
    statistics.pkt_byte_cnt+=size;
}

void 
stats_inc_hdr_size(u64 size)
{
    statistics.hdr_size+=size;
}

void 
stats_inc_body_size(u64 size)
{
    statistics.body_size+=size;
}

void 
stats_inc_resp_cnt(u64 resp_num)
{
    statistics.resp_cnt+=resp_num;
}

void 
stats_dump()
{
    printf("Statistics==============================================================\n");
    // status code
    printf("└> Status Code:\n");
    for(int i=0; i<5; i++){
        printf("    [%dxx]: %d\n", i+1, statistics.status_code[i]);
        if(verbose){
            int j;
            switch(i){
                case 0:
                    for(j=_100_CONTINUE-1; j<_101_SWITCHING_PROTO; j++){
                        printf("\t[%s]: %-7d", get_http_status_code_by_idx[j+1], statistics.status_code_detail[j]);
                    }
                    printf("\n");
                    break;
                case 1:
                    for(j=_200_OK-1; j<_205_RESET_CONTENT; j++){
                        printf("\t[%s]: %-7d", get_http_status_code_by_idx[j+1], statistics.status_code_detail[j]);
                    }
                    printf("\n");
                    break;
                case 2:
                    for(j=_300_MULTI_CHOICES-1; j<_307_TEMP_REDIRECT; j++){
                        printf("\t[%s]: %-7d", get_http_status_code_by_idx[j+1], statistics.status_code_detail[j]);
                    }
                    printf("\n");
                    break;
                case 3:
                    for(j=_400_BAD_REQUEST-1; j<_426_UPGRADE_REQUIRED; j++){
                        printf("\t[%s]: %-7d", get_http_status_code_by_idx[j+1], statistics.status_code_detail[j]);
                    }
                    printf("\n");
                    break;
                case 4:
                    for(j=_500_INTERNAL_SERV_ERR-1; j<_505_HTTP_VER_NOT_SUPPORTED; j++){
                        printf("\t[%s]: %-7d", get_http_status_code_by_idx[j+1], statistics.status_code_detail[j]);
                    }
                    printf("\n");
                    break;
            }
        }
    } /* status code */
    printf("************************************************************************\n");
    // byte counts
    if(statistics.resp_cnt>0){
        printf("└> Pkts: \n");
        printf("| %-30s | %-30s | %-30s | %-30s | %-30s |\n", 
            "Total received bytes", "Total response pkts", "Avg. bytes per pkt", "Avg. header length", "Avg. body length");
        printf("| %-30lld | %-30lld | %-30lld | %-30lld | %-30lld |\n", 
            statistics.pkt_byte_cnt, statistics.resp_cnt, statistics.pkt_byte_cnt/statistics.resp_cnt
            , statistics.hdr_size/statistics.resp_cnt, statistics.body_size/statistics.resp_cnt);
        printf("************************************************************************\n");
    }
    printf("========================================================================\n");
}
