#include "conn_mgnt.h"

int main(int argc, char *argv[]){
    parsed_args_t *args=create_argparse();
    u8 ret=argparse(&args, argc, argv);

    STATS_INIT(); // init statistics
    conn_mgnt_t *conn_mgnt=create_conn_mgnt(args);
    if(ret==USE_URL){
        conn_mgnt_run(conn_mgnt);
    } /* USE_URL */

    // dump the statistics
    STATS_DUMP();
    return 0;
}
