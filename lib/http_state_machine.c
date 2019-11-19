#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "logger.h"
#include "http.h"

// http parsing state
typedef enum {
    // start-line states
    START_LINE=1,
    VER,
    CODE_OR_TOKEN,
    REASON_OR_RESOURCE,
    // header fields
    HEADER,
    FIELD_NAME,
    FIELD_VALUE,
    // msg body
    MSG_BODY,
    // terminate successfully
    END,
    // terminate with error 
    ABORT
} http_state;

// character
typedef enum {
    NON,
    CR,
    LF,
    CRLF,
    NEXT
} parse_state;

/* helper function */
char *get_http_state(http_state state);
http_state next_http_state(http_state cur_state, http_msg_type type);
parse_state next_parse_state(parse_state cur_pstate, parse_state nxt_pstate);

int http_state_machine(int sockfd, http_t *http_request)
{
    // 1. check the conformance of http_request
    
    // 2. send the request (similar with http_request())
    char *req=NULL;
    http_recast(http_request, &req);
    // logging
    char *reqlen=itoa(strlen(req));
    int sendbytes=send(sockfd, req, strlen(req), 0);
    char *sent=(char*)itoa(sendbytes);

    syslog("DEBUG", __func__, "Finished HTTP Recasting. Request length: ", reqlen, "; Sent bytes: ", sent, NULL);

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
    http_msg_type msg_state=UNDEFINED;
    http_state state=START_LINE;
    parse_state pstate=NON;
    // current using http_t
    // http_t *http_response=malloc(sizeof(http_t));
    // scenario 1: byte-by-byte reading
    while(flag){
        if(!recv(sockfd, &bytebybyte, 1, 0)){
            break;
        }
        /* record */
        readbuf[buf_idx]=bytebybyte;
        buf_idx++;
        parse_len++;
        // check size, if not enough, then realloc
        if(buf_idx==buf_size){
            buf_size+=chunk; 
            readbuf=realloc(readbuf, buf_size*sizeof(char));
            if(readbuf==NULL){
                syslog("ERROR", __func__, "REALLOC failure when realloc to *readbuf, check the memory. Current buf_size: ", itoa(buf_size), NULL);
                exit(1);
            }
            // if you feel this message is too noisy, can comment this line or increase chunk size
            //syslog("DEBUG", __func__, "REALLOC readbuf size. Current buf_size: ", itoa(buf_size), NULL);
        }

        // read byte, check the byte 
        switch(bytebybyte){
            case '\r':
                pstate=next_parse_state(pstate, CR);
                state=next_http_state(state, RES);
                // printf("Type: %d\n", state);
                if(state<MSG_BODY && parse_len>0){
                    // parse last-element of START-LINE, or field-value
                    char *tmp;
                    if(state>HEADER){
                        // header fields
                        // printf("[Field-value] %s\n", get_http_state(state));
                        // fwrite(readbuf+(buf_idx-parse_len+1), sizeof(char), parse_len-1, stdout);
                        // puts("\n");

                        tmp=malloc((parse_len-1)*sizeof(char));
                        snprintf(tmp, parse_len-1, "%s", readbuf+1+(buf_idx-parse_len));
                        syslog("DEBUG", __func__, "Parsing state ", get_http_state(state), ", Parsed value: ", tmp, NULL);

                    } else {
                        // (last-element) start-line
                        //printf("[Start line] %s\n", get_http_state(state));
                        //fwrite(readbuf+(buf_idx-parse_len), sizeof(char), parse_len, stdout);
                        //puts("\n");
                        
                        tmp=malloc((parse_len)*sizeof(char));
                        snprintf(tmp, parse_len, "%s", readbuf+(buf_idx-parse_len));
                        syslog("DEBUG", __func__, "Parsing state ", get_http_state(state), ", Parsed value: ", tmp, NULL);
                    }
                    free(tmp);
                    parse_len=0;
                }
                break;
            case '\n':
                pstate=next_parse_state(pstate, LF);
                state=next_http_state(state, RES);
                if(pstate==NEXT && state!=MSG_BODY){
                    state=MSG_BODY;

                } else if(pstate==NEXT && state==MSG_BODY) {
                    printf("Parsing process has been done.\n");
                    flag=0;
                }
                break;
            case ':':
                // for parsing field-name & value
                if(state==HEADER && parse_len>0){
                    state=next_http_state(state, RES);
                    //fwrite(readbuf+1+(buf_idx-parse_len), sizeof(char), parse_len-2, stdout);
                    //puts("\n");

                    char *tmp=malloc((parse_len-1)*sizeof(char));
                    snprintf(tmp, parse_len-1, "%s", readbuf+1+(buf_idx-parse_len));
                    syslog("DEBUG", __func__, "Parsing state: ", get_http_state(state), ". Parsed string:", tmp, NULL);
                    free(tmp);
                    parse_len=0;
                }
            case ' ':
                // parse only when in start-line state 
                if(state>=START_LINE && state<=REASON_OR_RESOURCE && parse_len>0){
                    // always RES
                    state=next_http_state(state, RES);
                    //printf("[Start line] %s\n", get_http_state(state));
                    //fwrite(readbuf+(buf_idx-parse_len), sizeof(char), parse_len, stdout);
                    //puts("\n");
                    
                    char *tmp=malloc((parse_len)*sizeof(char));
                    snprintf(tmp, parse_len, "%s", readbuf+(buf_idx-parse_len));
                    syslog("DEBUG", __func__, "Parsing state: ", get_http_state(state), ". Parsed string:", tmp, NULL);
                    free(tmp);
                    parse_len=0;
                    break;
                }
            default:
                // append to readbuf
                pstate=NON;
        }
    }

    /* malloc would encounter malloc error. (msg_body may have 40K~50K) */
    // printf("[Header]\n");
    // fwrite(readbuf, sizeof(char), buf_idx, stdout);
    // printf("%s\n", readbuf);

    // printf("[MSG_BODY]\n");
    // fwrite(readbuf+(buf_idx-parse_len), sizeof(char), parse_len, stdout);
    // printf("Sizeof reabuf: %ld\n", strlen(readbuf));
    syslog("DEBUG", __func__, "Total received: ", itoa(buf_idx), " bytes.", NULL);
    free(readbuf);

    // 4. finish, log the response and close the connection.
    close(sockfd);

    return 0;
}

