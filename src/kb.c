#include "signal_handler.h"
#include "conn_mgnt.h"
#include "probe_mgnt.h"

int main(int argc, char *argv[]){
    parsed_args_t *args=create_argparse();
    u8 ret=argparse(&args, argc, argv);

    /* global var init */
    SAVE_ALLOC(g_thrd_id_mapping, args->thrd, int);

    /* select mode (default is loadgen) */
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
    // var
    struct _thrd_t *thrds=NULL;
    u16 num_failed_thrd=0;
    u32 total_req=0, leftover=0;

    // signal handler
    SIG_HANDLE(SIGINT);
    SIG_HANDLE(SIGALRM);
    SIG_HANDLE(SIGSEGV);
    SIG_HANDLE(SIGPIPE);

    // pthread_self() not always return 0, need to maintain a mapping table 
    STATS_INIT(); // init statistics
    STATS_TIME_START();

    // enable multiple threads
    SAVE_ALLOC(thrds, args->thrd, struct _thrd_t);
    
    // distribute the total reqs (e.g. args->conn) to each thread 
    // Notice: args->conn has been modified later, need to store this value first
    total_req=args->reqs;
    leftover=args->reqs%args->thrd; /* FIXME: how to deal with leftover */
    args->reqs=(args->reqs/args->thrd);
    // need to create more threads
    for(int i=0; i<args->thrd; i++){
        LOG_INIT(i);
        thrds[i].num=i;
        if(i==(args->thrd-1)){ // the last thread carry the leftover
            args->reqs+=leftover;
        }
        if(args->use_non_block){
            thrds[i].mgnt=create_conn_mgnt_non_blocking(args);
            thrds[i].mgnt->thrd_num=i;
            if(ret==USE_URL){
                if(pthread_create(&thrds[i].tid, NULL, (void *)&conn_mgnt_run_non_blocking, thrds[i].mgnt) < 0){
                    num_failed_thrd++;
                    perror("Cannot create new thread.");
                    exit(1);
                }
            } else { // not support
                exit(1);
            }
        } else { // 
            thrds[i].mgnt=create_conn_mgnt(args);
            thrds[i].mgnt->thrd_num=i;
            if(ret==USE_URL){
                if(pthread_create(&thrds[i].tid, NULL, (void *)&conn_mgnt_run, thrds[i].mgnt) < 0) {
                    num_failed_thrd++;
                    perror("Cannot create new thread.");
                    exit(1);
                }
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
            usleep(USEC); // print periodically (0.5 sec)
        } while(STATS_PROGRESS(total_req));
        // print user config after finish
        print_config(args);
        // print http request ?
    }

    for(int i=0; i<args->thrd; i++){
        pthread_join(thrds[i].tid, NULL);
        LOG_CLOSE(i);
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
