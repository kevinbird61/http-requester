#include "stats.h"

/** useful box letter symbol: https://theasciicode.com.ar/extended-ascii-code/box-drawing-character-single-line-upper-right-corner-ascii-code-191.html
 * ──, └, └>, ├
*/
stat_t statistics;
stat_t priv_statistics[MAX_THREAD];

void 
stats_init()
{
    memset(&statistics, 0x00, sizeof(stat_t));
    for(int i=0; i<MAX_THREAD; i++){
        memset(&priv_statistics[i], 0x00, sizeof(stat_t));
    }
}

void 
stats_inc_code(u8 thrd_num, unsigned char code_enum)
{
    PRIV_STATS[thrd_num].status_code_detail[code_enum-1]++; // because index start from 1
    switch(code_enum){
        case _100_CONTINUE ... _101_SWITCHING_PROTO:
            PRIV_STATS[thrd_num].status_code[0]++;
            break;
        case _200_OK ... _205_RESET_CONTENT:
            PRIV_STATS[thrd_num].status_code[1]++;
            break;
        case _300_MULTI_CHOICES ... _307_TEMP_REDIRECT:
            PRIV_STATS[thrd_num].status_code[2]++;
            break;
        case _400_BAD_REQUEST ... _426_UPGRADE_REQUIRED:
            PRIV_STATS[thrd_num].status_code[3]++;
            break;
        case _500_INTERNAL_SERV_ERR ... _505_HTTP_VER_NOT_SUPPORTED:
            PRIV_STATS[thrd_num].status_code[4]++;
            break;
    }
}

void 
stats_conn(
    void* cm)
{
    // setup connection manager
    conn_mgnt_t* mgnt=(conn_mgnt_t*) cm;
    // get the condition of each connection
    PRIV_STATS[mgnt->thrd_num].sockets=mgnt->sockets;
    // store the statistics
    PRIV_STATS[mgnt->thrd_num].conn_num=mgnt->args->conc;
    for(int i=0; i<PRIV_STATS[mgnt->thrd_num].conn_num; i++){
        PRIV_STATS[mgnt->thrd_num].retry_conn_num+=mgnt->sockets[i].retry_conn_num;
    }
}

void 
stats_dump()
{
    // sum up all threads' statistics
    for(int i=0; i<MAX_THREAD; i++){
        // status code
        for(int j=0; j<5; j++){
            statistics.status_code[j]+=priv_statistics[i].status_code[j];
        }
        // status code detail
        for(int j=0; j<STATUS_CODE_MAXIMUM; j++){
            statistics.status_code_detail[j]+=priv_statistics[i].status_code_detail[j];
        }
        // pkt,byte counts
        statistics.pkt_byte_cnt+=priv_statistics[i].pkt_byte_cnt;
        statistics.hdr_size+=priv_statistics[i].hdr_size;
        statistics.body_size+=priv_statistics[i].body_size;
        statistics.resp_cnt+=priv_statistics[i].resp_cnt;
        // conn stat
        statistics.conn_num+=priv_statistics[i].conn_num;
        statistics.retry_conn_num+=priv_statistics[i].retry_conn_num;
    }

    printf("Statistics======================================================================\n");
    // status code
    printf("└─> Status Code:\n");
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
    printf("********************************************************************************\n");
    // packet / byte counts
    if(statistics.resp_cnt>0){
        printf("└─> Pkts: \n");
        printf("* %-30s: %lld\n", "Total recevied bytes", statistics.pkt_byte_cnt);
        printf("* %-30s: %lld\n", "Total response pkts", statistics.resp_cnt);
        printf("* %-30s: %lld\n", "Avg. bytes per pkt", statistics.pkt_byte_cnt/statistics.resp_cnt);
        printf("* %-30s: %lld\n", "Avg. header length", statistics.hdr_size/statistics.resp_cnt);
        printf("* %-30s: %lld\n", "Avg. body length", statistics.body_size/statistics.resp_cnt); 
        printf("********************************************************************************\n");
    } /* packet, byte counts */
    printf("********************************************************************************\n");
    // conn state
    if(statistics.conn_num>0){
        printf("└─> Connection state: \n");
        printf("* %-30s: %lld\n", "Number of connections", statistics.conn_num);
        printf("* %-30s: %lld\n", "Number of retry connections", statistics.retry_conn_num);
        printf("* %-30s: %f\n", "Avg. retry (per conn)", statistics.retry_conn_num/(float)statistics.conn_num);
        /** this part will be unavailable under multi-threads mode 
         * (because each thread has its connections, so this part need to re-design)
         * 
        printf("* Each connection status: \n");
        printf("|%-10s|%-10s|%-10s|%-10s|\n", "socket ID", "unsent_req", "sent_req", "recv_resp");
        for(int i=0; i<statistics.conn_num; i++){
            printf("|%-10d|%-10d|%-10d|%-10d|\n", 
                    statistics.sockets[i].sockfd, statistics.sockets[i].unsent_req, 
                    statistics.sockets[i].sent_req, statistics.sockets[i].rcvd_res);
        }*/
    }
    printf("********************************************************************************\n");
    // response time interval
    printf("└─> Response time interval: TODO\n");
    printf("================================================================================\n");
}
