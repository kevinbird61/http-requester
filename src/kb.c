#include "signal_handler.h"
#include "conn_mgnt.h"
#include "probe_mgnt.h"

int main(int argc, char *argv[]){
    parsed_args_t *args=create_argparse();
    u8 ret=argparse(&args, argc, argv);

    /* global var init */
    g_thrd_id_mapping=calloc(args->thrd, sizeof(int));
    /* select mode */
    if(args->use_probe_mode){
        goto kb_probe_mode;
    } else {
        goto kb_loadgen;
    }

kb_probe_mode:{
    g_thrd_id_mapping[0]=(int)pthread_self(); // setup id mapping (enable logging)
    probe_mgnt_t *prober=create_probe_mgnt(args);
    probe_mgnt_run(prober);

    return 0;
} /* kb_probe_mode */

kb_loadgen:{
    // signal handler
    SIG_HANDLE(SIGINT);
    SIG_HANDLE(SIGALRM);
    SIG_HANDLE(SIGSEGV);
    SIG_HANDLE(SIGPIPE);

    // pthread_self() not always return 0, need to maintain a mapping table 
    STATS_INIT(); // init statistics
    STATS_TIME_START();

    // enable multiple threads
    struct _thrd_t* thrds=malloc((args->thrd)*sizeof(struct _thrd_t));
    // distribute the total reqs (e.g. args->conn) to each thread 
    // Notice: args->conn has been modified later, need to store this value first
    u32 total_req=args->reqs;
    int leftover=args->reqs%args->thrd; /* FIXME: how to deal with leftover */
    args->reqs=(args->reqs/args->thrd);
    // need to create more threads
    for(int i=0; i<args->thrd; i++){
        thrds[i].num=i;
        if(i==(args->thrd-1)){ // the last thread carry the leftover
            args->reqs+=leftover;
        }
        if(args->use_non_block){
            thrds[i].mgnt=create_conn_mgnt_non_blocking(args);
            thrds[i].mgnt->thrd_num=i;
            if(ret==USE_URL){
                int rc=pthread_create(&thrds[i].tid, NULL, (void *)&conn_mgnt_run_non_blocking, thrds[i].mgnt);
            } else { // not support
                exit(1);
            }
            
        } else { // 
            thrds[i].mgnt=create_conn_mgnt(args);
            thrds[i].mgnt->thrd_num=i;
            if(ret==USE_URL){
                int rc=pthread_create(&thrds[i].tid, NULL, (void *)&conn_mgnt_run, thrds[i].mgnt);
            } else { // not support
                exit(1);
            }
        }

        g_thrd_id_mapping[i]=(int)thrds[i].tid; // setup id mapping
        STATS_THR_INIT(i, (u32)thrds[i].tid);
        // these protected obj can be handled when exception occur (e.g. SIGXXX) and close them elegantly
        SIG_PROTECT_CM(thrds[i].mgnt);
    }

    if(g_verbose){ // only enable when user specify `-v`
        CLEAR_SCREEN();
        do {
            UPDATE_SCREEN();
            usleep(500000); // print periodically (0.5 sec)
        } while(STATS_PROGRESS(total_req));
        // print user config after finish
        print_config(args);
        // print http request ?
    }

    for(int i=0; i<args->thrd; i++){
        pthread_join(thrds[i].tid, NULL);
    }
    
    // t_end
    STATS_TIME_END();
    // dump the statistics
    STATS_DUMP();
    free(thrds);
    free(args);
    return 0;
} /* kb_loadgen */
} /* main() */
