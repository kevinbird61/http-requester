#include "http.h"

control_var_t
multi_bytes_http_parsing_state_machine_non_blocking(
    state_machine_t *state_m,
    int sockfd,
    u32 num_reqs)
{
    /* get all the data:
     * - poll the data until connection state is TCP_CLOSE_WAIT, and recvbytes is 0 
     */
    /* stats */
    int flag=1, recvbytes=0, fin_resp=0, fin_idx=0;
    control_var_t control_var;
    /* Parse the data, and store the result into respones objs */
    while(flag){
        /*if(fin_resp==1)
            printf("NON-BLOCKING while\n");*/

        // call parser (store the state and response obj into state_machine's instance)
        control_var=http_resp_parser(state_m);
recv_again:
        switch (control_var.rcode)
        {
            case RCODE_POLL_DATA:
                // if not enough, keep polling 
                if(num_reqs==0){ // finish all response
                    control_var.num_resp=fin_resp;
                    control_var.rcode=RCODE_FIN;
                    return control_var;
                }
                /* require another recv() to poll new data */
                // check upperbound & move (prevent using too many realloc, and too much buff length)
                if( state_m->data_size > ((RECV_BUFF_SCALE-1)*CHUNK_SIZE) ){
                    /* we need to skip the chunked data (or store it temporary) to prevent override the http header */
                    if(state_m->chunked_size==0 && state_m->curr_chunked_size>0){
                        // override current chunk (to prevent too many chunked in one response) 
                        // can't override the hdr, so move the reading ptr to `last_fin_idx + msg_hdr_len`
                        // FIXME: if last_fin_idx is too large? (return RCODE_ERROR)
                        state_m->data_size=state_m->resp->msg_hdr_len;
                    } else {
                        // other case
                        state_m->data_size=state_m->data_size-state_m->last_fin_idx;
                    }
                    // then move the leftover to the front, also need to reset buf_idx;
                    if(state_m->use_content_length ){ 
                        // if parsing "content length" (chunked need to consider more case), 
                        // we don't care the data_size (just set to 0, means that all parsed 
                        // content has been dropped -> we don't store it currently)
                        state_m->data_size=state_m->resp->msg_hdr_len; 
                        state_m->parsed_len=0; 
                    } else if(state_m->use_chunked){ // if parsing chunk data now, then we just set the data_size & parse_len to 0
                        state_m->data_size=state_m->last_fin_idx + state_m->resp->msg_hdr_len; 
                        state_m->parsed_len=0;
                    }
                    // need to adjust the offset of each response obj
                    update_res_header_idx(state_m->resp, state_m->last_fin_idx);
                    LOG(KB_SM ,"( Prevbytes: %d ) Last fin idx: %d, Move %d bytes", state_m->prev_rcv_len, state_m->last_fin_idx, state_m->data_size);
                    state_m->buf_idx=state_m->data_size;
                    memcpy(state_m->buff, state_m->buff+state_m->last_fin_idx, state_m->data_size);
                    // reset the rest and fin_idx
                    memset(state_m->buff+state_m->data_size, 0x00, state_m->max_buff_size-state_m->data_size);
                    state_m->last_fin_idx=0; 
                }

                recvbytes=recv(sockfd, state_m->buff+state_m->data_size, CHUNK_SIZE, MSG_DONTWAIT);
                if(recvbytes>0){
                    LOG(KB_SM, "RECV: %d bytes", recvbytes);
                    state_m->prev_rcv_len=recvbytes;
                    state_m->data_size+=recvbytes; // save (recvbytes <= CHUNK_SIZE)
                } else if(recvbytes==0){ // handle the state that RX is empty and server has send fin
                    if(state_m->buf_idx < state_m->data_size){ // if there are something still in the buffer
                        LOG(KB_SM, "Keep parsing: %d (%ld)(%d)", state_m->buf_idx, strlen(state_m->buff), state_m->data_size);
                        //continue;
                        break;
                    } else {
                        LOG(KB_SM, "Finish all response parsing, buf_idx=%d, strlen(buff)=%ld", state_m->buf_idx, strlen(state_m->buff));
                        // reset state machine
                        reset_parsing_state_machine(state_m);
                        // FIXME: does here need to use get_tcp_conn_stat() ? (syscall)
                        control_var.rcode=RCODE_CLOSE; // server turn off the connection
                        flag=0;
                        break;
                    }
                } else { 
                    // -1
                    // LOG(NORMAL, "ERROR RECV: %d bytes, num_reqs: %d, fin_resp: %d, %d", recvbytes, num_reqs, fin_resp, control_var.rcode);
                    if(num_reqs>0){ // not recv all resp, but don't hang in here, turn back (don't block)
                        flag=0;
                        break;
                    } else { // num_reqs<=0, 
                        control_var.num_resp=fin_resp;
                        control_var.rcode=RCODE_FIN;
                        return control_var;
                    }
                }
                break;
            case RCODE_REDIRECT:
                /* perform redirection, notify caller that need to abort the response and resend */
                LOG(KB_SM, "Require rediretion.");
                // return control_var;
                flag=0;
                break;
            case RCODE_ERROR:
                // print the error message instead of exit?
                LOG(KB_SM, "Parsing error occur.");
                flag=0;
                break;
            case RCODE_SERVER_ERR: // 5xx
            case RCODE_CLIENT_ERR: // 4xx
                LOG(KB_SM, "%s", rcode_str[control_var.rcode]);
                /* increase stats */
                STATS_INC_PKT_BYTES(state_m->thrd_num, state_m->resp->msg_hdr_len+state_m->total_content_length); // CL and TE use same memory
                STATS_INC_HDR_BYTES(state_m->thrd_num, state_m->resp->msg_hdr_len);
                STATS_INC_BODY_BYTES(state_m->thrd_num, state_m->total_content_length);
                STATS_INC_RESP_NUM(state_m->thrd_num, 1);
                STATS_INC_CODE(state_m->thrd_num, state_m->resp->status_code);
                flag=0;
                break;
            case RCODE_FIN:
            case RCODE_NEXT_RESP:
                LOG(KB_SM, "Finish one respose.");
                num_reqs--; // finish one response
                fin_resp++;
                /* increase stats */
                STATS_INC_PKT_BYTES(state_m->thrd_num, state_m->resp->msg_hdr_len+state_m->total_content_length); // CL and TE use same memory
                STATS_INC_HDR_BYTES(state_m->thrd_num, state_m->resp->msg_hdr_len);
                STATS_INC_BODY_BYTES(state_m->thrd_num, state_m->total_content_length);
                STATS_INC_RESP_NUM(state_m->thrd_num, 1);
                STATS_INC_CODE(state_m->thrd_num, state_m->resp->status_code);
                if(num_reqs<=0){ // check if we have finished all response or not
                    LOG(KB_SM, "FIN: num_reqs: %d, fin_resp: %d", num_reqs, fin_resp);
                    control_var.num_resp=fin_resp;
                    control_var.rcode=RCODE_FIN;
                    return control_var;
                }
                /* update fin_idx */
                state_m->last_fin_idx=state_m->buf_idx;

                // Finish one resp, and need to parse next:
                // - create a new resp obj (TODO: maintain a response queue to store
                //  all the parsed responses.)
                // - we don't modify the buf_idx, just pass buff ptr to response obj.
                // - reset the essential states, prepare for parsing next response.
                memset(state_m->resp, 0x00, 1*sizeof(http_res_header_status_t));
                state_m->resp->buff=state_m->buff;
                state_m->p_state=VER;
                state_m->resp->status_code=0;
                state_m->resp->http_ver=0;
                state_m->use_content_length=0;
                state_m->use_chunked=0;
                state_m->content_length=0;
                state_m->total_content_length=0;
                state_m->curr_chunked_size=0;
                state_m->parsed_len=0; // prevent from reading wrong starting point
                break;
            case RCODE_NOT_SUPPORT: 
                /* cautious: this case could be caused by parsing error. 
                 * but mostly it will cause by non-blocking recv (which means 
                 * that still need to check if there still need to poll another
                 * data)
                 */
                LOG(KB_SM, "Not support."); // huge burst will hang up here
                state_m->parsed_len=0;
                if(state_m->buf_idx < state_m->data_size){ // still have other data, which means that parsing error 
                    flag=0;
                    control_var.rcode=RCODE_ERROR; // or using RCODE_ERROR
                } else {
                    // require another data (keep parsing)
                    state_m->buf_idx=state_m->last_fin_idx;
                    control_var.rcode=RCODE_POLL_DATA;
                    goto recv_again; /* receive as more as it can */
                }
                break;
            default:
                LOG(KB_SM, "Unknown: %d", control_var.rcode);
                flag=0;
                break;
        }
    }

    control_var.failed_resp=num_reqs; // the leftover are unfinish num_resp 
    control_var.num_resp=fin_resp;
    return control_var;
}

