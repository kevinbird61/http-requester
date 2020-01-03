#include "conn_mgnt.h"

// global init
u32 g_burst_length=NUM_GAP;
u8  g_fast=0; // default is false

int 
conn_mgnt_run_non_blocking(conn_mgnt_t *this)
{
    STATS_THR_TIME_START(this->thrd_num); // start time of thrd

    char *packed_req=NULL, *http_request=NULL;
    int packed_len=0, leftover=0, request_size=0, timeout=POLL_TIMEOUT;
    u64 t_start=0, t_end=0, resp_intvl=0;
    int all_fin=0, prev_fin=0, ret=0;

    // start-line
    http_req_create_start_line(&http_request, this->args->method, this->args->path, HTTP_1_1);
    // header fields
    http_req_obj_ins_header_by_idx(&this->args->http_req, REQ_HOST, this->args->host);
    http_req_obj_ins_header_by_idx(&this->args->http_req, REQ_CONN, ((this->total_req > 1)? "keep-alive": "close"));
    http_req_obj_ins_header_by_idx(&this->args->http_req, REQ_USER_AGENT, AGENT);
    // finish
    http_req_finish(&http_request, this->args->http_req);
    request_size=strlen(http_request);

    // pre-allocated sending reqs (for burst)
    if(this->args->enable_pipe){
        if(this->num_gap < MIN_NUM_GAP){
            packed_req=copy_str_n_times(http_request, MIN_NUM_GAP);
        } else {
            packed_req=copy_str_n_times(http_request, this->num_gap);
        }
    }

    // using poll()
    struct pollfd ufds[this->args->conn];
    for(int i=0;i<this->args->conn;i++){
        ufds[i].fd=this->sockets[i].sockfd;
        ufds[i].events = POLLIN | POLLOUT; // both write & read
        this->sockets[i].state_m->thrd_num=this->thrd_num; // set thrd_num 
    }

    while(all_fin<this->total_req){
        if ( (ret=poll(ufds, this->args->conn, timeout) )>0 ) { // poll
            // reset timeout to 1 sec
            timeout=POLL_TIMEOUT;

            // using ring-like sending model (not always start from 0, each node has equal change)
            for(int i=0; i<this->args->conn; i++){
                if(this->sockets[i].unsent_req==0 && this->sockets[i].sent_req==0){ // we can skip this one (which have finish its sending process)
                    // finish, close
                    continue;
                }

                /********************************************* error check *********************************************/
                if( ufds[i].revents & POLLERR ){
                    /** An error has occurred on the device or stream. (This flag is only valid in the revents bitmask; it shall be ignored in the events member)
                     * - remote device cannot connect (send RST to us), we need to wait
                     * - remote device close the connection
                     */
                    // check connection & unanwser req (sent_req) -> will handle by POLLIN event (check POLLIN)
                    if( ufds[i].revents & POLLIN || ufds[i].revents & POLLOUT ){
                        // will handle by R/W event, waiting
                        if(errno==EWOULDBLOCK || errno==EAGAIN){
                            // Operation would block, in non-blocking mode, we just ignore it
                            LOG(KB_CM, "Operation would block, wait for next turn.");
                        } else if(errno==EINPROGRESS){
                            // try to connect now 
                            LOG(KB_CM, "[INPROGRESS] Connection is establishing now, please wait (%d)", 1);
                        } 
                    } else {
                        // remote device is unavailable, create a new one
                        LOG(KB_CM, "POLLERR");
                        goto close_and_create;
                    }
                } else if( ufds[i].revents & POLLHUP ){
                    /* The device has been disconnected */
                    LOG(KB_CM, "POLLHUP"); // create a new one for it
                    goto close_and_create;
                } else if( ufds[i].revents & POLLNVAL ){
                    /* The specified fd value is invalid. This flag is only valid in the revents member; it shall ignored in the events member. */
                    LOG(KB_CM, "POLLNVAL");
                    goto end; // invalid, close the program
                }

                /********************************************* able to recv *********************************************/
                if ( ufds[i].revents & POLLIN ) { 
                    // record response `timestamp` only when "single connection", and then PUSH into response interval queue
                    if(this->args->conn==1 && resp_intvl>0){ // single connection, and send() successfully
                        resp_intvl=read_tsc()-resp_intvl;
                        STATS_PUSH_RESP_INTVL(this->thrd_num, resp_intvl);
                        resp_intvl=0; // reset
                    }

                    // able to recv
                    control_var_t control_var;
                    t_start=read_tsc();
                    // upperbound num of resp is unanswered req
                    control_var=multi_bytes_http_parsing_state_machine_non_blocking(this->sockets[i].state_m, this->sockets[i].sockfd, this->sockets[i].sent_req);
                    t_end=read_tsc();
                    STATS_INC_PROCESS_TIME(this->thrd_num, t_end-t_start);

                    this->sockets[i].rcvd_res+=control_var.num_resp;
                    all_fin+=control_var.num_resp;
                    this->sockets[i].unsent_req-=control_var.num_resp;
                    this->sockets[i].sent_req-=control_var.num_resp;
                    this->sockets[i].sent_req=(this->sockets[i].sent_req<0)? 0: this->sockets[i].sent_req; // can sent_req be negative ?
                    
                    switch(control_var.rcode){
                        case RCODE_POLL_DATA: // still have unfinished data (pipe), don't wait
                            break;
                        case RCODE_FIN: // finish (with no more data)
                        case RCODE_CLOSE: // connection close by peer, check the workload
                            reset_parsing_state_machine(this->sockets[i].state_m);
                            if(all_fin==this->total_req){ 
                                goto end;
                            }
                            // recv 0: means that peer side has disconnected, we don't need to check tcp state again.
                            if(this->sockets[i].sent_req>0){ // you sent too much, recv side can't consume
                                int prev_num_gap=this->sockets[i].num_gap;
                                this->sockets[i].num_gap=((this->sockets[i].num_gap*DEC_RATE_NUMERAT)/(DEC_RATE_DENOMIN));
                                this->sockets[i].num_gap=((this->sockets[i].num_gap< MIN_NUM_GAP)? MIN_NUM_GAP: this->sockets[i].num_gap); // min-pipe size
                                this->sockets[i].retry_conn_num++; // inc. the number of retry connection
                                this->sockets[i].sent_req=0; // new connection, set sent_req to 0
                                LOG(KB_CM, "(THR: %d, SOCK: %d) CLOSE-RESIZE, from %d to %d.", this->thrd_num, this->sockets[i].sockfd, prev_num_gap, this->sockets[i].num_gap);
                            }

                            // LOG(KB_CM, "(THR: %d, SOCK: %d) CONN still ESTAB, waiting ... ", this->thrd_num, this->sockets[i].sockfd);
                            LOG(KB_CM, "(THR: %d, SOCK: %d) All: %d, Total: %d | Sent: %d, Unsent: %d, Rcvd: %d | NUM_GAP: %d", 
                                this->thrd_num, this->sockets[i].sockfd,
                                all_fin, this->total_req,
                                this->sockets[i].sent_req,  this->sockets[i].unsent_req,  this->sockets[i].rcvd_res,
                                this->sockets[i].num_gap);

                            close(this->sockets[i].sockfd); // do we need to wait ?
                            char *port=itoa(this->args->port);
                            this->sockets[i].sockfd=create_tcp_conn_non_blocking(this->args->host, port);
                            ufds[i].fd=this->sockets[i].sockfd;
                            this->sockets[i].leftover=0; // need to reset leftover 
                            free(port);
                            // because fd has been changed, we need to move to next connection
                            continue;
                            break;
                        case RCODE_REDIRECT: /* TODO: require redirection (need to update URL) */
                            // 1) need to close all current connection and create new one
                            // 2) need to finish the rest of reqs with new URL
                            // break;
                            goto end; // currently we can't handle redirection (until we support SSL)
                        case RCODE_SERVER_ERR: // 5xx server side error (Do we need to retry in pipeline mode?)
                            LOG(KB_CM, "(THR: %d, SOCK: %d) %s, close the program immediately.", 
                                this->thrd_num, this->sockets[i].sockfd,
                                rcode_str[control_var.rcode]);
                            goto end;
                        case RCODE_CLIENT_ERR: // 4xx client side error 
                            LOG(KB_CM, "%s", rcode_str[control_var.rcode]); 
                        case RCODE_ERROR: {// error occur, need to abort this connection
                            // not finish yet, need to open new connection
                            /*if(get_tcp_conn_stat(this->sockets[i].sockfd)==TCP_CLOSE_WAIT || 
                                get_tcp_conn_stat(this->sockets[i].sockfd)==TCP_CLOSE){ // FIXME: use `Connection` field to determine that we need to create a new conn or not 
                            */
                            if( this->sockets[i].state_m->resp->status_code == _404_NOT_FOUND || 
                                this->sockets[i].state_m->resp->status_code == _403_FORBIDDEN || 
                                this->sockets[i].state_m->resp->status_code == _405_METHOD_NOT_ALLOWED || 
                                this->sockets[i].state_m->resp->status_code == _406_NOT_ACCEPTABLE){ 
                                /** in these situation, we may not need to close connection, instead 
                                 * of using the origin connection again.
                                 * FIXME: there are still have other status code 
                                 */
                                this->sockets[i].sent_req=0;
                                this->sockets[i].leftover=0; // we don't need to shift sending data for incomplete request                            
                                reset_parsing_state_machine(this->sockets[i].state_m); // reset the parsing state machine 
                                continue;
                            } else {
                                /**
                                 * For example: 400, 408, 413, ... 
                                 * the connection will be closed by server side.
                                 * FIXME: other 4xx response ?
                                 */
close_and_create:
                            if(this->sockets[i].sent_req>0){ // means that we have sent some reqs before, but this conn has been closed => we should decrease max-req size
                                int prev_num_gap=this->sockets[i].num_gap;
                                this->sockets[i].num_gap=((this->sockets[i].num_gap*DEC_RATE_NUMERAT)/(DEC_RATE_DENOMIN)); // decrease max_req_size, and reset num_gap (burst length)
                                this->sockets[i].num_gap=((this->sockets[i].num_gap<MIN_NUM_GAP)? MIN_NUM_GAP: this->sockets[i].num_gap); // min-pipe size
                                LOG(KB_CM, "(THR: %d, SOCK: %d) ERROR-RESIZE, from %d to %d.",
                                    this->thrd_num, this->sockets[i].sockfd,
                                    prev_num_gap, this->sockets[i].num_gap);
                            }
                            // not finish yet, need to open new connection
                            close(this->sockets[i].sockfd);
                            // need to wait a second 
                            LOG(KB_CM, "Close the connection (%d) and wait `%d sec.` to create a new one.", this->sockets[i].sockfd, RETRY_WAIT_TIME); // FIXME: warning ?
                            sleep(RETRY_WAIT_TIME); // sleep will hang the user that using huge burst length
                            char *port=itoa(this->args->port);
                            this->sockets[i].sockfd=create_tcp_conn_non_blocking(this->args->host, port);
                            ufds[i].fd=this->sockets[i].sockfd;
                            this->sockets[i].retry_conn_num++; // inc. the number of retry connection
                            this->sockets[i].sent_req=0;
                            this->sockets[i].leftover=0; // we don't need to shift sending data for incomplete request                            
                            reset_parsing_state_machine(this->sockets[i].state_m); // reset the parsing state machine 
                            free(port);
                            // because current fd is invalid, go to next connection
                            continue;
                            } /* close connection by status_code (4xx) */
                            /* get_tcp_conn_stat (only close and create when server close this conn first) */
                        }
                        case RCODE_NOT_SUPPORT: // parsing not finished (suspend by incomplete packet), or parsing error
                        default: // TODO: other errors
                            LOG(KB_CM, "Error, code=%s", rcode_str[control_var.rcode]); // FIXME: warning ?
                            close(this->sockets[i].sockfd);
                            exit(1);
                    }
                }

                /********************************************* able to sent *********************************************/
                if ( ufds[i].revents & POLLOUT ) { 
                    // check workload (unsent_req), and if there have any unanswer req (sent_req)
                    // if(this->sockets[i].unsent_req>0 && this->sockets[i].sent_req <= MAX_SENT_REQ){ // send if there have unsent requests, and there are no any unanswered req
                    if(this->sockets[i].unsent_req>0){ // send if there have unsent requests
                        if(this->args->enable_pipe){ // enable pipe 
                            // calculate workload (if there have unanswered req, workload == number of responses msg)
                            int workload =  (this->sockets[i].unsent_req < this->sockets[i].num_gap)? 
                                            (this->sockets[i].unsent_req-this->sockets[i].sent_req): 
                                            (this->sockets[i].num_gap-this->sockets[i].sent_req);
                            workload=(workload<0)? 0: workload;
                            int workload_bytes= (workload==0)? 
                                                this->sockets[i].leftover: 
                                                (request_size*(workload-1))+(request_size-this->sockets[i].leftover);
                            // if there have nothing to send, then go to next connection
                            if(workload_bytes==0){ 
                                continue;
                            }

                            if(!g_fast){ // non-fast mode
                                for(int j=0; j<workload; j++){
                                    // send the requests
                                    this->sockets[i].sent_req++;
                                    if((send(this->sockets[i].sockfd, http_request, request_size, 0)) < 0){
                                        // handle errno
                                        sock_sent_err_handler(&(this->sockets[i]));
                                        break;
                                    }
                                    STATS_INC_SENT_BYTES(this->thrd_num, request_size);
                                    STATS_INC_SENT_REQS(this->thrd_num, 1);
                                }
                            } else {
                                // because is fast mode, we deliver all reqs in one send(), so its hard to handle DONWAIT
                                int sentbytes=0, sent_req=0;

                                if( (sentbytes=send(this->sockets[i].sockfd, packed_req+this->sockets[i].leftover, workload_bytes, MSG_DONTWAIT)) < 0){
                                    // req not sent complete
                                    sock_sent_err_handler(&(this->sockets[i]));
                                } else if(sentbytes==0) {
                                    // do we need to adjust num_gap again in here?
                                    continue;
                                } else {
                                    // sentbytes > 0
                                    /* using sendbytes to adjust the num_gap
                                     * - FIXME: if we set the num_gap in recv ?
                                     */
                                    sent_req=(sentbytes+this->sockets[i].leftover)/request_size;
                                    this->sockets[i].leftover=(sentbytes+this->sockets[i].leftover)%request_size;
                                    this->sockets[i].sent_req+=sent_req;
                                    STATS_INC_SENT_BYTES(this->thrd_num, sentbytes);
                                    STATS_INC_SENT_REQS(this->thrd_num, sent_req);
                                }
                            }
                        } else {
                            // non-pipe
                            if(this->sockets[i].sent_req==0 && this->sockets[i].unsent_req>0){
                                if(send(this->sockets[i].sockfd, http_request, request_size, 0) < 0){
                                    // req sent not success
                                    sock_sent_err_handler(&(this->sockets[i]));
                                } else {
                                    this->sockets[i].sent_req++;
                                    STATS_INC_SENT_BYTES(this->thrd_num, request_size);
                                    STATS_INC_SENT_REQS(this->thrd_num, 1);
                                    // record request `timestamp` only when "single connection"
                                    if(this->args->conn==1){
                                        resp_intvl=read_tsc();     
                                    }
                                }
                            }
                        } 
                    } else { /* else => don't sent (unsent_req < 0) */ 
                        //this->num_gap=((this->num_gap*DEC_RATE_NUMERAT)/(DEC_RATE_DENOMIN)); // decrease max_req_size, and reset num_gap (burst length)
                        //this->num_gap=((this->num_gap<MIN_NUM_GAP)? MIN_NUM_GAP: this->num_gap); // min-pipe size
                    }
                } /* POLLOUT */
            } /* for loop for each poll event */
        } else if (ret==-1){
            perror("poll");
            poll_err_handler(this);
            break;
        } else {
            // need to wait more time (no R/W available now)
            LOG(KB_CM, "Timeout occurred! No data after waiting seconds.");
            LOG(KB_CM, "(%d) Waiting for available R/W ... (poll timeout=%d)", this->thrd_num, timeout); 
            timeout*=2; // increase timeout (reset when ret>0)
            if(timeout > POLL_MAX_TIMEOUT){ // wait until POLL_MAX_TIMEOUT
                LOG(KB_CM, "CLOSE sending process, we can't wait it anymore.\n");
                // FIXME: do we need to retry again ?
                break;
            }
        }
    }

end:
    LOG(KB_CM, "Thrd %d has finished.", this->thrd_num);
    STATS_THR_TIME_END(this->thrd_num); // end time of thrd
    // store the connection status into statistics.
    STATS_CONN(this); 
    
    /* free */
    if(packed_req!=NULL){ free(packed_req); }
    free(http_request);
    for(int i=0;i<this->args->conn;i++){
        close(this->sockets[i].sockfd); // close all conn
        if(this->sockets[i].state_m->resp!=NULL){
            free(this->sockets[i].state_m->resp);
        }
        if(this->sockets[i].state_m->buff){
            free(this->sockets[i].state_m->buff);
        }
        if(this->sockets[i].state_m){
            free(this->sockets[i].state_m);
        }
    }
    if(this->sockets!=NULL){
        free(this->sockets);
    }
    if(this!=NULL){
        free(this); // conn_mgnt obj
    }

    /* finish: need to print all the statistics again */
    pthread_exit(0);
}

