#include "probe_mgnt.h"

char *common_uri[]={
    "/",
    "/index.html",
    "/test.html",
    "/about.html",
    "/about",
    "/research",
    "/student",
    "/announcement",
    "/resource",
    "/phpmyadmin",
    NULL /* last one must be NULL */
};

int 
probe_mgnt_run(
    probe_mgnt_t *this)
{
    /* simple connection version 
     * - we cannot open too many connections to probe!
     */
    int i=0, num_found=0, num_missed=0;
    char *port=itoa(this->args->port);
    // first, we check common_uri
    while(common_uri[i]!=NULL){
        // pack our uri
        char *probe_req;
        http_req_create_start_line(&probe_req, this->args->method, common_uri[i], HTTP_1_1);
        http_req_obj_ins_header_by_idx(&this->args->http_req, REQ_HOST, this->args->host);
        http_req_obj_ins_header_by_idx(&this->args->http_req, REQ_USER_AGENT, AGENT);
        http_req_finish(&probe_req, this->args->http_req);
        // send + recv (blocking) 
        send(this->probe_fd, probe_req, strlen(probe_req), 0);
        control_var_t control_var;
        control_var=multi_bytes_http_parsing_state_machine(this->state_m, this->probe_fd, 1);
        switch(control_var.rcode){
            case RCODE_REDIRECT: // tread as found now, but need more tags on this kind
            case RCODE_POLL_DATA:
            case RCODE_FIN: // finished
                // not support now
                // puts("Not support redirection now.");
                // all these case we treat as success, record this uri
                printf("Found %s\n", common_uri[i]);
                num_found++;
                break;
            case RCODE_CLOSE: // conn close (should we treat this as success?)
            case RCODE_SERVER_ERR:
            case RCODE_CLIENT_ERR:
            case RCODE_ERROR:
                // this uri doesn't exist, test next one
                // need to create a new connection
                printf("Missed %s\n", common_uri[i]);
                num_missed++;
                usleep(5000);
                break;
            default:
                // other errors (need further investigation)
                break;
        }

        //free(probe_req);
        // need to create a new conn
        close(this->probe_fd);
        this->probe_fd=create_tcp_conn(this->args->host, port);
        reset_parsing_state_machine(this->state_m);
        i++;
    }

    /* TODO: examine with variable-length uri */

end: 
    /* print out all statistics */
    printf("********************************************************************************\n");
    printf("Finished all probing work.\n");
    printf("--------------------------------------------------------------------------------\n");
    printf("%-10s: %d\n", "Found", num_found);
    printf("%-10s: %d\n", "Missed", num_missed);
    printf("********************************************************************************\n");
    return 0;
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