control_var_t
multi_bytes_http_parsing_state_machine(
    state_machine_t *state_m,
    int sockfd,
    u32 num_reqs)
{
    /* get all the data:
     * - poll the data until connection state is TCP_CLOSE_WAIT, and recvbytes is 0 
     */
    /* stats */
    int flag=1, recvbytes=0, fin_resp=0;
    control_var_t control_var;
    /* Parse the data, and store the result into respones objs */
    /* FIXME: enhancement - need to free buffer (e.g. finished resp) 
     *  to prevent occupying too much memory ?
     */
    while(flag){
        // call parser (store the state and response obj into state_machine's instance)
        control_var=http_resp_parser(state_m);
        switch (control_var.rcode)
        {
            case RCODE_POLL_DATA:
                // if not enough, keep polling 
                if(!num_reqs){ // finish all response
                    flag=0;
                    break;
                }
                /* require another recv() to poll new data */
                // check upperbound & move (prevent using too many realloc, and too much buff length)
                // if( (strlen(state_m->buff)) > ((RECV_BUFF_SCALE-1)*CHUNK_SIZE) ){
                if( state_m->data_size > ((RECV_BUFF_SCALE-1)*CHUNK_SIZE) ){
                    /* we need to skip the chunked data (or store it temporary) to prevent override the http header */
                    if(state_m->chunked_size==0 && state_m->curr_chunked_size>0){
                        // override current chunk (to prevent too many chunked in one response) 
                        // can't override the hdr, so move the reading ptr to `last_fin_idx + msg_hdr_len`
                        // FIXME: what if last_fin_idx is too large?
                        state_m->data_size=state_m->resp->msg_hdr_len;
                    } else {
                        // other case
                        state_m->data_size=state_m->data_size-state_m->last_fin_idx;
                    }
                    LOG(KB_SM ,"Data bytes: %d, last_fin_idx=%d (use_chunked: %d, chunked size=%d)", state_m->data_size, state_m->last_fin_idx, state_m->use_chunked, state_m->chunked_size);
                    // then move the leftover to the front, also need to reset buf_idx;
                    if(state_m->use_content_length ){ // if parsing chunk data now, then we just set the data_size & parse_len to 0
                        state_m->data_size=0; // if parsing "content length" (chunked need to consider more case), we don't care the data_size (just set to 0, means that all parsed content has been dropped -> we don't store it currently)
                        state_m->parsed_len=0; 
                    }
                    // need to adjust the offset of each response obj
                    update_res_header_idx(state_m->resp, state_m->last_fin_idx);
                    LOG(KB_SM ,"( Prevbytes: %d ) Last fin idx: %d, Move %d bytes", state_m->prev_rcv_len, state_m->last_fin_idx, state_m->data_size);
                    state_m->buf_idx=state_m->data_size;
                    memcpy(state_m->buff, state_m->buff+state_m->last_fin_idx, state_m->data_size);
                    // reset the rest and fin_idx
                    memset(state_m->buff+state_m->data_size, 0x00, state_m->max_buff_size-state_m->data_size);
                    state_m->last_fin_idx=0; 
                }
               

                /* FIXME: check the connection state before call recv() */
                // check_tcp_conn_stat(sockfd);
                /*if(get_tcp_conn_stat(sockfd)==TCP_CLOSE){
                    // connection is close or ready to close 
                    LOG(WARNING, "Connection(%d) is closed.", sockfd);
                    flag=0;
                    break;
                }*/

                recvbytes=recv(sockfd, state_m->buff+state_m->data_size, CHUNK_SIZE, 0);
                state_m->prev_rcv_len=recvbytes;
                if(recvbytes>0){
                    LOG(KB_SM, "RECV: %d bytes", recvbytes);
                    state_m->prev_rcv_len=recvbytes;
                    state_m->data_size+=recvbytes; // save (recvbytes <= CHUNK_SIZE)
                } else if(recvbytes==0){ // handle the state that RX is empty and server has send fin
                    if(state_m->buf_idx < state_m->data_size){ // if there are something still in the buffer
                        LOG(KB_SM, "Keep parsing: %d (%ld)(%d)", state_m->buf_idx, strlen(state_m->buff), state_m->data_size);
                        break;
                    } else {
                        LOG(KB_SM, "Finish all response parsing, buf_idx=%d, strlen(buff)=%ld", state_m->buf_idx, strlen(state_m->buff));
                        // reset state machine
                        reset_parsing_state_machine(state_m);
                        // FIXME: does here need to use get_tcp_conn_stat() ? (syscall)
                        control_var.rcode=RCODE_CLOSE; // server turn off the connection
                        flag=0;
                        break;
                    }
                } else { // don't hang in here, turn back
                    // -1
                    flag=0;
                    break; 
                }
                break;
            case RCODE_REDIRECT:
                /* perform redirection, notify caller that need to abort the response and resend */
                LOG(KB_SM, "Require rediretion.");
                return control_var;
                //flag=0;
                //break;
            case RCODE_ERROR:
                // print the error message instead of exit?
                LOG(KB_SM, "Parsing error occur.");
                flag=0;
                break;
            case RCODE_SERVER_ERR: // 5xx
            case RCODE_CLIENT_ERR: // 4xx
                LOG(KB_SM, "%s", rcode_str[control_var.rcode]);
                /* increase stats */
                STATS_INC_PKT_BYTES(state_m->thrd_num, state_m->resp->msg_hdr_len+state_m->total_content_length); // CL and TE use same memory
                STATS_INC_HDR_BYTES(state_m->thrd_num, state_m->resp->msg_hdr_len);
                STATS_INC_BODY_BYTES(state_m->thrd_num, state_m->total_content_length);
                STATS_INC_RESP_NUM(state_m->thrd_num, 1);
                STATS_INC_CODE(state_m->thrd_num, state_m->resp->status_code);
                flag=0;
                break;
            case RCODE_FIN:
            case RCODE_NEXT_RESP:
                LOG(KB_SM, "Finish one respose.");
                num_reqs--; // finish one response
                fin_resp++;
                /* increase stats */
                STATS_INC_PKT_BYTES(state_m->thrd_num, state_m->resp->msg_hdr_len+state_m->total_content_length); // CL and TE use same memory
                STATS_INC_HDR_BYTES(state_m->thrd_num, state_m->resp->msg_hdr_len);
                STATS_INC_BODY_BYTES(state_m->thrd_num, state_m->total_content_length);
                STATS_INC_RESP_NUM(state_m->thrd_num, 1);
                STATS_INC_CODE(state_m->thrd_num, state_m->resp->status_code);
                /* update fin_idx */
                state_m->last_fin_idx=state_m->buf_idx;

                // Finish one resp, and need to parse next:
                // - create a new resp obj (TODO: maintain a response queue to store
                //  all the parsed responses.)
                // - we don't modify the buf_idx, just pass buff ptr to response obj.
                // - reset the essential states, prepare for parsing next response.
                memset(state_m->resp, 0x00, 1*sizeof(http_res_header_status_t));
                state_m->resp->buff=state_m->buff;
                state_m->p_state=VER;
                state_m->resp->status_code=0;
                state_m->resp->http_ver=0;
                state_m->use_content_length=0;
                state_m->use_chunked=0;
                state_m->content_length=0;
                state_m->total_content_length=0;
                state_m->curr_chunked_size=0;
                state_m->parsed_len=0; // prevent from reading wrong starting point
 
                break;
            case RCODE_NOT_SUPPORT: /* cautious: this case could be caused by parsing error. */
                LOG(KB_SM, "Not support.");
                flag=0; // do not keep parsing, break 
                /* FIXME: do we need to recover from this kind of error? */
                state_m->buf_idx=state_m->last_fin_idx;
                state_m->parsed_len=0;
                break; 
            default:
                LOG(KB_SM, "Unknown: %d", control_var.rcode);
                flag=0;
                break;
        }
    }
    
    control_var.failed_resp=num_reqs; // the leftover are unfinish num_resp 
    control_var.num_resp=fin_resp;
    return control_var;
}

