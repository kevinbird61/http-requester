#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "http.h"

state_machine_t *
create_parsing_state_machine()
{
    state_machine_t *new_obj = calloc(1, sizeof(state_machine_t));
    new_obj->buff=calloc(RECV_BUFF_SCALE*CHUNK_SIZE, sizeof(char));
    new_obj->max_buff_size=RECV_BUFF_SCALE*CHUNK_SIZE;
    return new_obj;
}

control_var_t *
multi_bytes_http_parsing_state_machine(
    int sockfd,
    u32 num_reqs)
{
    /* create objs */
    state_machine_t *state_m=create_parsing_state_machine();
    /* get all the data:
     * - poll the data until connection state is TCP_CLOSE_WAIT, and recvbytes is 0 
     */
    /* stats */
    int flag=1, recvbytes=0;
    state_m->resp=create_http_header_status(state_m->buff);
    state_m->p_state=VER;
    state_m->resp->status_code=0;
    state_m->resp->http_ver=0;

    /* Parse the data, and store the result into respones objs */
    /* FIXME: enhancement - need to free buffer (e.g. finished resp) 
     *  to prevent occupying too much memory ?
     */
    control_var_t *control_var;
    while(flag){
        // call parser (store the state and response obj into state_machine's instance)
        control_var=http_resp_parser(state_m);
        switch (control_var->rcode)
        {
            case RCODE_POLL_DATA:
                if(!num_reqs){
                    flag=0;
                    break;
                }
                /* require another recv() to poll new data */
                // check upperbound & move (prevent using too many realloc, and too much buff length)
                // if( (strlen(state_m->buff)) > ((RECV_BUFF_SCALE-1)*CHUNK_SIZE) ){
                if( state_m->data_size > ((RECV_BUFF_SCALE-1)*CHUNK_SIZE) ){
                    // then move the leftover to the front, also need to reset buf_idx;
                    state_m->data_size=state_m->data_size-state_m->last_fin_idx;
                    if(state_m->use_content_length){
                        state_m->data_size=0; // if parsing "content length" (chunked need to consider more case), we don't care the data_size (just set to 0, means that all parsed content has been dropped -> we don't store it currently)
                    }
                    // need to adjust the offset of each response obj
                    update_res_header_idx(state_m->resp, state_m->last_fin_idx);
                    LOG(NORMAL ,"( Prevbytes: %d ) Last fin idx: %d, Move %d bytes", state_m->prev_rcv_len, state_m->last_fin_idx, state_m->data_size);
                    state_m->buf_idx=state_m->data_size;
                    memcpy(state_m->buff, state_m->buff+state_m->last_fin_idx, state_m->data_size);
                    // reset the rest and fin_idx
                    memset(state_m->buff+state_m->data_size, 0x00, state_m->max_buff_size-state_m->data_size);
                    state_m->last_fin_idx=0; 
                }
                
                recvbytes=recv(sockfd, state_m->buff+state_m->data_size, CHUNK_SIZE, 0);
                state_m->prev_rcv_len=recvbytes;
                
                if(recvbytes==0 && get_tcp_conn_stat(sockfd)==TCP_CLOSE_WAIT){
                    // puts(state_m->buff);
                    // if(state_m->buf_idx < strlen(state_m->buff)){
                    if(state_m->buf_idx < state_m->data_size){
                        LOG(NORMAL, "Keep parsing: %d (%ld)(%d)", state_m->buf_idx, strlen(state_m->buff), state_m->data_size);
                        STATS_DUMP();
                        break;
                        // exit(1); // debug
                    } else {
                        LOG(NORMAL, "Finish all response parsing, buf_idx=%d, strlen(buff)=%ld", state_m->buf_idx, strlen(state_m->buff));
                        // STATS_DUMP();
                        flag=0;
                        // exit(1);
                    }
                }
                state_m->data_size+=recvbytes;
                /*if(state_m->data_size>=(state_m->max_buff_size-CHUNK_SIZE)){
                    state_m->buff=realloc(state_m->buff, (state_m->max_buff_size+=CHUNK_SIZE));
                    state_m->resp->buff=state_m->buff; // set resp buff to state_m->buff
                }*/
                // state_m->parsed_len=0; // because current parse_len is incomplete, need to reset
                break;
            case RCODE_REDIRECT:
                /* perform redirection, notify caller that need to abort the response and resend */
                LOG(WARNING, "Require rediretion.");
                // return control_var;
                flag=0;
                break;
            case RCODE_ERROR:
                // print the error message instead of exit?
                LOG(WARNING, "Error occur.");
                // return control_var;
                flag=0;
                break;
            case RCODE_FIN:
            case RCODE_NEXT_RESP:
                LOG(WARNING, "Finish one respose.");
                control_var->num_resp++;
                num_reqs--; // finish one response
                /* update fin_idx */
                state_m->last_fin_idx=state_m->buf_idx;

                // Finish one resp, and need to parse next:
                // - create a new resp obj (TODO: maintain a response queue to store
                //  all the parsed responses.)
                // - we don't modify the buf_idx, just pass buff ptr to response obj.
                // - reset the essential states, prepare for parsing next response.
                state_m->resp=create_http_header_status(state_m->buff);
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
            default:
                LOG(ERROR, "Unknown: %d", control_var->rcode);
                exit(1); // should terminate.
        }
    }

    // dump the statistics
    STATS_DUMP();
    // output the statistics
    LOG(NORMAL, "TCP connection state: %s", tcpi_state_str[get_tcp_conn_stat(sockfd)] );
    LOG(NORMAL, "Total received: %ld bytes (data size: %d bytes, max buff size: %d bytes)",
        strlen(state_m->buff), state_m->data_size, state_m->max_buff_size);

    free(state_m->buff);

    return control_var;
}

