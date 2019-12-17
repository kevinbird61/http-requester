#include "conn_mgnt.h"

// global init
u32 burst_length=NUM_GAP;
u8  fast=0; // default is false

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
    
    char *packed_req=NULL;
    int packed_len=0;
    // timeout
    int timeout=500;

    // init our state machine
    state_machine_t *state_m=create_parsing_state_machine();
    // record total connection
    this->total_req=this->args->conn;

    if(this->args->enable_pipe){
        /* support pipeline */
        // using poll()
        struct pollfd ufds[this->args->conc];
        for(int i=0;i<this->args->conc;i++){
            ufds[i].fd=this->sockets[i].sockfd;
            ufds[i].events = POLLIN;
        }

        // each sockfd check its workload, and send its request 
        int all_fin=0;
        // if unfinished, keep send & recv
        while(all_fin<this->total_req){
            // printf("Work status(%d/%d)\n", all_fin, this->total_req);
            u32 workload=0; // in each iteration
            // each socket send num_gap at one time
            for(int i=0;i<this->args->conc;i++){
                // each sockfd's status
                LOG(NORMAL, "(%d/%d) Sockfd(%d): unsent_req=%d, sent_req=%d, rcvd_res=%d", 
                    all_fin, this->total_req,
                    this->sockets[i].sockfd, this->sockets[i].unsent_req,
                    this->sockets[i].sent_req, this->sockets[i].rcvd_res);
                // sent when your sent_req is 0
                if(this->sockets[i].sent_req==0){
                    // each socket send num_gap (or smaller) at one time
                    if(this->sockets[i].unsent_req < this->num_gap && this->sockets[i].unsent_req>=0){
                        // check if --fast is enable or not
                        // DONWAIT: (using non-blocking) measure how many reqs have been sent (it can't be use under --fast mode)
                        if(!fast){
                            // if < num_gap, just send all
                            while(this->sockets[i].unsent_req--){
                                //if((send(this->sockets[i].sockfd, http_request, strlen(http_request), MSG_DONTWAIT)) < 0){
                                if((send(this->sockets[i].sockfd, http_request, strlen(http_request), 0)) < 0){
                                    // sent error
                                    perror("Socket sent error.");
                                    exit(1);
                                    // sometime means that peer's recv buffer is full, abort
                                    //this->sockets[i].unsent_req++;
                                    //break;
                                }
                                this->sockets[i].sent_req++;
                            }
                            // unsent will be -1 when break
                            this->sockets[i].unsent_req++;
                        } else {
                            // fast is enable, pack several send request together
                            if(packed_len != this->sockets[i].unsent_req){
                                packed_req=copy_str_n_times(http_request, this->sockets[i].unsent_req);
                                packed_len=this->sockets[i].unsent_req;
                            }
                            
                            // because is fast mode, we deliver all reqs in one send(), so its hard to handle DONWAIT
                            if(send(this->sockets[i].sockfd, packed_req, strlen(packed_req), 0) < 0){
                                // sent error
                                perror("Socket sent error. (packed req)");
                                exit(1);
                            }
                            this->sockets[i].sent_req=this->sockets[i].unsent_req;
                            this->sockets[i].unsent_req=0;
                        }
                        // add workload 
                        workload+=this->sockets[i].sent_req;
                    } else if(this->sockets[i].unsent_req >= this->num_gap){
                        // check if --fast is enable or not
                        if(!fast){
                            // if not, sent num_gap at one time
                            for(int j=0;j<this->num_gap;j++){
                                //if(send(this->sockets[i].sockfd, http_request, strlen(http_request), MSG_DONTWAIT) < 0){
                                if(send(this->sockets[i].sockfd, http_request, strlen(http_request), 0) < 0){
                                    // sent error
                                    perror("Socket sent error.");
                                    // check_tcp_conn_stat(this->sockets[i].sockfd);
                                    //printf("Sent: %d\n, Unsent: %d\n", this->sockets[i].sent_req, this->sockets[i].unsent_req);
                                    //break;
                                    exit(1);
                                }
                                this->sockets[i].unsent_req--;
                                this->sockets[i].sent_req++;
                            }   
                        } else {
                            // fast is enable, pack several send request together
                            if(packed_len != this->num_gap){
                                packed_req=copy_str_n_times(http_request, this->num_gap);
                                packed_len=this->num_gap;
                            }
                            
                            if(send(this->sockets[i].sockfd, packed_req, strlen(packed_req), 0) < 0){
                                // sent error
                                perror("Socket sent error. (packed req)");
                                exit(1);
                            }
                            this->sockets[i].sent_req=this->num_gap;
                            this->sockets[i].unsent_req-=this->num_gap;
                        }
                        workload+=this->num_gap;
                    }
                } /* sent only when your sent_req (workload) is finished (e.g. == 0) */
            } /* each socket send their workload */

            // quit when finish all workload of this round
            while(workload>0){
                // using poll()
                int ret=0;
                if ( (ret=poll(ufds, this->args->conc, timeout) )>0 ) { // 0.5 sec
                    // printf("Ret=%d\n", ret);
                    for(int i=0;i<this->args->conc;i++){
                        // check each one if there have any available rcv or not
                        if ( ufds[i].revents & POLLIN ) {
                            control_var_t *control_var;
                            control_var=multi_bytes_http_parsing_state_machine(state_m, this->sockets[i].sockfd, this->sockets[i].sent_req);
                            /** Issue (single thread, multi-connections): 
                             *  - if you tend to finish all `burst_length` recv at one time, it will block the following connections 
                             *  - if we always start from first one, it will always let first connection finished, while let the rest keep waiting (sometimes it will timeout)
                             */
                            this->sockets[i].rcvd_res+=control_var->num_resp;
                            workload-=control_var->num_resp;
                            all_fin+=control_var->num_resp;
                            this->sockets[i].unsent_req+=(this->sockets[i].sent_req-control_var->num_resp);
                            this->sockets[i].sent_req-=control_var->num_resp;

                            // check control_var's ret, if no error, then we can increase counter
                            // - using control->num_resp as the response we have finished
                            // - this part need to handle error when there using http pipelining 
                            // - in some case, control_var->num_resp != sent_req (need to retransmit)
                            //      e.g. using very big number of --burst (pipeline size)
                            switch(control_var->rcode){
                                case RCODE_POLL_DATA: // still have unfinished data (pipe)
                                case RCODE_FIN: // finish
                                case RCODE_CLOSE: // connection close, check the workload
                                    if(this->sockets[i].sent_req>0){
                                        // not finish yet, need to open new connection
                                        close(this->sockets[i].sockfd);
                                        this->sockets[i].sockfd=create_tcp_conn(this->args->host, itoa(this->args->port));
                                        ufds[i].fd=this->sockets[i].sockfd;
                                        workload-=this->sockets[i].sent_req; // exit the loop
                                        this->sockets[i].sent_req=0;
                                        this->sockets[i].retry_conn_num++; // inc. the number of retry connection
                                    }
                                    break;
                                case RCODE_REDIRECT: // TODO: require redirection
                                    break;
                                default: // other errors
                                    printf("Error, code=%s\n", rcode_str[control_var->rcode]);
                                    exit(1);
                                    break;
                            }
                            this->sockets[i].retry=0; // if this socket has sent something, reset retry
                        }
                    }
                } else if(ret==-1) {
                    perror("poll");
                } else {
                    // need to wait more time
                    LOG(WARNING, "Timeout occurred! No data after waiting seconds.");
                    // check which socket has unfinished (e.g. unsent_req > 0)
                    /** FIXME: need to set the retry-retry limitation */
                    for(int i=0; i<this->args->conc; i++){
                        if(this->sockets[i].unsent_req>0){
                            if(get_tcp_conn_stat(this->sockets[i].sockfd)==TCP_CLOSE_WAIT || 
                                    get_tcp_conn_stat(this->sockets[i].sockfd)==TCP_CLOSE){
                                // reset the connection
                                close(this->sockets[i].sockfd);
                                this->sockets[i].sockfd=create_tcp_conn(this->args->host, itoa(this->args->port));
                                ufds[i].fd=this->sockets[i].sockfd; // update the file descriptor in polling set
                                this->sockets[i].retry_conn_num++; // inc. the number of retry connection
                                this->sockets[i].retry=0;
                            }
                        }
                    }
                }
            }
        }
    } else {
        /* not pipeline */
        struct pollfd ufds[this->args->conc];
        for(int i=0;i<this->args->conc;i++){
            ufds[i].fd=this->sockets[i].sockfd;
            ufds[i].events = POLLIN;
        }

        int all_fin=0;
        while(all_fin<this->total_req){
            // workload in current iteration
            u32 workload=0;
            for(int i=0;i<this->args->conc;i++){
                LOG(NORMAL, "(%d/%d) Sockfd(%d): unsent_req=%d, sent_req=%d, rcvd_res=%d", 
                        all_fin, this->total_req,
                        this->sockets[i].sockfd, this->sockets[i].unsent_req,
                        this->sockets[i].sent_req, this->sockets[i].rcvd_res);
                // send one if there have available workload
                if(this->sockets[i].sent_req==0 && this->sockets[i].unsent_req>0){
                    if(send(this->sockets[i].sockfd, http_request, strlen(http_request), 0) < 0){
                        // sent error
                        perror("Socket sent error.");
                        exit(1);
                    }
                    this->sockets[i].unsent_req--;
                    this->sockets[i].sent_req++;
                }
                // inc workload
                workload+=this->sockets[i].sent_req;
            }
            // need to make sure all the workload has been finished !
            while(workload>0){
                int ret=0;
                if ( (ret= poll(ufds, this->args->conc, timeout) )>0 ) {
                    for(int i=0;i<this->args->conc;i++){
                        // check each one if there have any available rcv or not
                        if ( ufds[i].revents & POLLIN ) {
                            control_var_t *control_var;
                            control_var=multi_bytes_http_parsing_state_machine(state_m, this->sockets[i].sockfd, this->sockets[i].sent_req);
                            // check control_var's ret, if no error, then we can increase counter
                            this->sockets[i].rcvd_res+=control_var->num_resp;
                            workload-=control_var->num_resp; // decrease the worload
                            all_fin+=control_var->num_resp; 
                            this->sockets[i].unsent_req+=(this->sockets[i].sent_req-control_var->num_resp);
                            this->sockets[i].sent_req-=control_var->num_resp; 
                            switch(control_var->rcode){
                                case RCODE_POLL_DATA: // ?
                                case RCODE_FIN:
                                    // finish
                                case RCODE_CLOSE:
                                    // connection close, check the workload
                                    if(this->sockets[i].sent_req>0){
                                        // not finish yet, need to open new connection 
                                        close(this->sockets[i].sockfd);
                                        this->sockets[i].sockfd=create_tcp_conn(this->args->host, itoa(this->args->port));
                                        ufds[i].fd=this->sockets[i].sockfd;
                                        workload-=this->sockets[i].sent_req; // exit the loop
                                        this->sockets[i].retry_conn_num++; // inc. the number of retry connection
                                        this->sockets[i].sent_req=0;
                                    }
                                    break;
                                case RCODE_REDIRECT:
                                    // redirect
                                    break;
                                default:
                                    // other errors
                                    printf("Error=%s\n", rcode_str[control_var->rcode]);
                                    exit(1);
                                    break;
                            }
                            this->sockets[i].retry=0; // if this socket has sent something, reset retry
                        }
                    }
                } else if(ret==-1) {
                    perror("poll");
                } else {
                    // need to wait more time
                    LOG(WARNING, "Timeout occurred! No data after waiting seconds.");
                    // check which socket has unfinished (e.g. unsent_req > 0)
                    /** FIXME: need to set the retry-retry limitation */
                    for(int i=0; i<this->args->conc; i++){
                        if(this->sockets[i].unsent_req>0){
                            if(get_tcp_conn_stat(this->sockets[i].sockfd)==TCP_CLOSE_WAIT || get_tcp_conn_stat(this->sockets[i].sockfd)==TCP_CLOSE){
                                // reset the connection
                                close(this->sockets[i].sockfd);
                                this->sockets[i].sockfd=create_tcp_conn(this->args->host, itoa(this->args->port));
                                ufds[i].fd=this->sockets[i].sockfd; // update the fd into polling set
                                this->sockets[i].retry_conn_num++; // inc. the number of retry connection
                                this->sockets[i].retry=0;
                            }
                        }
                    }
                }
            
            }
        }
    }

    // store the connection status into statistics.
    STATS_CONN(this); 
    // dump the statistics
    STATS_DUMP();

    /* free */
    free(http_request);
    free(state_m);
    free(this); // conn_mgnt obj
    /* finish: need to print all the statistics again */
    return 0; 
}