control_var_t
http_resp_parser(
    state_machine_t *state_m)
{
    // parsing status are stored in state_m, setup control_var to determine the next action
    u8 flag=1;
    http_res_header_status_t *http_h_status_check=state_m->resp;
    control_var_t control_var;

    while(flag){
        // store legal char
        state_m->buf_idx++;
        state_m->parsed_len++;
        // check if we need more data or not
        if(state_m->buf_idx>state_m->data_size){
            /* require more data */
            state_m->buf_idx--;
            state_m->parsed_len--;
            control_var.rcode=RCODE_POLL_DATA;
            return control_var;
        }
        /** Put some checking mechanism to terminate parsing
         * state machine before recipient send FIN back (timeout).
         */
        if(state_m->use_chunked && state_m->chunked_size==0){
            // printf("END, idx: %d\n", state_m->buf_idx-1);
            LOG(KB_PS, "[Transfer-Encoding: Chunked] Parsing process has been done. Total chunked size: %d bytes (%d KB)", state_m->total_chunked_size, state_m->total_chunked_size/1024);
            control_var.rcode=RCODE_FIN;
            break;
        }
        // check if header using transfer-encoding
        if(state_m->use_chunked){
            /*if(!(--state_m->chunked_size)){
                state_m->curr_chunked_size++;
                LOG(INFO, "Finish chunk, idx: %d (Parsed: %d)", state_m->buf_idx-1, state_m->curr_chunked_size);
                // state_m->last_fin_idx=state_m->buf_idx-1; // update last_fin_idx
                state_m->parsed_len=0;
                state_m->use_chunked=0;
                state_m->p_state=NEXT_CHUNKED;
            } else {
                // Parsing other chunk features here 
                state_m->curr_chunked_size++;
                continue;
            }*/
            if(state_m->chunked_size>0){
                // move idx
                int avail_data=(state_m->data_size-state_m->buf_idx)+1;
                if(avail_data >= state_m->chunked_size){
                    state_m->buf_idx+=(state_m->chunked_size-1);
                    state_m->curr_chunked_size+=state_m->chunked_size;
                    LOG(KB_PS, "Finish chunk, idx: %d (Parsed: %d)", state_m->buf_idx, state_m->curr_chunked_size);
                    state_m->chunked_size=0;
                    state_m->parsed_len=0;
                    state_m->use_chunked=0;
                    state_m->p_state=NEXT_CHUNKED;
                } else {
                    state_m->buf_idx=state_m->data_size;
                    state_m->chunked_size-=avail_data;
                    state_m->curr_chunked_size+=avail_data;
                    continue;
                }
            }
        }

        // check if header using content-length
        if(state_m->use_content_length){
            /*if(!(--state_m->content_length)){
                LOG(INFO, "[CL] Parsing process has been done. Total content length: %d bytes (%d KB)", state_m->total_content_length, state_m->total_content_length/1024);
                control_var->rcode=RCODE_FIN;
                // return control_var;
                break;
            }
            continue; // if not enough, then keep going
            */
            if(state_m->content_length>0){
                // move idx
                int avail_data=(state_m->data_size-state_m->buf_idx)+1;
                // printf("content_length: %d, avail: %d\n", state_m->content_length, avail_data);
                if(avail_data >= state_m->content_length){
                    state_m->buf_idx+=(state_m->content_length-1); // -1 (buf_idx)
                    state_m->content_length=0;
                    LOG(KB_PS, "[CL] Parsing process has been done. Total content length: %d bytes (%d KB)", state_m->total_content_length, state_m->total_content_length/1024);
                    control_var.rcode=RCODE_FIN;
                    break;
                } else {
                    // avail_data < state_m->content_length;
                    state_m->buf_idx=state_m->data_size;
                    state_m->content_length-=avail_data;
                }
                continue;
            }
        }

        // header will go to here, count length
        http_h_status_check->msg_hdr_len++;

        // read byte, check the byte
        switch(state_m->buff[state_m->buf_idx-1]){
            case '\r':
                // dealing with message header part
                if(state_m->p_state<MSG_BODY && state_m->parsed_len>1){
                    // parse last-element of START-LINE, or field-value
                    if(state_m->p_state>HEADER && state_m->p_state<MSG_BODY){
                        // header field-values
                        insert_new_header_field_value(http_h_status_check, state_m->buf_idx, state_m->parsed_len);
                    } else {
                        /** (last-element) start-line
                         * - Response: Reason phrase
                         */
                        /*char *tmp;
                        tmp=malloc((state_m->parsed_len)*sizeof(char));
                        snprintf(tmp, state_m->parsed_len, "%s", state_m->buff+(state_m->buf_idx-state_m->parsed_len));
                        LOG(INFO,  "[Reason Phrase: %s]", tmp);
                        free(tmp);*/
                    }
                    state_m->p_state=next_http_state(state_m->p_state, '\r');
                    state_m->parsed_len=0;
                } else if(state_m->p_state == REASON_OR_RESOURCE && state_m->parsed_len==1){
                    // special case (when server don't send reason phase), need to change it to HEADER here
                    state_m->p_state=next_http_state(state_m->p_state, '\r');
                }
                break;
            case '\n':
                if(state_m->p_state==HEADER){
                    state_m->p_state=next_http_state(state_m->p_state, '\n');
                } else if(state_m->p_state==FIELD_NAME && !state_m->use_chunked) {
                    /** Finished all headers, analyzing now */

                    state_m->p_state=MSG_BODY;
                    LOG(KB_PS,  "Message header length: %d", http_h_status_check->msg_hdr_len);
                    /** Version -
                     * - Not support HTTP/0.9, /2.0, /3.0 (e.g. http_ver==0)
                     */
                    if(!state_m->resp->http_ver) {
                        LOG(KB_PS, "Only support HTTP/1.0 and /1.1 now.");
                        control_var.rcode=RCODE_NOT_SUPPORT;
                        return control_var;
                    }

                    /** Status Code -
                     * - Skip 1xx
                     * - if 2xx, check:
                     *      - 200: keep processing
                     *      - ...
                     * - if 3xx, then we need to check header fields:
                     *      - 301: check `Location` field, if exist, return redirect_request error code;
                     *          if not exist, then we need to terminate this connection.
                     *      - 302: use effective request URI for future requests
                     *          (also using `Location` to perform redirect)
                     *      - ...
                     *
                     * - if 4xx or 5xx, then connection can be terminated; (FIXME: Can we terminate directly?)
                     *
                     */
                    if(state_m->resp->status_code<_200_OK){
                        // 1xx
                        LOG(KB_PS, "Response from server : %s (%s)", 
                            get_http_status_code_by_idx[state_m->resp->status_code], 
                            get_http_reason_phrase_by_idx[state_m->resp->status_code]);
                        // TODO:
                    } else if(state_m->resp->status_code>=_200_OK && state_m->resp->status_code<_300_MULTI_CHOICES){
                        // 2xx
                        LOG(KB_PS, "Response from server : %s (%s)", 
                            get_http_status_code_by_idx[state_m->resp->status_code], 
                            get_http_reason_phrase_by_idx[state_m->resp->status_code]);
                        // keep processing
                    } else if(state_m->resp->status_code>=_300_MULTI_CHOICES && state_m->resp->status_code<_400_BAD_REQUEST){
                        switch(state_m->resp->status_code){
                            case _301_MOVED_PERMANENTLY:
                            case _302_FOUND:{
                                // search `Location` field-value
                                char *loc=strndup(
                                    state_m->buff+1+(http_h_status_check->field_value[RES_LOC].idx), 
                                    http_h_status_check->field_value[RES_LOC].offset);
                                LOG(KB_PS, "Redirect to `Location`: %s, close the connection.", loc);
                                // redirect to new target (new conn + modified request)
                                // reuse http_request ptr (to store Location)
                                control_var.return_obj=loc;
                                control_var.rcode=RCODE_REDIRECT;
                                return control_var;
                            }
                            default:
                                // not support, keep processing
                                break;
                        }
                    } else if(state_m->resp->status_code>=_400_BAD_REQUEST && state_m->resp->status_code<=_505_HTTP_VER_NOT_SUPPORTED){
                        LOG(KB_PS, "Connection is terminated by %s's failure ...", state_m->resp->status_code<_500_INTERNAL_SERV_ERR?"client":"server");
                        LOG(KB_PS, "Response from server : %s (%s)", get_http_status_code_by_idx[state_m->resp->status_code], get_http_reason_phrase_by_idx[state_m->resp->status_code]);
                        // return error code, we don't need the parse the rest of data
                        control_var.rcode=state_m->resp->status_code<_500_INTERNAL_SERV_ERR? RCODE_CLIENT_ERR: RCODE_SERVER_ERR;
                        return control_var;
                    }

                    /** Transfer coding/Message body info -
                     * check current using Transfer-Encoding or Content-Length
                     */
                    if(http_h_status_check->content_len_dirty){
                        state_m->use_content_length=1;
                        state_m->content_length=atoi(state_m->buff+http_h_status_check->field_value[RES_CONTENT_LEN].idx);
                        LOG(KB_PS, "[Content length] size = %d", state_m->content_length);
                        state_m->total_content_length=state_m->content_length;
                        flag=(state_m->content_length==0) ? 0: flag;
                    } else if(http_h_status_check->transfer_encoding_dirty){
                        // need to parse under chunked size=0
                        state_m->p_state=CHUNKED;
                    }

                    /**
                     * FIXME: need to check the connection state (close, or keep-alive)
                     * - close: client side need to close the connection, and assume those sent pipelining requests have not been processed by server
                     * - keep-alive: calculate the "max" and "timeout" value fron `Keep-Alive` header
                     */

                } else if(state_m->p_state==CHUNKED){
                    // check if it is `chunked` (If no extension)
                    /** FIXME: need to using strtok to parse all possible value
                     */
                    if(http_h_status_check->use_chunked){
                        char *tmp=calloc(state_m->parsed_len, sizeof(char));
                        char *chunked_str=strndup(state_m->buff+(state_m->buf_idx-state_m->parsed_len), state_m->parsed_len);
                        sprintf(tmp, "%ld", strtol(chunked_str, NULL, 16));
                        if((atoi(tmp))>0){
                            state_m->chunked_size=atoi(tmp);
                            state_m->total_chunked_size+=state_m->chunked_size;
                            LOG(KB_PS, "[Get Chunk] size = %d", state_m->chunked_size);
                            state_m->p_state=CHUNKED; // don't need to check extension
                            state_m->use_chunked=1;
                        } else if(state_m->parsed_len==2){
                            // Case - chunked size=0
                            // FIXME: is this right condition?
                            state_m->chunked_size=atoi(tmp);
                            LOG(KB_PS, "[Last Chunk] size = %d", state_m->chunked_size);
                            // state_m->use_chunked=1; // let outside know 
                            flag=0; // exit
                            // state_m->use_chunked=1;
                        }
                        free(chunked_str);
                        free(tmp);
                    }
                } else if(state_m->p_state==CHUNKED_EXT){
                    // chunk extension goes here
                    char *chunked_ext=strndup(state_m->buff+(state_m->buf_idx-state_m->parsed_len), state_m->parsed_len);
                    LOG(KB_PS, "[Chunk-Ext] %s", chunked_ext);
                    state_m->p_state=CHUNKED;
                    /** TODO:
                     *  store the data into response obj
                    */
                    free(chunked_ext);
                } else if(state_m->p_state==NEXT_CHUNKED){
                    // finish one chunk, then change the state to CHUNKED again to keep parsing next chunked
                    state_m->p_state=CHUNKED;
                }
                state_m->parsed_len=0;
                break;
            case ':':
                // for parsing field-name
                if(state_m->p_state==FIELD_NAME && state_m->parsed_len>0){
                    state_m->p_state=next_http_state(state_m->p_state, RES);
                    /* check the return value, if return error, then terminate the parsing process */
                    insert_new_header_field_name(http_h_status_check, state_m->buf_idx, state_m->parsed_len);
                    state_m->parsed_len=0;
                }
                state_m->p_state=next_http_state(state_m->p_state, ':');
            case 0x09: // HTAB
            case ' ': // SP
                // parse only when in "start-line" state;
                if(state_m->p_state>START_LINE && state_m->p_state<REASON_OR_RESOURCE && state_m->parsed_len>0){
                    int ret=0;
                    switch (state_m->p_state)
                    {
                        case VER:
                            if((ret=encap_http_version(state_m->buff+(state_m->buf_idx-state_m->parsed_len))) > 1){ // current only support HTTP/1.1
                                LOG(KB_PS, "[Version: %s]",  get_http_version_by_idx[ret]);
                                state_m->resp->http_ver=ret;
                            } else { // if not support, just terminate
                                char *tmp=malloc((state_m->parsed_len)*sizeof(char));
                                snprintf(tmp, state_m->parsed_len, "%s", state_m->buff+(state_m->buf_idx-state_m->parsed_len));
                                LOG(KB_PS, "[Version not support: %s]", tmp);
                                free(tmp);
                                flag=0;
                                // means parse error, set rcode and return
                                control_var.rcode=RCODE_NOT_SUPPORT;
                                return control_var;
                            }
                            state_m->parsed_len=0;
                            break;
                        case CODE_OR_TOKEN:
                            if((ret=encap_http_status_code(atoi(state_m->buff+(state_m->buf_idx-state_m->parsed_len)))) > 0){
                                state_m->resp->status_code=ret;
                                LOG(KB_PS, "[Status code: %s]", get_http_status_code_by_idx[ret]);
                            } else { // not support
                                control_var.rcode=RCODE_NOT_SUPPORT;
                                return control_var;
                            }
                            state_m->parsed_len=0;
                            break;
                        default:
                            break;
                    }
                    // change the state
                    state_m->p_state=next_http_state(state_m->p_state, ' ');
                    break;
                } else if(state_m->p_state==CHUNKED){
                    // need to parse the chunk size here, and prepare to parse chunk extension
                    if( http_h_status_check->use_chunked ){
                        char *tmp=calloc(state_m->parsed_len, sizeof(char));
                        char *chunked_str=strndup(state_m->buff+(state_m->buf_idx-state_m->parsed_len), state_m->parsed_len);
                        sprintf(tmp, "%ld", strtol(chunked_str, NULL, 16));
                        if((atoi(tmp))>0){
                            state_m->chunked_size=atoi(tmp);
                            LOG(KB_PS, "[Get Chunk] size = %d", state_m->chunked_size);
                            state_m->p_state=CHUNKED_EXT;
                            // use_chunked=1;
                        } else if(state_m->parsed_len==2){
                            // Case - chunked size=0
                            // FIXME: is this right condition?
                            state_m->chunked_size=atoi(tmp);
                            LOG(KB_PS, "[Last Chunk] size = %d", state_m->chunked_size);
                            state_m->p_state=CHUNKED_EXT;
                            // use_chunked=1;
                        }
                        free(chunked_str);
                        free(tmp);
                    }
                } /* CHUNKED (if extension is valid) */
            default:
                // append to readbuf
                /* conformance check: https://tools.ietf.org/html/rfc7230#section-3.1 */
                switch(state_m->p_state){
                    case REASON_OR_RESOURCE: // in reason phrase
                        if( !isWSP(state_m->buff[state_m->buf_idx-1]) && 
                            !isVCHAR(state_m->buff[state_m->buf_idx-1]) &&
                            !isOBS_TEXT(state_m->buff[state_m->buf_idx-1])){
                                // error 
                                LOG(KB_PS, "Invalid char: %d\n", state_m->buff[state_m->buf_idx-1]);
                                LOG(KB_PS, "Illegal Reason Phrase");
                                exit(1); 
                            }
                        break;
                    case FIELD_NAME:
                    case FIELD_VALUE:
                        if( !isTCHAR(state_m->buff[state_m->buf_idx-1]) &&
                            !isQSTR(state_m->buff[state_m->buf_idx-1])){
                            // error 
                            LOG(KB_PS, "Illegal Field Name/Value");
                            LOG(KB_PS, "Invalid char: %d\n", state_m->buff[state_m->buf_idx-1]);
                        }
                        break;
                    default:
                        break;
                }
        }
    }

    // finish one request
    control_var.rcode=RCODE_FIN;
    if(state_m->buf_idx <= state_m->data_size){
        // need to call parser again 
        control_var.rcode=RCODE_NEXT_RESP;
    }
    return control_var;
}

