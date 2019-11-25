#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "http.h"

/* calculate next http parsing state */
http_state next_http_state(http_state cur_state, char ch);

int http_state_machine(int sockfd, void **http_request, int reuse, int raw)
{
    // 1. check the conformance of http_request
    
    // 2. send the request (similar with http_request())
    char *req=NULL;
    if(!raw){
        http_recast((http_t *)*http_request, &req);
        char *reqlen=itoa(strlen(req));
        int sendbytes=send(sockfd, req, strlen(req), 0);
        char *sent=(char*)itoa(sendbytes);
        syslog("DEBUG", __func__, "Finished HTTP Recasting. Request length: ", reqlen, "; Sent bytes: ", sent, NULL);
    } else {
        req=(char *)*http_request;
        int sendbytes=send(sockfd, req, strlen(req), 0);
        syslog("DEBUG", __func__, "Using raw HTTP request. Request length: ", itoa(strlen(req)), "; Sent bytes: ", itoa(sendbytes), NULL);
    }

    // 3. recv and parse, until enter finish/abort state (state machine part)
    // - remember: if response status code is 302, we need to send a new redirect request.
    // - TODO: need to create a new socket to send redirect request?
    //  non-blocking
    /*
    struct timeval tv;
    fd_set readfds;
    // set timeout to 1.5 sec (wait next received.)
    tv.tv_sec=1;
    tv.tv_usec=500000;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    select(sockfd, &readfds, NULL, NULL, &tv);
    */

    int buf_size=64, chunk=64, buf_idx=0, parse_len=0, flag=1;
    char bytebybyte, *bufbybuf;
    char *readbuf=calloc(buf_size, sizeof(char)); // pre-allocated 32 bytes
    if(readbuf==NULL){
        syslog("ERROR", __func__, "MALLOC error when alloc to *readbuf. Current buf_size: ", itoa(buf_size), NULL);
        exit(1);
    }
    u8 state=VER; // start with version
    u8 status_code=0;
    u8 http_ver=0;

    // http_header_status_t - conformance check
    http_header_status_t *http_h_status_check=create_http_header_status(readbuf);
    u8 use_content_length=0, use_chunked=0;
    u32 content_length=0, chunked_size=0;
    // scenario 1: byte-by-byte reading
    while(flag){
        /** Put some checking mechanism to terminate parsing 
         * state machine before recipient send FIN back (timeout).
         */
        if(use_chunked && chunked_size==0){
            // printf("END, idx: %d\n", buf_idx-1);
            printf("[Transfer-Encoding: Chunked] Parsing process has been done.\n");
            break;
        } 

        /* recv byte by byte */
        if(!recv(sockfd, &bytebybyte, 1, 0)){
            break;
        }
        readbuf[buf_idx]=bytebybyte;
        buf_idx++;
        parse_len++;
        // check size, if not enough, then realloc
        if(buf_idx==buf_size){
            buf_size+=chunk; 
            readbuf=realloc(readbuf, buf_size*sizeof(char));
            http_h_status_check->buff=readbuf;
            if(readbuf==NULL){
                syslog("ERROR", __func__, "REALLOC failure when realloc to *readbuf, check the memory. Current buf_size: ", itoa(buf_size), NULL);
                exit(1);
            }
            // if you feel this message is too noisy, can comment this line or increase chunk size
            //syslog("DEBUG", __func__, "REALLOC readbuf size. Current buf_size: ", itoa(buf_size), NULL);
        }

        // check if header using content-length
        if(use_content_length){
            // --content_length;
            if(!(--content_length)){ 
                printf("[CL] Parsing process has been done.\n");
                break; 
            }
        }

        // check if header using transfer-encoding 
        if(use_chunked){
            if(!(--chunked_size)){ 
                printf("Finish chunk, idx: %d\n", buf_idx-1);
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
                        syslog("DEBUG", __func__, " [ Reason Phrase: ", tmp, " ] ", NULL);
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
                    state=MSG_BODY;
                    syslog("DEBUG", __func__, "Message header length: ", itoa(buf_idx));
                    /** Version -
                     * - Not support HTTP/0.9, /2.0, /3.0 (e.g. http_ver==0)
                     */
                    if(!http_ver) {
                        // syslog - not support
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
                    if(status_code<_200_OK){
                        // 1xx
                        syslog("NOT SUPPORT", __func__, "Response from server :", get_http_status_code_by_idx[status_code], ", ",get_http_reason_phrase_by_idx[status_code], NULL);
                        // TODO:
                    } else if(status_code>=_200_OK && status_code<_300_MULTI_CHOICES){
                        // 2xx
                        syslog("SUCCESSFUL", __func__, "Response from server :", get_http_status_code_by_idx[status_code], ", ",get_http_reason_phrase_by_idx[status_code], NULL);
                        // keep processing 
                    } else if(status_code>=_300_MULTI_CHOICES && status_code<_400_BAD_REQUEST){
                        switch(status_code){
                            case _301_MOVED_PERMANENTLY:
                            case _302_FOUND:
                                // search `Location` field-value
                                printf("Redirect to `Location`: %s\n", strndup(readbuf+1+(http_h_status_check->field_value[LOCATION].idx), http_h_status_check->field_value[LOCATION].offset));
                                // close the connection
                                close(sockfd);
                                // redirect to new target (new conn + modified request)
                                // reuse http_request ptr (to store Location)
                                *http_request=strndup(readbuf+1+(http_h_status_check->field_value[LOCATION].idx), http_h_status_check->field_value[LOCATION].offset-2); // -2: delete CRLF
                                return ERR_REDIRECT;
                                // exit(1);
                            default:
                                // not support, keep processing
                                break;
                        }
                    } else if(status_code>=_400_BAD_REQUEST && status_code<=_505_HTTP_VER_NOT_SUPPORTED){
                        printf("Connection is terminated by %s's failure ...\n", status_code<_500_INTERNAL_SERV_ERR?"client":"server");
                        printf("Response from server: %s, %s\n", get_http_status_code_by_idx[status_code], get_http_reason_phrase_by_idx[status_code]);
                        // syslog 
                        syslog("CONNECTION ABORT", __func__, "Response from server :", get_http_status_code_by_idx[status_code], ", ",get_http_reason_phrase_by_idx[status_code], NULL);
                        // terminate, we don't need the parse the rest of data
                        close(sockfd);
                        // FIXME: return error code instead terminate directly
                        exit(1);
                    }

                    /** Transfer coding/Message body info - 
                     * check current using Transfer-Encoding or Content-Length
                     */
                    if(http_h_status_check->content_length_dirty){
                        use_content_length=1;
                        content_length=atoi(readbuf+http_h_status_check->field_value[CONTENT_LEN].idx);
                        printf("Get content length= %d\n", content_length);
                        if(content_length==0){
                            flag=0;
                        }
                    } else if(http_h_status_check->transfer_encoding_dirty){
                        // need to parse under chunked size=0
                        state=CHUNKED;
                    }

                    /**
                     * 
                     */

                } else if(state==CHUNKED || state==NEXT_CHUNKED){
                    // check if it is `chunked`
                    //fprintf(stdout, "Debug, Chunked size: %s\n", strndup(readbuf+(buf_idx-parse_len), parse_len));
                    // FIXME: need to using strtok to parse all possible value
                    if(!strncasecmp(strndup(readbuf+1+(http_h_status_check->field_value[TRANSFER_ENCODING].idx), http_h_status_check->field_value[TRANSFER_ENCODING].offset), "chunked", 7)){
                        char *tmp=calloc(parse_len, sizeof(char));
                        sprintf(tmp, "%ld", strtol(strndup(readbuf+(buf_idx-parse_len), parse_len), NULL, 16));
                        
                        // fprintf(stdout, "%s\n", strndup(readbuf+(buf_idx-parse_len), parse_len));
                        // printf("isdigit: %d\n", isdigit(atoi(tmp)));
                        if((atoi(tmp))>0){
                            // printf("%s, %s\n", tmp, strndup(readbuf+(buf_idx-parse_len), parse_len));
                            chunked_size=atoi(tmp);
                            printf("[Get Chunk] Chunk size: %d\n", chunked_size);
                            syslog("DEBUG", __func__, "Get Chunk, size=", tmp, NULL);
                            state=CHUNKED;
                            use_chunked=1;
                        } else if(parse_len==2){
                            // Case - chunked size=0 
                            // FIXME: is this right condition?
                            chunked_size=atoi(tmp);
                            printf("[Last Chunk] Chunk size: %d\n", chunked_size);
                            syslog("DEBUG", __func__, "Last Chunk, size=", tmp, NULL);
                            use_chunked=1;
                        }
                        free(tmp);
                    }
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
                                syslog("DEBUG", __func__, " [ Version: ", get_http_version_by_idx[ret], " ] ", NULL);
                                http_ver=ret;
                            } else { // if not support, just terminate
                                char *tmp=malloc((parse_len)*sizeof(char));
                                snprintf(tmp, parse_len, "%s", readbuf+(buf_idx-parse_len));
                                syslog("DEBUG", __func__, " [ Not support: ", tmp, " ] ", NULL);
                                free(tmp);
                                flag=0;
                            }
                            parse_len=0;
                            break;
                        case CODE_OR_TOKEN:
                            // printf("%d\n", encap_http_status_code(atoi(readbuf+(buf_idx-parse_len))));
                            if((ret=encap_http_status_code(atoi(readbuf+(buf_idx-parse_len)))) > 0){
                                status_code=ret;
                                syslog("DEBUG", __func__, " [ Status code: ", get_http_status_code_by_idx[ret], " ] ", NULL);
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
                }
            default:
                // append to readbuf
                break;
        }
    }

    // printf("[MSG_BODY]\n");
    // fwrite(readbuf+(buf_idx-parse_len), sizeof(char), parse_len, stdout);
    // printf("Sizeof reabuf: %ld\n", strlen(readbuf));
    syslog("DEBUG", __func__, "Total received: ", itoa(buf_idx), " bytes.", NULL);
    free(readbuf);

    // 4. finish, log the response and close the connection.
    if(!reuse){
        close(sockfd);
    }

    return 0;
}

http_state next_http_state(http_state cur_state, char ch)
{
    // current only use in Response
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
                default:
                    return CODE_OR_TOKEN;
            }
        case REASON_OR_RESOURCE:
            switch(ch){
                case ' ':
                    return REASON_OR_RESOURCE;
                case '\r':
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
    }
}