// deprecated currently
int 
conn_mgnt_run_blocking(conn_mgnt_t *this)
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

    // init our state machine
    state_machine_t *state_m=create_parsing_state_machine();
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
            control_var=multi_bytes_http_parsing_state_machine(state_m, sockfd, sent_req);
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
            control_var=multi_bytes_http_parsing_state_machine(state_m, sockfd, 1);
        }
    }
}

conn_mgnt_t *
create_conn_mgnt(
    parsed_args_t *args)
{
    conn_mgnt_t *mgnt=calloc(1, sizeof(conn_mgnt_t));
    mgnt->args=args;
    mgnt->num_gap=burst_length; // configurable

    /* create socket lists */
    mgnt->sockets=calloc(args->conc, sizeof(struct _conn_t));
    for(int i=0;i<args->conc;i++){
        // create sockfd
        mgnt->sockets[i].sockfd=create_tcp_conn(args->host, itoa(args->port));
        // mgnt->sockets[i].sockfd=create_tcp_keepalive_conn(
        //    args->host, itoa(args->port), 5, 1, 5);
        if(mgnt->sockets[i].sockfd<0){
            printf("Fail to reate sockfd: %d\n", mgnt->sockets[i].sockfd);
            exit(1);
        }
        // set total reqs (workload) of each sockfd
        mgnt->sockets[i].unsent_req=(args->conn/args->conc);
    }
    // append the rest workload to last sockfd
    mgnt->sockets[args->conc-1].unsent_req+=(args->conn%args->conc);

    return mgnt;
}