state_machine_t *
create_parsing_state_machine()
{
    state_machine_t *new_obj = calloc(1, sizeof(state_machine_t));
    new_obj->buff=calloc(RECV_BUFF_SCALE*CHUNK_SIZE, sizeof(char));
    new_obj->resp=create_http_header_status(new_obj->buff);
    new_obj->max_buff_size=RECV_BUFF_SCALE*CHUNK_SIZE;
    return new_obj;
}

void 
reset_parsing_state_machine(
    state_machine_t *state_m)
{
    // state_m->resp=create_http_header_status(state_m->buff);
    memset(state_m->resp, 0x00, 1*sizeof(http_res_header_status_t));
    memset(state_m->buff, 0x00, state_m->max_buff_size*sizeof(char));
    state_m->resp->msg_hdr_len=0;
    state_m->resp->buff=state_m->buff;

    state_m->p_state=VER;
    state_m->resp->status_code=0;
    state_m->resp->http_ver=0;

    state_m->last_fin_idx=0;
    state_m->prev_rcv_len=0;
    state_m->buf_idx=0;
    state_m->data_size=0;
    
    state_m->use_content_length=0;
    state_m->use_chunked=0;
    state_m->content_length=0;
    state_m->total_content_length=0;
    state_m->curr_chunked_size=0;
    state_m->parsed_len=0;
}

