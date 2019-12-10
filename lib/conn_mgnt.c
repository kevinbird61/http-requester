#include "conn_mgnt.h"

int 
conn_mgnt_run(conn_mgnt_t *this)
{
    /* forge http request header */
    char *http_request;
    // start-line
    http_req_create_start_line(&http_request, this->args->method, this->args->path, HTTP_1_1);
    // header fields
    http_req_obj_ins_header_by_idx(&this->args->http_req, REQ_HOST, this->args->host);
    http_req_obj_ins_header_by_idx(&this->args->http_req, REQ_CONN, ((this->args->conn > 1)? "keep-alive": "close"));
    http_req_obj_ins_header_by_idx(&this->args->http_req, REQ_USER_AGENT, AGENT);
    // finish
    http_req_finish(&http_request, this->args->http_req);
    printf("HTTP request:*******************************************************************\n");
    printf("%s\n", http_request);
    printf("================================================================================\n");
    
    /* create sock */
    int sockfd=create_tcp_conn(this->args->host, itoa(this->args->port));
    if(sockfd<0){
        exit(1);
    }

    // record total connection
    this->total_req=this->args->conn;

    if(this->args->enable_pipe){
        /* support pipeline */
        while(this->rcvd_res<this->total_req){
            // send num_gap in one time, then recv all
            // , check the rest # of reqs: 
            int sent_req=0;
            if(this->sent_req > (this->total_req-this->num_gap)){
                for(int i=0;i<(this->total_req-this->sent_req);i++){
                    send(sockfd, http_request, strlen(http_request), 0);
                }
                /* Problem: when you want to send packet all requests, it will make program hanging */
                // char *total_reqs=copy_str_n_times(http_request, this->total_req-this->sent_req);
                // send(sockfd, total_reqs, strlen(total_reqs), 0);
                sent_req=this->total_req-this->num_gap;
            } else {
                // check if total_req < num_gap
                if(this->total_req > this->num_gap){
                    for(int i=0;i<this->num_gap;i++){
                        send(sockfd, http_request, strlen(http_request), 0);
                    }
                    // char *total_reqs=copy_str_n_times(http_request, this->total_req-this->sent_req);
                    // send(sockfd, total_reqs, strlen(total_reqs), 0);
                    sent_req+=this->num_gap;
                } else {
                    for(int i=0;i<this->total_req;i++){
                        send(sockfd, http_request, strlen(http_request), 0);
                    }
                    // char *total_reqs=copy_str_n_times(http_request, this->total_req);
                    // send(sockfd, total_reqs, strlen(total_reqs), 0);
                    sent_req+=this->total_req;
                }
            }
            printf("Sent: %d\n", sent_req);
            this->sent_req+=sent_req;
            // check connection state first
            // check_tcp_conn_stat(sockfd);
            if(get_tcp_conn_stat(sockfd)==TCP_CLOSE_WAIT){
                // terminate
                exit(1);
            }
            // call recv
            control_var_t *control_var;
            control_var=multi_bytes_http_parsing_state_machine(sockfd, sent_req);
            // TODO: handle control_var
            // - dealing with connection close (open another new connection to send rest reqs) ... 
            this->rcvd_res+=sent_req; // this should be check control_var first (whether if there has any error)
        }
    } else {
        /* not pipeline */
        for(int i=0;i<this->total_req;i++){
            // send one
            send(sockfd, http_request, strlen(http_request), 0);
            // recv one
            control_var_t *control_var;
            control_var=multi_bytes_http_parsing_state_machine(sockfd, 1);
        }
    }
}

conn_mgnt_t *
create_conn_mgnt(
    parsed_args_t *args)
{
    conn_mgnt_t *mgnt=calloc(1, sizeof(conn_mgnt_t));
    mgnt->args=args;
    mgnt->num_gap=NUM_GAP; // default

    return mgnt;
}
