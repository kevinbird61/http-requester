#include "prober.h"

int 
probe_mgnt_run(
    probe_mgnt_t *this)
{
    /* simple connection version (we cannot open too many connections to probe!) */
    
    /*  */
}

probe_mgnt_t *
create_probe_mgnt(
    parsed_args_t *args)
{
    probe_mgnt_t *mgnt=calloc(1, sizeof(probe_mgnt_t));
    mgnt->args=args;
    mgnt->probe_fd = create_tcp_conn(args->host, itoa(args->port));
    mgnt->state_m = create_parsing_state_machine();
    reset_parsing_state_machine(mgnt->state_m);

    return mgnt;
}