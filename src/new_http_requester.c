#include "conn_mgnt.h"

int main(int argc, char *argv[]){
    parsed_args_t *args=create_argparse();
    u8 ret=argparse(&args, argc, argv);
    
    // pthread_self() not always return 0, need to maintain a mapping table 
    STATS_INIT(); // init statistics

    // enable multiple threads
    if(args->thrd){
        struct _thrd_t* thrds=malloc((args->thrd)*sizeof(struct _thrd_t));
        // need to create more threads
        for(int i=0; i<args->thrd; i++){
            thrds[i].num=i;
            thrds[i].mgnt=create_conn_mgnt(args);
            thrds[i].mgnt->thrd_num=i;
            if(ret==USE_URL){
                int rc=pthread_create(&thrds[i].tid, NULL, (void *)&conn_mgnt_run, thrds[i].mgnt);
            }
        }
        for(int i=0; i<args->thrd; i++){
            pthread_join(thrds[i].tid, NULL);
        }
    }
    /*conn_mgnt_t *conn_mgnt=create_conn_mgnt(args);
    if(ret==USE_URL){
        conn_mgnt_run(conn_mgnt);
    } */

    // dump the statistics
    STATS_DUMP();
    return 0;
}