// current only use in Response
http_state
next_http_state(http_state cur_state, char ch)
{
    switch(cur_state)
    {
        // start-line
        case START_LINE:
            return VER;
        case VER:
            switch(ch){
                case ' ':
                    return CODE_OR_TOKEN;
                default:
                    return VER;
            }
        case CODE_OR_TOKEN:
            switch(ch){
                case ' ':
                    return REASON_OR_RESOURCE;
                case '\r':
                case '\n':
                    return HEADER;
                default:
                    return CODE_OR_TOKEN;
            }
        case REASON_OR_RESOURCE:
            switch(ch){
                case ' ':
                    return REASON_OR_RESOURCE;
                case '\r':
                case '\n':
                default:
                    return HEADER;
            }
        // header
        case HEADER:
            switch(ch){
                case '\r':
                case '\n':
                    return FIELD_NAME;
                default:
                    return HEADER;
            }
        case FIELD_NAME:
            switch(ch){
                case ':':
                    return FIELD_VALUE;
                case '\r':
                    return MSG_BODY;
                default:
                    return FIELD_NAME;
            }
        case FIELD_VALUE:
            switch(ch){
                case '\r':
                    return HEADER;
                default:
                    return FIELD_VALUE;
            }
        case MSG_BODY:
            return MSG_BODY;
        case CHUNKED:
            return CHUNKED;
        default:
            return cur_state;
    }
}