char *get_http_state(http_state state)
{
    switch(state){
        case START_LINE:
            return "Start-line (Init state)";
        case VER:
            return "HTTP version";
        case CODE_OR_TOKEN:
            return "Status code or Method token";
        case REASON_OR_RESOURCE:
            return "REASON phrase or Target resource";
        case HEADER:
            return "Header section";
        case FIELD_NAME:
            return "Field-name";
        case FIELD_VALUE:
            return "Field-value";
        case MSG_BODY:
            return "Message-body";
        case END:
            return "End of parsing process";
        case ABORT:
            return "Failure";
    }
}

http_state next_http_state(http_state cur_state, http_msg_type type)
{
    switch(cur_state)
    {
        // start-line
        case START_LINE:
            if(type==REQ){
                return CODE_OR_TOKEN;
            } else {
                return VER;
            }
        case VER:
            if(type==REQ){
                return HEADER;
            } else {
                return CODE_OR_TOKEN;
            }
        case CODE_OR_TOKEN:
            return REASON_OR_RESOURCE;
        case REASON_OR_RESOURCE:
            if(type==REQ){
                return VER;
            } else {
                return HEADER;
            }
        // header
        case HEADER:
            return FIELD_NAME;
        case FIELD_NAME:
            return FIELD_VALUE;
        case FIELD_VALUE:
            return HEADER;
        case MSG_BODY:
            return MSG_BODY;
    }
}

parse_state next_parse_state(parse_state cur_pstate, parse_state nxt_pstate)
{
    if(cur_pstate==NON){
        return nxt_pstate;
    } else if(cur_pstate==CR && nxt_pstate==LF){
        return CRLF;
    } else if(cur_pstate==CRLF && nxt_pstate==CR){
        return CRLF;
    } else if(cur_pstate==CRLF && nxt_pstate==LF){
        return NEXT;
    } else {
        // all turn to next state
        return nxt_pstate;
    }
}