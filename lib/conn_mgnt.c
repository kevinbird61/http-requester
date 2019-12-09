#include "conn_mgnt.h"




conn_mgnt_t *
create_conn_mgnt(
    parsed_args_t *args)
{
    conn_mgnt_t *mgnt=calloc(1, sizeof(conn_mgnt_t));
    mgnt->args=args;
    mgnt->num_gap=NUM_GAP; // default

    return mgnt;
}