control_var_t *
http_resp_parser(
    state_machine_t *state_m)
{
    // parsing status are stored in state_m, setup control_var to determine the next action
    u8 flag=1;
    http_res_header_status_t *http_h_status_check=state_m->resp;
    control_var_t *control_var=calloc(1, sizeof(control_var_t));

    while(flag){
        // store legal char
        state_m->buf_idx++;
        state_m->parsed_len++;
        // check if we need more data or not
        if(state_m->buf_idx>state_m->data_size){
            /* require more data */
            state_m->buf_idx--;
            state_m->parsed_len--;
            control_var->rcode=RCODE_POLL_DATA;
            return control_var;
        }
        /** Put some checking mechanism to terminate parsing
         * state machine before recipient send FIN back (timeout).
         */
        if(state_m->use_chunked && state_m->chunked_size==0){
            // printf("END, idx: %d\n", state_m->buf_idx-1);
            LOG(INFO, "[Transfer-Encoding: Chunked] Parsing process has been done. Total chunked size: %d bytes (%d KB)", state_m->total_chunked_size, state_m->total_chunked_size/1024);
            control_var->rcode=RCODE_FIN;
            break;
        }

        // check if header using content-length
        if(state_m->use_content_length){
            if(!(--state_m->content_length)){
                LOG(INFO, "[CL] Parsing process has been done. Total content length: %d bytes (%d KB)", state_m->total_content_length, state_m->total_content_length/1024);
                control_var->rcode=RCODE_FIN;
                break;
            }
            continue; // if not enough, then keep going
        }

        // check if header using transfer-encoding
        if(state_m->use_chunked){
            if(!(--state_m->chunked_size)){
                state_m->curr_chunked_size++;
                LOG(INFO, "Finish chunk, idx: %d (Parsed: %d)\n", state_m->buf_idx-1, state_m->curr_chunked_size);
                state_m->parsed_len=0;
                state_m->use_chunked=0;
                state_m->p_state=NEXT_CHUNKED;
            } else {
                /* Parsing other chunk features here */
                state_m->curr_chunked_size++;
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
                        char *tmp;
                        tmp=malloc((state_m->parsed_len)*sizeof(char));
                        snprintf(tmp, state_m->parsed_len, "%s", state_m->buff+(state_m->buf_idx-state_m->parsed_len));
                        LOG(DEBUG,  "[Reason Phrase: %s]", tmp);
                        free(tmp);
                    }
                    state_m->p_state=next_http_state(state_m->p_state, '\r');
                    state_m->parsed_len=0;
                }
                break;
            case '\n':
                if(state_m->p_state==HEADER){
                    state_m->p_state=next_http_state(state_m->p_state, '\n');
                } else if(state_m->p_state==FIELD_NAME && !state_m->use_chunked) {
                    /** Finished all headers, analyzing now */

                    state_m->p_state=MSG_BODY;
                    LOG(INFO,  "Message header length: %d", http_h_status_check->msg_hdr_len);
                    /** Version -
                     * - Not support HTTP/0.9, /2.0, /3.0 (e.g. http_ver==0)
                     */
                    if(!state_m->resp->http_ver) {
                        LOG(ERROR, "Only support HTTP/1.0 and /1.1 now.");
                        control_var->rcode=RCODE_NOT_SUPPORT;
                        exit(1);
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
                    STATS_INC_CODE(state_m->resp->status_code);
                    if(state_m->resp->status_code<_200_OK){
                        // 1xx
                        LOG(INFO, "Response from server : %s (%s)", 
                            get_http_status_code_by_idx[state_m->resp->status_code], 
                            get_http_reason_phrase_by_idx[state_m->resp->status_code]);
                        // TODO:
                    } else if(state_m->resp->status_code>=_200_OK && state_m->resp->status_code<_300_MULTI_CHOICES){
                        // 2xx
                        LOG(INFO, "Response from server : %s (%s)", 
                            get_http_status_code_by_idx[state_m->resp->status_code], 
                            get_http_reason_phrase_by_idx[state_m->resp->status_code]);
                        // keep processing
                    } else if(state_m->resp->status_code>=_300_MULTI_CHOICES && state_m->resp->status_code<_400_BAD_REQUEST){
                        switch(state_m->resp->status_code){
                            case _301_MOVED_PERMANENTLY:
                            case _302_FOUND:
                                // search `Location` field-value
                                LOG(INFO, "Redirect to `Location`: %s, close the connection.", 
                                    strndup(
                                        state_m->buff+1+(http_h_status_check->field_value[RES_LOC].idx), 
                                        http_h_status_check->field_value[RES_LOC].offset));
                                // close the connection
                                // close(sockfd);
                                // redirect to new target (new conn + modified request)
                                // reuse http_request ptr (to store Location)
                                control_var->return_obj=
                                    strndup(
                                        state_m->buff+1+(http_h_status_check->field_value[RES_LOC].idx), 
                                        http_h_status_check->field_value[RES_LOC].offset); 
                                control_var->rcode=RCODE_REDIRECT;
                                return control_var;
                                // exit(1);
                            default:
                                // not support, keep processing
                                break;
                        }
                    } else if(state_m->resp->status_code>=_400_BAD_REQUEST && state_m->resp->status_code<=_505_HTTP_VER_NOT_SUPPORTED){
                        LOG(DANGER, "Connection is terminated by %s's failure ...", state_m->resp->status_code<_500_INTERNAL_SERV_ERR?"client":"server");
                        LOG(ERROR, "Response from server : %s (%s)", get_http_status_code_by_idx[state_m->resp->status_code], get_http_reason_phrase_by_idx[state_m->resp->status_code]);
                        // terminate, we don't need the parse the rest of data
                        // close(sockfd);
                        // FIXME: return error code instead terminate directly
                        control_var->rcode=RCODE_ERROR;
                        return control_var;
                        // exit(1);
                    }

                    /** Transfer coding/Message body info -
                     * check current using Transfer-Encoding or Content-Length
                     */
                    // if(http_h_status_check->dirty_bit_align&(1<<(RES_CONTENT_LEN-1))){
                    if(http_h_status_check->content_len_dirty){
                        // printf("%d\n", http_h_status_check->content_len_dirty);
                        state_m->use_content_length=1;
                        state_m->content_length=atoi(state_m->buff+http_h_status_check->field_value[RES_CONTENT_LEN].idx);
                        LOG(NORMAL, "[Content length] size = %d", state_m->content_length);
                        state_m->total_content_length=state_m->content_length;
                        if(state_m->content_length==0){
                            flag=0;
                        }
                    } else if(http_h_status_check->transfer_encoding_dirty){
                        // need to parse under chunked size=0
                        // printf("%d, %d\n", http_h_status_check->transfer_encoding_dirty, http_h_status_check->spare2);
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
                    if(!strncasecmp(strndup(state_m->buff+1+(http_h_status_check->field_value[RES_TRANSFER_ENCODING].idx), http_h_status_check->field_value[RES_TRANSFER_ENCODING].offset), "chunked", 7)){
                        char *tmp=calloc(state_m->parsed_len, sizeof(char));
                        sprintf(tmp, "%ld", strtol(strndup(state_m->buff+(state_m->buf_idx-state_m->parsed_len), state_m->parsed_len), NULL, 16));
                        if((atoi(tmp))>0){
                            state_m->chunked_size=atoi(tmp);
                            state_m->total_chunked_size+=state_m->chunked_size;
                            LOG(INFO, "[Get Chunk] size = %d", state_m->chunked_size);
                            state_m->p_state=CHUNKED; // don't need to check extension
                            state_m->use_chunked=1;
                        } else if(state_m->parsed_len==2){
                            // Case - chunked size=0
                            // FIXME: is this right condition?
                            state_m->chunked_size=atoi(tmp);
                            LOG(INFO, "[Last Chunk] size = %d", state_m->chunked_size);
                            state_m->use_chunked=1;
                        }
                        free(tmp);
                    }
                } else if(state_m->p_state==CHUNKED_EXT){
                    // chunk extension goes here
                    LOG(INFO, "[Chunk-Ext] %s", strndup(state_m->buff+(state_m->buf_idx-state_m->parsed_len), state_m->parsed_len));
                    state_m->p_state=CHUNKED;
                    /** TODO:
                     *  store the data into response obj
                    */
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
                    //fwrite(readbuf+1+(state_m->buf_idx-state_m->parsed_len), sizeof(char), state_m->parsed_len-2, stdout);
                    /* check the return value, if return error, then terminate the parsing process */
                    insert_new_header_field_name(http_h_status_check, state_m->buf_idx, state_m->parsed_len);
                    state_m->parsed_len=0;
                }
                state_m->p_state=next_http_state(state_m->p_state, ':');
            case ' ':
                // parse only when in "start-line" state;
                if(state_m->p_state>START_LINE && state_m->p_state<REASON_OR_RESOURCE && state_m->parsed_len>0){
                    //fwrite(readbuf+(state_m->buf_idx-state_m->parsed_len), sizeof(char), state_m->parsed_len, stdout);
                    int ret=0;
                    switch (state_m->p_state)
                    {
                        case VER:
                            // printf("%d\n", encap_http_version(readbuf+(state_m->buf_idx-state_m->parsed_len)));
                            if((ret=encap_http_version(state_m->buff+(state_m->buf_idx-state_m->parsed_len))) > 1){ // current only support HTTP/1.1
                                LOG(DEBUG, "[Version: %s]",  get_http_version_by_idx[ret]);
                                state_m->resp->http_ver=ret;
                            } else { // if not support, just terminate
                                char *tmp=malloc((state_m->parsed_len)*sizeof(char));
                                snprintf(tmp, state_m->parsed_len, "%s", state_m->buff+(state_m->buf_idx-state_m->parsed_len));
                                LOG(ERROR, "[Version not support: %s]", tmp);
                                free(tmp);
                                flag=0;
                                // means parse error, set rcode and return
                                control_var->rcode=RCODE_NOT_SUPPORT;
                                return control_var;
                            }
                            state_m->parsed_len=0;
                            break;
                        case CODE_OR_TOKEN:
                            // printf("%d\n", encap_http_status_code(atoi(readbuf+(state_m->buf_idx-state_m->parsed_len))));
                            if((ret=encap_http_status_code(atoi(state_m->buff+(state_m->buf_idx-state_m->parsed_len)))) > 0){
                                state_m->resp->status_code=ret;
                                LOG(DEBUG, "[Status code: %s]", get_http_status_code_by_idx[ret]);
                            } else { // not support
                                // char *tmp=malloc((state_m->parsed_len)*sizeof(char));
                                // snprintf(tmp, state_m->parsed_len, "%s", readbuf+(state_m->buf_idx-state_m->parsed_len));
                                // free(tmp);
                                control_var->rcode=RCODE_NOT_SUPPORT;
                                return control_var;
                            }
                            state_m->parsed_len=0;
                            break;
                        default:
                            break;
                    }
                    // change the state
                    state_m->p_state=next_http_state(state_m->p_state, ' ');
                    // printf("Pstate: %d\n", state_m->p_state);
                    break;
                } else if(state_m->p_state==CHUNKED){
                    // need to parse the chunk size here, and prepare to parse chunk extension
                    if( !strncasecmp(
                            strndup(state_m->buff+1+(http_h_status_check->field_value[RES_TRANSFER_ENCODING].idx),
                            http_h_status_check->field_value[RES_TRANSFER_ENCODING].offset),
                            "chunked", strlen("chunked")) ){

                        char *tmp=calloc(state_m->parsed_len, sizeof(char));
                        sprintf(tmp, "%ld", strtol(strndup(state_m->buff+(state_m->buf_idx-state_m->parsed_len), state_m->parsed_len), NULL, 16));
                        if((atoi(tmp))>0){
                            // printf("%s, %s\n", tmp, strndup(readbuf+(state_m->buf_idx-state_m->parsed_len), state_m->parsed_len));
                            state_m->chunked_size=atoi(tmp);
                            LOG(INFO, "[Get Chunk] size = %d", state_m->chunked_size);
                            state_m->p_state=CHUNKED_EXT;
                            // use_chunked=1;
                        } else if(state_m->parsed_len==2){
                            // Case - chunked size=0
                            // FIXME: is this right condition?
                            state_m->chunked_size=atoi(tmp);
                            LOG(INFO, "[Last Chunk] size = %d", state_m->chunked_size);
                            state_m->p_state=CHUNKED_EXT;
                            // use_chunked=1;
                        }
                        free(tmp);
                    }
                } /* CHUNKED (if extension is valid) */
            default:
                // append to readbuf
                break;
        }
    }

    // LOG(INFO, "Total received: %d bytes", state_m->buf_idx); // buf_idx can be rotated, so currently it is not correct in this func
    // printf("Parsed: %d (Total data length: %ld)\n", state_m->buf_idx, strlen(state_m->buff));
    control_var->rcode=RCODE_FIN;
    //control_var->rcode=RCODE_POLL_DATA;
    if(state_m->buf_idx < strlen(state_m->buff)){
        // need to call parser again 
        control_var->rcode=RCODE_NEXT_RESP;
    }
    return control_var;
}

int
http_rcv_state_machine(
    int sockfd,
    void **return_obj)
{
    /** stats and vars:
     * - flag:          control the end of state machine.
     * - buf_size:      initial size of RX buffer
     * - chunk:         when RX buffer size is full, allocate a new buffer sized `chunk` to Rx
     * - buf_idx:       points to the latest index, increase when store a new char into RX buffer.
     * - parse_len:     record the length of current meaningful data
     */
    u8 flag=1;
    int buf_size=64, chunk=64, buf_idx=0, parse_len=0;
    char bytebybyte, *bufbybuf;
    char *readbuf=calloc(buf_size, sizeof(char)); // pre-allocated 32 bytes
    if(readbuf==NULL){
        LOG(ERROR, "MALLOC error when alloc to *readbuf. Current buf_size: %d", buf_size);
        exit(1);
    }

    // maintain parsing status
    http_res_header_status_t *http_h_status_check=create_http_header_status(readbuf);
    u8 state=VER; // start with version
    u8 status_code=0;
    u8 http_ver=0;
    u8 use_content_length=0, use_chunked=0;
    u32 content_length=0, chunked_size=0, total_content_length=0, total_chunked_size=0;

    // scenario 1: byte-by-byte reading
    while(flag){
        /** Put some checking mechanism to terminate parsing
         * state machine before recipient send FIN back (timeout).
         */
        if(use_chunked && chunked_size==0){
            // printf("END, idx: %d\n", buf_idx-1);
            printf("[Transfer-Encoding: Chunked] Parsing process has been done. Total chunked size: %d bytes (%d KB)\n", total_chunked_size, total_chunked_size/1024);
            break;
        }
        /* recv byte by byte */
        if(!recv(sockfd, &bytebybyte, 1, 0)){
            break;
        }
        // store legal char
        readbuf[buf_idx]=bytebybyte;
        buf_idx++;
        parse_len++;

        // check size, if not enough, then realloc
        if(buf_idx==buf_size){
            buf_size+=chunk;
            readbuf=realloc(readbuf, buf_size*sizeof(char));
            http_h_status_check->buff=readbuf;
            if(readbuf==NULL){
                LOG(ERROR, "REALLOC failure when realloc to *readbuf, check the memory. Current buf_size: %d", buf_size);
                exit(1);
            }
            // if you feel this message is too noisy, can comment this line or increase chunk size
            // syslog("DEBUG", __func__, "REALLOC readbuf size. Current buf_size: ", itoa(buf_size), NULL);
        }

        // check if header using content-length
        if(use_content_length){
            // --content_length;
            if(!(--content_length)){
                printf("[CL] Parsing process has been done. Total content length: %d bytes (%d KB)\n", total_content_length, total_content_length/1024);
                break;
            }
        }

        // check if header using transfer-encoding
        if(use_chunked){
            if(!(--chunked_size)){
                printf("Finish chunk, idx: %d (Parsed: %d)\n", buf_idx-1, parse_len);
                parse_len=0;
                use_chunked=0;
                // state=FIELD_NAME;
                state=NEXT_CHUNKED;
            } else {
                /* Parsing other chunk features here */
                continue;
            }
        }

        // read byte, check the byte
        switch(bytebybyte){
            case '\r':
                // dealing with message header part
                if(state<MSG_BODY && parse_len>1){
                    // parse last-element of START-LINE, or field-value
                    if(state>HEADER && state<MSG_BODY){
                        // header field-values
                        insert_new_header_field_value(http_h_status_check, buf_idx, parse_len);
                    } else {
                        /** (last-element) start-line
                         * - Response: Reason phrase
                         */
                        char *tmp;
                        tmp=malloc((parse_len)*sizeof(char));
                        snprintf(tmp, parse_len, "%s", readbuf+(buf_idx-parse_len));
                        LOG(INFO,  "[Reason Phrase: %s]", tmp);
                        free(tmp);
                    }
                    state=next_http_state(state, '\r');
                    parse_len=0;
                }
                break;
            case '\n':
                if(state==HEADER){
                    state=next_http_state(state, '\n');
                } else if(state==FIELD_NAME && !use_chunked) {
                    /** Finished all headers, analyzing now */

                    state=MSG_BODY;
                    LOG(INFO,  "Message header length: %d", buf_idx);
                    /** Version -
                     * - Not support HTTP/0.9, /2.0, /3.0 (e.g. http_ver==0)
                     */
                    if(!http_ver) {
                        LOG(ERROR, "Only support HTTP/1.0 and /1.1 now.");
                        fprintf(stderr, "Only support HTTP/1.0 and /1.1 now.");
                        exit(1);
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
                    STATS_INC_CODE(status_code);
                    if(status_code<_200_OK){
                        // 1xx
                        LOG(WARNING, "Response from server : %s (%s)", get_http_status_code_by_idx[status_code], get_http_reason_phrase_by_idx[status_code]);
                        // TODO:
                    } else if(status_code>=_200_OK && status_code<_300_MULTI_CHOICES){
                        // 2xx
                        LOG(NORMAL, "Response from server : %s (%s)", get_http_status_code_by_idx[status_code], get_http_reason_phrase_by_idx[status_code]);
                        // keep processing
                    } else if(status_code>=_300_MULTI_CHOICES && status_code<_400_BAD_REQUEST){
                        switch(status_code){
                            case _301_MOVED_PERMANENTLY:
                            case _302_FOUND:
                                // search `Location` field-value
                                printf("Redirect to `Location`: %s\n", strndup(readbuf+1+(http_h_status_check->field_value[RES_LOC].idx), http_h_status_check->field_value[RES_LOC].offset));
                                LOG(INFO, "Redirect to `Location`: %s, close the connection.", strndup(readbuf+1+(http_h_status_check->field_value[RES_LOC].idx), http_h_status_check->field_value[RES_LOC].offset));
                                // close the connection
                                close(sockfd);
                                // redirect to new target (new conn + modified request)
                                // reuse http_request ptr (to store Location)
                                *return_obj=strndup(readbuf+1+(http_h_status_check->field_value[RES_LOC].idx), http_h_status_check->field_value[RES_LOC].offset); // -2: delete CRLF
                                return ERR_REDIRECT;
                                // exit(1);
                            default:
                                // not support, keep processing
                                break;
                        }
                    } else if(status_code>=_400_BAD_REQUEST && status_code<=_505_HTTP_VER_NOT_SUPPORTED){
                        printf("Connection is terminated by %s's failure ...\n", status_code<_500_INTERNAL_SERV_ERR?"client":"server");
                        printf("Response from server: %s, %s\n", get_http_status_code_by_idx[status_code], get_http_reason_phrase_by_idx[status_code]);
                        LOG(ERROR, "Response from server : %s (%s)", get_http_status_code_by_idx[status_code], get_http_reason_phrase_by_idx[status_code]);
                        // terminate, we don't need the parse the rest of data
                        close(sockfd);
                        // FIXME: return error code instead terminate directly
                        exit(1);
                    }

                    /** Transfer coding/Message body info -
                     * check current using Transfer-Encoding or Content-Length
                     */
                    // if(http_h_status_check->dirty_bit_align&(1<<(RES_CONTENT_LEN-1))){
                    if(http_h_status_check->content_len_dirty){
                        // printf("%d\n", http_h_status_check->content_len_dirty);
                        use_content_length=1;
                        content_length=atoi(readbuf+http_h_status_check->field_value[RES_CONTENT_LEN].idx);
                        printf("Get content length= %d\n", content_length);
                        total_content_length=content_length;
                        if(content_length==0){
                            flag=0;
                        }
                    } else if(http_h_status_check->transfer_encoding_dirty){
                        // need to parse under chunked size=0
                        // printf("%d, %d\n", http_h_status_check->transfer_encoding_dirty, http_h_status_check->spare2);
                        state=CHUNKED;
                    }

                    /**
                     * FIXME: need to check the connection state (close, or keep-alive)
                     * - close: client side need to close the connection, and assume those sent pipelining requests have not been processed by server
                     * - keep-alive: calculate the "max" and "timeout" value fron `Keep-Alive` header
                     */

                } else if(state==CHUNKED){
                    // check if it is `chunked` (If no extension)
                    /** FIXME: need to using strtok to parse all possible value
                     */
                    if(!strncasecmp(strndup(readbuf+1+(http_h_status_check->field_value[RES_TRANSFER_ENCODING].idx), http_h_status_check->field_value[RES_TRANSFER_ENCODING].offset), "chunked", 7)){
                        char *tmp=calloc(parse_len, sizeof(char));
                        sprintf(tmp, "%ld", strtol(strndup(readbuf+(buf_idx-parse_len), parse_len), NULL, 16));
                        if((atoi(tmp))>0){
                            // printf("%s, %s\n", tmp, strndup(readbuf+(buf_idx-parse_len), parse_len));
                            chunked_size=atoi(tmp);
                            printf("[Get Chunk] Chunk size: %d\n", chunked_size);
                            total_chunked_size+=chunked_size;
                            LOG(DEBUG, "Get Chunk, size = %d", chunked_size);
                            state=CHUNKED; // don't need to check extension
                            use_chunked=1;
                        } else if(parse_len==2){
                            // Case - chunked size=0
                            // FIXME: is this right condition?
                            chunked_size=atoi(tmp);
                            printf("[Last Chunk] Chunk size: %d\n", chunked_size);
                            LOG(DEBUG, "Last Chunk, size = %d", chunked_size);
                            use_chunked=1;
                        }
                        free(tmp);
                    }
                } else if(state==CHUNKED_EXT){
                    // chunk extension goes here
                    printf("[Chunk-Ext] %s\n", strndup(readbuf+(buf_idx-parse_len), parse_len));
                    state=CHUNKED;
                    /** TODO:
                     *  store the data into response obj
                    */
                } else if(state==NEXT_CHUNKED){
                    // finish one chunk, then change the state to CHUNKED again to keep parsing next chunked
                    state=CHUNKED;
                }
                parse_len=0;
                break;
            case ':':
                // for parsing field-name
                if(state==FIELD_NAME && parse_len>0){
                    state=next_http_state(state, RES);
                    //fwrite(readbuf+1+(buf_idx-parse_len), sizeof(char), parse_len-2, stdout);
                    /* check the return value, if return error, then terminate the parsing process */
                    insert_new_header_field_name(http_h_status_check, buf_idx, parse_len);
                    parse_len=0;
                }
                state=next_http_state(state, ':');
            case ' ':
                // parse only when in "start-line" state;
                if(state>START_LINE && state<REASON_OR_RESOURCE && parse_len>0){
                    //fwrite(readbuf+(buf_idx-parse_len), sizeof(char), parse_len, stdout);
                    int ret=0;
                    switch (state)
                    {
                        case VER:
                            // printf("%d\n", encap_http_version(readbuf+(buf_idx-parse_len)));
                            if((ret=encap_http_version(readbuf+(buf_idx-parse_len))) > 1){ // current only support HTTP/1.1
                                LOG(DEBUG, "[Version: %s]",  get_http_version_by_idx[ret]);
                                http_ver=ret;
                            } else { // if not support, just terminate
                                char *tmp=malloc((parse_len)*sizeof(char));
                                snprintf(tmp, parse_len, "%s", readbuf+(buf_idx-parse_len));
                                LOG(ERROR, "[Version not support: %s]", tmp);
                                free(tmp);
                                flag=0;
                            }
                            parse_len=0;
                            break;
                        case CODE_OR_TOKEN:
                            // printf("%d\n", encap_http_status_code(atoi(readbuf+(buf_idx-parse_len))));
                            if((ret=encap_http_status_code(atoi(readbuf+(buf_idx-parse_len)))) > 0){
                                status_code=ret;
                                LOG(DEBUG, "[Status code: %s]", get_http_status_code_by_idx[ret]);
                            } else { // not support
                                // char *tmp=malloc((parse_len)*sizeof(char));
                                // snprintf(tmp, parse_len, "%s", readbuf+(buf_idx-parse_len));
                                // free(tmp);
                            }
                            parse_len=0;
                            break;
                        default:
                            break;
                    }
                    // change the state
                    state=next_http_state(state, ' ');
                    break;
                } else if(state==CHUNKED){
                    // need to parse the chunk size here, and prepare to parse chunk extension
                    if( !strncasecmp(
                        strndup(readbuf+1+(http_h_status_check->field_value[RES_TRANSFER_ENCODING].idx),
                        http_h_status_check->field_value[RES_TRANSFER_ENCODING].offset),
                        "chunked", strlen("chunked")) ){

                        char *tmp=calloc(parse_len, sizeof(char));
                        sprintf(tmp, "%ld", strtol(strndup(readbuf+(buf_idx-parse_len), parse_len), NULL, 16));
                        if((atoi(tmp))>0){
                            // printf("%s, %s\n", tmp, strndup(readbuf+(buf_idx-parse_len), parse_len));
                            chunked_size=atoi(tmp);
                            printf("[Get Chunk] Chunk size: %d\n", chunked_size);
                            LOG(DEBUG, "Get Chunk, size = %d", chunked_size);
                            state=CHUNKED_EXT;
                            // use_chunked=1;
                        } else if(parse_len==2){
                            // Case - chunked size=0
                            // FIXME: is this right condition?
                            chunked_size=atoi(tmp);
                            printf("[Last Chunk] Chunk size: %d\n", chunked_size);
                            LOG(DEBUG, "Last Chunk, size = %d", chunked_size);
                            state=CHUNKED_EXT;
                            // use_chunked=1;
                        }
                        free(tmp);
                    }
                } /* CHUNKED (if extension is valid) */
            default:
                // append to readbuf
                break;
        }
    }

    LOG(INFO, "Total received: %d bytes", buf_idx);
    free(readbuf);

    return 0;
}

int
http_state_machine(
    int sockfd,
    void **http_request,
    int reuse,
    int raw)
{
    /** check conformance when construct http request header. */
    /** send the request (similar with http_request()) */
    char *req=NULL;
    if(!raw){
        /** FIXME: functions which relate to `http_t` need to be fixed
         *  with error of memory access.
         */
        http_recast((http_t *)*http_request, &req);
        int sendbytes=send(sockfd, req, strlen(req), 0);
        LOG(DEBUG, "Finished HTTP recasting. Request length: %d; Sent bytes: %d", strlen(req), sendbytes);
    } else {
        req=(char *)*http_request;
        int sendbytes=send(sockfd, req, strlen(req), 0);
        LOG(DEBUG, "Using raw HTTP request header. Request length: %d; Sent bytes: %d", strlen(req), sendbytes);
    }

    // using one state machine
    http_rcv_state_machine(sockfd, http_request);

    // 4. finish, log the response and close the connection.
    if(!reuse){
        close(sockfd);
    }

    return 0;
}

int
http_handle_state_machine_ret(
    int ret,
    parsed_args_t *args,
    int *sockfd,
    void **return_obj)
{
    char *loc;
    switch(ret){
        /********************************************** handle redirection ***********************************************/
        case ERR_REDIRECT:
            /** return_obj will be the field value of `Location` (type casting from void)
             */
            loc=(char*)(*return_obj);
            // need to parse again
            ret=parse_url(loc, &args->host, &args->path);
            switch(ret){
                case ERR_NONE:
                    if(!(args->flags&SPE_PORT)){ // user hasn't specifed port-num
                        args->port=DEFAULT_PORT;
                    }
                    break;
                case ERR_USE_SSL_PORT:
                    if(!(args->flags&SPE_PORT)){ // user hasn't specifed port-num
                        puts("***********************Using default SSL Port***********************");
                        args->port=DEFAULT_SSL_PORT;
                    }
                    break;
                case ERR_INVALID_HOST_LEN:
                default:
                    exit(1);
            }
            // create new conn
            *sockfd=create_tcp_conn(args->host, itoa(args->port));
            if(sockfd<0){
                exit(1);
            }
            // change attributes in http_req
            http_req_obj_ins_header_by_idx(&args->http_req, REQ_HOST, args->host);
            printf("================================================================================\n");
            printf("%-50s: %s\n", "Redirect URL: ", loc);
            printf("%-50s: %d\n", "Port number: ", args->port);
            printf("%-50s: %s\n", "Method: ", args->method);
            printf("********************************************************************************\n");
            return 1;
        default:
            break;
    }

    return 0;
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