int 
conn_mgnt_run(conn_mgnt_t *this)
{
    STATS_THR_TIME_START(this->thrd_num); // start time of thrd

    /* forge http request header */
    char *http_request;
    // start-line
    http_req_create_start_line(&http_request, this->args->method, this->args->path, HTTP_1_1);
    // header fields
    http_req_obj_ins_header_by_idx(&this->args->http_req, REQ_HOST, this->args->host);
    http_req_obj_ins_header_by_idx(&this->args->http_req, REQ_CONN, ((this->total_req > 1)? "keep-alive": "close"));
    http_req_obj_ins_header_by_idx(&this->args->http_req, REQ_USER_AGENT, AGENT);
    // finish
    http_req_finish(&http_request, this->args->http_req);

    char *packed_req=NULL;
    if(this->args->enable_pipe){
        if(this->num_gap < MIN_NUM_GAP){
            packed_req=copy_str_n_times(http_request, MIN_NUM_GAP);
        } else {
            packed_req=copy_str_n_times(http_request, this->num_gap);
        }
    }
    int packed_len=0, request_size=strlen(http_request);
    // timeout
    int timeout=POLL_TIMEOUT;

    if(this->args->enable_pipe){
        /* support pipeline */
        u64 t_start=0, t_end=0;
        // using poll()
        struct pollfd ufds[this->args->conn];
        for(int i=0;i<this->args->conn;i++){
            ufds[i].fd=this->sockets[i].sockfd;
            ufds[i].events = POLLIN;
            this->sockets[i].state_m->thrd_num=this->thrd_num; // set thrd_num 
        }

        // each sockfd check its workload, and send its request 
        int all_fin=0;
        // if unfinished, keep send & recv
        while(all_fin<this->total_req){
            u32 workload=0; // in each iteration
            // each socket send num_gap at one time
            for(int i=0;i<this->args->conn;i++){
                // each sockfd's status
                LOG(KB_CM, "(%d/%d) Sockfd(%d): unsent_req=%d, sent_req=%d, rcvd_res=%d", 
                    all_fin, this->total_req,
                    this->sockets[i].sockfd, this->sockets[i].unsent_req,
                    this->sockets[i].sent_req, this->sockets[i].rcvd_res);
                // sent when your sent_req is 0
                if(this->sockets[i].sent_req==0 && this->sockets[i].unsent_req>=0){ 
                    int sent_req=(this->sockets[i].unsent_req < this->num_gap)? (this->sockets[i].unsent_req): (this->num_gap);
                    // check if --fast is enable or not
                    if(!g_fast){
                        // if < num_gap, just send all
                        while(sent_req--){
                            if((send(this->sockets[i].sockfd, http_request, request_size, 0)) < 0){
                                // sent error
                                perror("Socket sent error.");
                                exit(1);
                            }
                            this->sockets[i].sent_req++;
                            this->sockets[i].unsent_req--;
                        }
                    } else {
                        // fast is enable, pack several send request together
                        if(packed_len!=sent_req){
                            packed_len=sent_req;
                        }
                        // because is fast mode, we deliver all reqs in one send(), so its hard to handle DONWAIT
                        if(send(this->sockets[i].sockfd, packed_req, request_size*packed_len, 0) < 0){
                            // sent error
                            perror("Socket sent error. (packed req)");
                            exit(1);
                        }
                        this->sockets[i].sent_req=sent_req;
                        this->sockets[i].unsent_req-=sent_req;
                    }
                    // add workload 
                    workload+=this->sockets[i].sent_req;
                    STATS_INC_SENT_BYTES(this->thrd_num, this->sockets[i].sent_req*request_size);
                    STATS_INC_SENT_REQS(this->thrd_num, this->sockets[i].sent_req);
                } /* sent only when your sent_req (workload) is finished (e.g. == 0) */
            } /* each socket send their workload */

            // quit when finish all workload of this round
            while(workload>0){
                // using poll()
                int ret=0;
                if ( (ret=poll(ufds, this->args->conn, timeout) )>0 ) { // 0.5 sec
                    for(int i=0;i<this->args->conn;i++){
                        // check each one if there have any available rcv or not
                        if ( ufds[i].revents & POLLIN ) {
                            control_var_t control_var;
                            t_start=read_tsc();
                            control_var=multi_bytes_http_parsing_state_machine(this->sockets[i].state_m, this->sockets[i].sockfd, this->sockets[i].sent_req);
                            t_end=read_tsc();
                            STATS_INC_PROCESS_TIME(this->thrd_num, t_end-t_start);
                            /** Issue (single thread, multi-connections): 
                             *  - if you tend to finish all `burst_length` recv at one time, it will block the following connections 
                             *  - if we always start from first one, it will always let first connection finished, while let the rest keep waiting (sometimes it will timeout)
                             */
                            this->sockets[i].rcvd_res+=control_var.num_resp;
                            workload-=control_var.num_resp;
                            all_fin+=control_var.num_resp;
                            this->sockets[i].unsent_req+=(this->sockets[i].sent_req-control_var.num_resp);
                            this->sockets[i].sent_req-=control_var.num_resp;

                            // check control_var's ret, if no error, then we can increase counter
                            // - using control->num_resp as the response we have finished
                            // - this part need to handle error when there using http pipelining 
                            // - in some case, control_var.num_resp != sent_req (need to retransmit)
                            //      e.g. using very big number of --burst (pipeline size)
                            switch(control_var.rcode){
                                case RCODE_POLL_DATA: // still have unfinished data (pipe)
                                case RCODE_FIN: // finish
                                case RCODE_CLOSE: // connection close, check the workload
                                    reset_parsing_state_machine(this->sockets[i].state_m);
                                    if(get_tcp_conn_stat(this->sockets[i].sockfd)==TCP_CLOSE_WAIT || 
                                        get_tcp_conn_stat(this->sockets[i].sockfd)==TCP_CLOSE){
                                        if(this->sockets[i].sent_req>0){
                                            //not finish yet, need to open new connection
                                            close(this->sockets[i].sockfd);
                                            this->sockets[i].sockfd=create_tcp_conn(this->args->host, itoa(this->args->port));
                                            ufds[i].fd=this->sockets[i].sockfd;
                                            workload-=this->sockets[i].sent_req; // exit the loop
                                            this->sockets[i].sent_req=0;
                                            this->sockets[i].retry_conn_num++; // inc. the number of retry connection
                                        } else {
                                            // finish (FIXME: exit here is good enough?)
                                            //exit(1);
                                        }
                                    }
                                    break;
                                case RCODE_REDIRECT: // TODO: require redirection
                                    // break;
                                    LOG(KB_CM, "%s, prepare for redirection.", rcode_str[control_var.rcode]);
                                    exit(1); // currently we can't handle redirection (until we support SSL)
                                case RCODE_SERVER_ERR: // 5xx server side error
                                    LOG(KB_CM, "%s, close the program immediately.", rcode_str[control_var.rcode]);
                                    exit(1);
                                case RCODE_CLIENT_ERR: // 4xx client side error 
                                    LOG(KB_CM, "%s", rcode_str[control_var.rcode]); 
                                case RCODE_ERROR: // error occur, need to abort this connection
                                    // not finish yet, need to open new connection
                                    close(this->sockets[i].sockfd);
                                    // need to wait a second 
                                    LOG(KB_CM, "Close the connection and wait %d to create a new one.", RETRY_WAIT_TIME); // FIXME: warning ?
                                    if(this->sockets[i].unsent_req<=0){ // finish
                                        continue;
                                    }
                                    sleep(RETRY_WAIT_TIME);
                                    char *port=itoa(this->args->port);
                                    this->sockets[i].sockfd=create_tcp_conn(this->args->host, port);
                                    ufds[i].fd=this->sockets[i].sockfd;
                                    this->sockets[i].retry_conn_num++; // inc. the number of retry connection
                                    this->sockets[i].sent_req=0;
                                    reset_parsing_state_machine(this->sockets[i].state_m); // reset the parsing state machine 
                                    free(port);
                                    break;
                                default: // other errors
                                    printf("Error, code=%s\n", rcode_str[control_var.rcode]);
                                    break;
                            }
                            this->sockets[i].retry=0; // if this socket has sent something, reset retry
                        }
                    }
                } else if(ret==-1) {
                    perror("poll");
                } else {
                    // need to wait more time
                    LOG(KB_CM, "Timeout occurred! No data after waiting seconds.");
                    // check which socket has unfinished (e.g. unsent_req > 0)
                    /** FIXME: need to set the retry-retry limitation */
                    for(int i=0; i<this->args->conn; i++){
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
        u64 t_start=0, t_end=0;
        u64 resp_intvl=0;
        struct pollfd ufds[this->args->conn];
        for(int i=0;i<this->args->conn;i++){
            ufds[i].fd=this->sockets[i].sockfd;
            ufds[i].events = POLLIN;
        }

        int all_fin=0;
        while(all_fin<this->total_req){
            // workload in current iteration
            u32 workload=0;
            for(int i=0;i<this->args->conn;i++){
                LOG(KB_CM, "(%d/%d) Sockfd(%d): unsent_req=%d, sent_req=%d, rcvd_res=%d", 
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
                    STATS_INC_SENT_BYTES(this->thrd_num, request_size);
                    STATS_INC_SENT_REQS(this->thrd_num, 1);
                }
                
                // inc workload
                workload+=this->sockets[i].sent_req;
                // record request `timestamp` only when "single connection"
                if(this->args->conn==1){
                    resp_intvl=read_tsc();     
                }
            }
            // need to make sure all the workload has been finished !
            while(workload>0){
                int ret=0;
                if ( (ret= poll(ufds, this->args->conn, timeout) )>0 ) {
                    // record response `timestamp` only when "single connection", and then PUSH into response interval queue
                    if(this->args->conn==1){
                        resp_intvl=read_tsc()-resp_intvl;
                        STATS_PUSH_RESP_INTVL(this->thrd_num, resp_intvl);
                    }
                    for(int i=0;i<this->args->conn;i++){
                        // check each one if there have any available rcv or not
                        if ( ufds[i].revents & POLLIN ) {
                            control_var_t control_var;
                            t_start=read_tsc();
                            control_var=multi_bytes_http_parsing_state_machine(this->sockets[i].state_m, this->sockets[i].sockfd, this->sockets[i].sent_req);
                            t_end=read_tsc();
                            // store into stats
                            STATS_INC_PROCESS_TIME(this->thrd_num, t_end-t_start);
                            // check control_var's ret, if no error, then we can increase counter
                            this->sockets[i].rcvd_res+=control_var.num_resp;
                            workload-=control_var.num_resp; // decrease the worload
                            all_fin+=control_var.num_resp; 
                            this->sockets[i].unsent_req+=(this->sockets[i].sent_req-control_var.num_resp);
                            this->sockets[i].sent_req-=control_var.num_resp; 
                            switch(control_var.rcode){
                                case RCODE_POLL_DATA: // ?
                                case RCODE_FIN:
                                    // finish
                                case RCODE_CLOSE:
                                case RCODE_NOT_SUPPORT: // parsing not finished (suspend by incomplete packet), or parsing error
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
                                    printf("Error=%s\n", rcode_str[control_var.rcode]);
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
                    LOG(KB_CM, "Timeout occurred! No data after waiting seconds.");
                    // check which socket has unfinished (e.g. unsent_req > 0)
                    /** FIXME: need to set the retry-retry limitation */
                    for(int i=0; i<this->args->conn; i++){
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

    STATS_THR_TIME_END(this->thrd_num); // end time of thrd
    // store the connection status into statistics.
    STATS_CONN(this); 
    
    /* free */
    if(packed_req!=NULL){ free(packed_req); }
    free(http_request);
    for(int i=0;i<this->args->conn;i++){
        close(this->sockets[i].sockfd); // close all conn
        free(this->sockets[i].state_m->resp);
        free(this->sockets[i].state_m->buff);
        free(this->sockets[i].state_m);
    }
    free(this->sockets);
    free(this); // conn_mgnt obj
    /* finish: need to print all the statistics again */
    return 0; 
}

conn_mgnt_t *
create_conn_mgnt_non_blocking(
    parsed_args_t *args)
{
    conn_mgnt_t *mgnt=calloc(1, sizeof(conn_mgnt_t));
    mgnt->thrd_num=0; // default is 0 (new thread need to set this value)
    mgnt->args=args;
    mgnt->num_gap=g_burst_length; // configurable
    mgnt->total_req=args->reqs;

    /* create socket lists */
    mgnt->sockets=calloc(args->conn, sizeof(struct _conn_t));
    for(int i=0;i<args->conn;i++){
        // create sockfd
        mgnt->sockets[i].sockfd=create_tcp_conn_non_blocking(args->host, itoa(args->port));
        mgnt->sockets[i].state_m=create_parsing_state_machine();
        // reset state machine
        reset_parsing_state_machine(mgnt->sockets[i].state_m);
        if(mgnt->sockets[i].sockfd<0){
            printf("Fail to create sockfd: %d\n", mgnt->sockets[i].sockfd);
            // should we retry ? or using non-blocking to handle
            exit(1);
        }
        // set total reqs (workload) of each sockfd
        mgnt->sockets[i].unsent_req=(mgnt->total_req/args->conn);
        mgnt->sockets[i].sent_req=0;
        mgnt->sockets[i].num_gap=mgnt->num_gap; // each connection can control its' max-req size
    }
    // append the rest workload to last sockfd
    mgnt->sockets[args->conn-1].unsent_req+=(mgnt->total_req%args->conn);

    return mgnt;
}

conn_mgnt_t *
create_conn_mgnt(
    parsed_args_t *args)
{
    conn_mgnt_t *mgnt=calloc(1, sizeof(conn_mgnt_t));
    mgnt->thrd_num=0; // default is 0 (new thread need to set this value)
    mgnt->args=args;
    mgnt->num_gap=g_burst_length; // configurable
    mgnt->total_req=args->reqs;

    /* create socket lists */
    mgnt->sockets=calloc(args->conn, sizeof(struct _conn_t));
    for(int i=0;i<args->conn;i++){
        // create sockfd
        mgnt->sockets[i].sockfd=create_tcp_conn(args->host, itoa(args->port));
        mgnt->sockets[i].state_m=create_parsing_state_machine();
        // reset state machine
        reset_parsing_state_machine(mgnt->sockets[i].state_m);
        if(mgnt->sockets[i].sockfd<0){
            printf("Fail to create sockfd: %d\n", mgnt->sockets[i].sockfd);
            // should we retry ? or using non-blocking to handle
            exit(1);
        }
        // set total reqs (workload) of each sockfd
        mgnt->sockets[i].unsent_req=(mgnt->total_req/args->conn);
        mgnt->sockets[i].sent_req=0;
        mgnt->sockets[i].num_gap=mgnt->num_gap; // each connection can control its' max-req size
    }
    // append the rest workload to last sockfd
    mgnt->sockets[args->conn-1].unsent_req+=(mgnt->total_req%args->conn);

    return mgnt;
}