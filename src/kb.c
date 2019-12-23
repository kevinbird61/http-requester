#include "signal_handler.h"
#include "conn_mgnt.h"

int main(int argc, char *argv[]){
    parsed_args_t *args=create_argparse();
    u8 ret=argparse(&args, argc, argv);

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
    int leftover=args->conn%args->thrd; /* FIXME: how to deal with leftover */
    args->conn=(args->conn/args->thrd);
    // need to create more threads
    for(int i=0; i<args->thrd; i++){
        thrds[i].num=i;
        if(i==(args->thrd-1)){ // the last thread carry the leftover
            args->conn+=leftover;
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
        // these protected obj can be handled when exception occur (e.g. SIGXXX) and close them elegantly
        SIG_PROTECT_CM(thrds[i].mgnt);
    }

    for(int i=0; i<args->thrd; i++){
        pthread_join(thrds[i].tid, NULL);
    }
    // t_end
    STATS_TIME_END();
    // dump the statistics
    STATS_DUMP();

    free(args);
    free(thrds);

    return 0;
}
