#include "stats.h"

/** useful box letter symbol: https://theasciicode.com.ar/extended-ascii-code/box-drawing-character-single-line-upper-right-corner-ascii-code-191.html
 * ──, └, └>, ├
*/
stat_t statistics;
stat_t priv_statistics[MAX_THREAD];
struct _resp_intvl_t resp_intvl_queue[MAX_THREAD];
static unsigned int cpuFreq;

void 
stats_init()
{
    memset(&statistics, 0x00, sizeof(stat_t));
    statistics.resp_intvl_max=-1;
    statistics.resp_intvl_min=(u64)~0;
    cpuFreq=get_cpufreq();

    for(int i=0; i<g_total_thrd_num; i++){ // g_total_thrd_num under argparse.h
        // priv statistics (for each thread)
        memset(&priv_statistics[i], 0x00, sizeof(stat_t));
        priv_statistics[i].resp_intvl_cnt=0;
        priv_statistics[i].resp_intvl_max=-1;
        priv_statistics[i].resp_intvl_min=(u64)~0;
        // response interval queue
        memset(&resp_intvl_queue[i], 0x00, sizeof(struct _resp_intvl_t));
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
stats_push_resp_intvl(
    u8 thrd_num,
    u64 intvl)
{
    // record intvl count
    priv_statistics[thrd_num].resp_intvl_cnt++;
    priv_statistics[thrd_num].avg_resp_intvl_time=(priv_statistics[thrd_num].avg_resp_intvl_time*(priv_statistics[thrd_num].resp_intvl_cnt-1)+intvl)/(float)priv_statistics[thrd_num].resp_intvl_cnt;
    if((long long)intvl > (long long)priv_statistics[thrd_num].resp_intvl_max){
        priv_statistics[thrd_num].resp_intvl_max=intvl;
    }
    if(intvl < priv_statistics[thrd_num].resp_intvl_min){
        priv_statistics[thrd_num].resp_intvl_min=intvl;
    }

    // push interval into queue
    if(resp_intvl_queue[thrd_num].next==NULL){
        resp_intvl_queue[thrd_num].next=calloc(1, sizeof(struct _resp_intvl_t));
        resp_intvl_queue[thrd_num].next->resp_intvl=intvl;
        resp_intvl_queue[thrd_num].next->next=NULL;
        return;
    }

    struct _resp_intvl_t *head=resp_intvl_queue[thrd_num].next;
    while(head->next!=NULL){
        head=head->next;
    }
    head->next=calloc(1, sizeof(struct _resp_intvl_t));
    head->next->resp_intvl=intvl;
    head->next->next=NULL;
}

void 
stats_conn(
    void* cm)
{
    // setup connection manager
    conn_mgnt_t* mgnt=(conn_mgnt_t*) cm;
    // record thread number
    statistics.thrd_cnt=mgnt->args->thrd;
    // get the condition of each connection
    PRIV_STATS[mgnt->thrd_num].sockets=mgnt->sockets;
    // store the statistics
    PRIV_STATS[mgnt->thrd_num].conn_num=mgnt->args->conc;
    for(int i=0; i<PRIV_STATS[mgnt->thrd_num].conn_num; i++){
        PRIV_STATS[mgnt->thrd_num].retry_conn_num+=mgnt->sockets[i].retry_conn_num;
        PRIV_STATS[mgnt->thrd_num].workload+=mgnt->sockets[i].rcvd_res;
    }
    // get max-request size
    PRIV_STATS[mgnt->thrd_num].max_req_size=mgnt->num_gap;
}

u8 
stats_progress(
    u32 total_workload)
{
    u32 curr_workload=0, num_fin_thrd=0;
    for(int i=0; i<g_total_thrd_num; i++){
        curr_workload+=priv_statistics[i].resp_cnt;
        if(priv_statistics[i].is_fin){
            num_fin_thrd++;
        }
    }
    
    // main thread
    printf("Complete: %-3d%% (%10d reqs), execution time: %-5.2f sec.\n", 
        (curr_workload*100)/total_workload, curr_workload,
        (read_tsc()-STATS.total_time)/(float)cpuFreq);
    // other thread
    for(int i=0; i<g_total_thrd_num; i++){
        printf("└─> (Thrd: %d) Complete: %-3lld%% (%10lld reqs)\n", 
            i, (priv_statistics[i].resp_cnt*100)/(total_workload/g_total_thrd_num), 
            priv_statistics[i].resp_cnt);
    }

    if(num_fin_thrd==g_total_thrd_num){ // all thrds are finished
        return 0;
    } else {
        return 1;
    }
}

void 
stats_dump()
{
    /************************************ sum up all threads' statistics ************************************/
    u64 thread_max_time=0; // for g_verbose 
    for(int i=0; i<g_total_thrd_num; i++){
        // status code
        for(int j=0; j<5; j++){
            statistics.status_code[j]+=priv_statistics[i].status_code[j];
        }
        // status code detail
        for(int j=0; j<STATUS_CODE_MAXIMUM; j++){
            statistics.status_code_detail[j]+=priv_statistics[i].status_code_detail[j];
        }
        // req pkt,byte counts
        statistics.sent_bytes+=priv_statistics[i].sent_bytes;
        statistics.sent_reqs+=priv_statistics[i].sent_reqs;
        // resp pkt,byte counts
        statistics.pkt_byte_cnt+=priv_statistics[i].pkt_byte_cnt;
        statistics.hdr_size+=priv_statistics[i].hdr_size;
        statistics.body_size+=priv_statistics[i].body_size;
        statistics.resp_cnt+=priv_statistics[i].resp_cnt;
        // conn stat
        statistics.conn_num+=priv_statistics[i].conn_num;
        statistics.retry_conn_num+=priv_statistics[i].retry_conn_num;
        statistics.workload+=priv_statistics[i].workload;
        statistics.max_req_size+=priv_statistics[i].max_req_size;
        // process time
        statistics.process_time+=priv_statistics[i].process_time;

        // response time (only enable when user using "single connection")
        if(statistics.conn_num > 0 && statistics.thrd_cnt > 0 ){
            if( (statistics.conn_num/statistics.thrd_cnt)==1 && resp_intvl_queue[i].next!=NULL ){
                statistics.resp_intvl_cnt+=priv_statistics[i].resp_intvl_cnt;
                statistics.avg_resp_intvl_time+=priv_statistics[i].avg_resp_intvl_time;
                // summarize, analyze the response time
                struct _resp_intvl_t *head=resp_intvl_queue[i].next;
                while(head){
                    // printf("Resp. time: %d\n", head->resp_intvl);
                    head=head->next;
                }
                // max, min 
                if((long long)priv_statistics[i].resp_intvl_max > (long long)statistics.resp_intvl_max){
                    statistics.resp_intvl_max=priv_statistics[i].resp_intvl_max;
                }
                if(priv_statistics[i].resp_intvl_min < statistics.resp_intvl_min){
                    statistics.resp_intvl_min=priv_statistics[i].resp_intvl_min;
                }
            }
        }

        // calculate thread's MAX time
        if(priv_statistics[i].total_time > thread_max_time){
            thread_max_time=priv_statistics[i].total_time;
        }
    }

    /************************************ print all statistics ************************************/
    printf("Statistics (using `-v` to enable verbose mode)==================================\n");
    // status code
    printf("└─> Status Code:\n");
    for(int i=0; i<5; i++){
        printf("    [%dxx]: %d\n", i+1, statistics.status_code[i]);
        if(g_verbose){ // only print this ugly detail log when enable verbose mode
            int j;
            switch(i){
                case 0:
                    for(j=_100_CONTINUE-1; j<_101_SWITCHING_PROTO; j++){
                        if(statistics.status_code_detail[j]>0){
                            printf("\t[%s]: %-7d\n", get_http_status_code_by_idx[j+1], statistics.status_code_detail[j]);
                        }
                    }
                    break;
                case 1:
                    for(j=_200_OK-1; j<_205_RESET_CONTENT; j++){
                        if(statistics.status_code_detail[j]>0){
                            printf("\t[%s]: %-7d\n", get_http_status_code_by_idx[j+1], statistics.status_code_detail[j]);
                        }
                    }
                    break;
                case 2:
                    for(j=_300_MULTI_CHOICES-1; j<_307_TEMP_REDIRECT; j++){
                        if(statistics.status_code_detail[j]>0){
                            printf("\t[%s]: %-7d\n", get_http_status_code_by_idx[j+1], statistics.status_code_detail[j]);
                        }
                    }
                    break;
                case 3:
                    for(j=_400_BAD_REQUEST-1; j<_426_UPGRADE_REQUIRED; j++){
                        if(statistics.status_code_detail[j]>0){
                            printf("\t[%s]: %-7d\n", get_http_status_code_by_idx[j+1], statistics.status_code_detail[j]);
                        }
                    }
                    break;
                case 4:
                    for(j=_500_INTERNAL_SERV_ERR-1; j<_505_HTTP_VER_NOT_SUPPORTED; j++){
                        if(statistics.status_code_detail[j]>0){
                            printf("\t[%s]: %-7d\n", get_http_status_code_by_idx[j+1], statistics.status_code_detail[j]);
                        }
                    }
                    break;
            }
        }
    } /* status code */
    printf("********************************************************************************\n");
    // req & resp packet / byte counts
    printf("└─> Request info: \n");
    printf("* %-30s: %lld\n", "Total sent bytes", statistics.sent_bytes);
    printf("* %-30s: %lld\n", "Total sent requests", statistics.sent_reqs);
    if(statistics.resp_cnt>0){
        printf("└─> Response info: \n");
        printf("* %-30s: %lld\n", "Total recevied bytes", statistics.pkt_byte_cnt);
        printf("* %-30s: %lld\n", "Total response pkts", statistics.resp_cnt);
        printf("* %-30s: %lld\n", "Avg. bytes per pkt", statistics.pkt_byte_cnt/statistics.resp_cnt);
        printf("* %-30s: %lld\n", "Avg. header length", statistics.hdr_size/statistics.resp_cnt);
        printf("* %-30s: %lld\n", "Avg. body length", statistics.body_size/statistics.resp_cnt); 
    } /* packet, byte counts */
    // calculate the wasted requests (if sent_reqs > resp_cnt)
    // which means that those requests waste our bandwidth because of the connection close 
    if(statistics.sent_reqs > statistics.resp_cnt){
        printf("└─> Wasted (Unanswered) : \n");
        printf("* %-30s: %lld\n", "Wasted requests", statistics.sent_reqs-statistics.resp_cnt);
        printf("* %-30s: %lld\n", "Wasted bytes (bandwidth)", (statistics.sent_bytes/statistics.sent_reqs)*(statistics.sent_reqs-statistics.resp_cnt));
    }
    printf("********************************************************************************\n");
    // thrd info
    if(statistics.thrd_cnt>0){
        printf("└─> Thread info: \n");
        printf("* %-30s: %d\n", "Number of threads", statistics.thrd_cnt);
        printf("* %-30s: %f\n", "Avg. max-requests(burst len)", statistics.max_req_size/(float)statistics.thrd_cnt);

        // per thrd execution time
        if(g_verbose){ // only print this ugly detail log when enable verbose mode
            printf("└─> Per-thread info (ID + VAL): \n");
            printf("--------------------------------------------------------------------------------\n");
            printf("|%-5s| %-12s | %-12s | %-12s | %-12s | %-12s |\n", " ", 
                "Thread ID", "Time (sec.)", "Workload", "Retry", "Log file");
            printf("--------------------------------------------------------------------------------\n");
            for(int i=0; i<g_total_thrd_num; i++){
                // logfile
                char *logfile=malloc((strlen(g_log_dir)+strlen(g_log_filename)+strlen(g_log_ext)+strlen(itoa(get_thrd_tid_from_id(priv_statistics[i].thrd_cnt)))+1)*sizeof(char));
                sprintf(logfile, "%s%s%d%s", g_log_dir, g_log_filename, get_thrd_tid_from_id(priv_statistics[i].thrd_cnt), g_log_ext);

                printf("|%-5s| %-12d | %-12f | %-12d | %-12d | %-12s |\n", 
                    " ",  get_thrd_tid_from_id(priv_statistics[i].thrd_cnt),  
                    priv_statistics[i].total_time/((float)cpuFreq),
                    priv_statistics[i].workload,
                    priv_statistics[i].retry_conn_num,
                    logfile);
            }
            printf("--------------------------------------------------------------------------------\n");
        }
    }
    printf("********************************************************************************\n");
    // conn state
    if(statistics.conn_num>0){
        printf("└─> Connection state: \n");
        printf("* %-30s: %d\n", "Number of connections", statistics.conn_num);
        printf("* %-30s: %d\n", "Avg. workload per connection", statistics.workload/statistics.conn_num);
        printf("* %-30s: %d\n", "Number of retry connections", statistics.retry_conn_num);
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
    // time
    printf("└─> Time/Rate: \n");
    if(statistics.total_time>0){
        if(g_verbose){ 
            // periodically print out progress will affect the accuracy of time, so use thread's MAX time 
            printf("* %-30s: %-15.2f (sec.)\n", "Total execution time", thread_max_time/(float)cpuFreq );
        } else { // use main thread's end time
            printf("* %-30s: %-15.2f (sec.)\n", "Total execution time", statistics.total_time/(float)cpuFreq );
        }
        printf("* %-30s: %-15.2f (requests/sec.)\n",  "Request rate", statistics.sent_reqs/(statistics.total_time/(float)cpuFreq) );
        printf("* %-30s: %-15.2f (responses/sec.)\n", "Reply rate", statistics.resp_cnt/(statistics.total_time/(float)cpuFreq) );
        printf("* %-30s: %-15.2f (MB/sec.)\n", "Throughput", ((statistics.pkt_byte_cnt+statistics.sent_bytes)/(1024*1024))/(statistics.total_time/(float)cpuFreq) );
    }
    // processing time
    if(statistics.process_time>0){
        printf("* %-30s: %-15.2f (sec./thrd)\n", "Avg. parsing time ", statistics.process_time/((float)cpuFreq*statistics.thrd_cnt));
        printf("* %-30s: %-15.2f (sec./conn)\n", "Avg. parsing time ", statistics.process_time/((float)cpuFreq*statistics.conn_num));
    }
    printf("********************************************************************************\n");
    // response time interval
    printf("└─> Response interval: (Only enable when using `single conn + non-pipeline`)\n");
    if(statistics.resp_intvl_cnt>0){
        printf("* %-30s: %lld\n", "# of response intvl", statistics.resp_intvl_cnt);
        printf("* %-30s: %-15f (sec.)\n", "Avg. response time", statistics.avg_resp_intvl_time/((float)cpuFreq));
        printf("* %-30s: %-15f (sec.)\n", "MAX resp. intvl", statistics.resp_intvl_max/(float)cpuFreq);
        printf("* %-30s: %-15f (sec.)\n", "MIN resp. intvl", statistics.resp_intvl_min/(float)cpuFreq);
    }
    printf("================================================================================\n");
}
