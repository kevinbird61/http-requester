#include "http.h"
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

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

typedef enum {
    NON,
    CR,
    LF,
    CRLF,
    NEXT
} parse_state;

http_state next_http_state(http_state cur_state, http_msg_type type);
parse_state next_parse_state(parse_state cur_pstate, parse_state nxt_pstate);

int http_state_machine(int sockfd, http_t *http_request)
{
    // 1. check the conformance of http_request
    
    // 2. send the request (similar with http_request())
    char *req=NULL;
    http_recast(http_request, &req);
    printf("strlen(req): %ld\n", strlen(req));
    printf("Sent: %ld\n" , send(sockfd, req, strlen(req), 0));
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
    char *readbuf=malloc(buf_size*sizeof(char)), *tmp; // pre-allocated 32 bytes
    http_msg_type msg_state=UNDEFINED;
    http_state state=START_LINE;
    parse_state pstate=NON;
    // current using http_t
    http_t *http_response=malloc(sizeof(http_t));
    // scenario 1: byte-by-byte reading
    while(flag){
        if(!recv(sockfd, &bytebybyte, 1, 0)){
            break;
        }
        // printf("Parse state: %d, HTTP state: %d\n", pstate, state);
        // read byte, check the byte 
        switch(bytebybyte){
            case '\r':
                pstate=next_parse_state(pstate, CR);
                state=next_http_state(state, RES);
                // printf("Type: %d\n", state);
                if(state<MSG_BODY && parse_len>0){
                    // parse last-element of START-LINE, or field-value
                    if(state>HEADER){
                        tmp=malloc((parse_len-1)*sizeof(char));
                        memset(tmp, 0x0, parse_len-1);
                        strncpy(tmp, readbuf+(buf_idx-parse_len+1), parse_len-1);
                    } else {
                        tmp=malloc(parse_len*sizeof(char));
                        memset(tmp, 0x0, parse_len);
                        strncpy(tmp, readbuf+(buf_idx-parse_len), parse_len);
                    }
                    // strncpy(tmp, readbuf+(buf_idx-parse_len), parse_len);
                    printf("[HTTP:%d] Parse_len: %d, %s\n", state, parse_len, tmp);
                    parse_len=0;
                    free(tmp);
                }
                break;
            case '\n':
                pstate=next_parse_state(pstate, LF);
                state=next_http_state(state, RES);
                if(pstate==NEXT && state!=MSG_BODY){
                    state=MSG_BODY;
                    printf("Go to MSG BODY\n");
                    // printf("Current data: %s\n\n", readbuf);
                } else if(pstate==NEXT && state!=MSG_BODY) {
                    printf("Parsing process has been done.\n");
                    flag=0;
                }
                break;
            case ':':
                // for parsing field-name & value
                if(state==HEADER && parse_len>0){
                    state=next_http_state(state, RES);
                    // printf("Type: %d\n", state);
                    tmp=malloc(parse_len*sizeof(char));
                    memset(tmp, 0x0, parse_len);
                    strncpy(tmp, readbuf+(buf_idx-parse_len), parse_len);
                    printf("Parse_len: %d, %s\n", parse_len, tmp);
                    parse_len=0;
                    free(tmp);
                }
            case ' ':
                if(state>=START_LINE && state<=REASON_OR_RESOURCE && parse_len>0){
                    // always RES
                    state=next_http_state(state, RES);
                    tmp=malloc((parse_len)*sizeof(char));
                    memset(tmp, 0x0, parse_len);
                    strncpy(tmp, readbuf+(buf_idx-parse_len), parse_len);
                    printf("Parse_len: %d, %s\n", parse_len, tmp);
                    free(tmp);
                    parse_len=0;
                    break;
                }
            default:
                // append to readbuf
                pstate=NON;
                readbuf[buf_idx]=bytebybyte;
                buf_idx++;
                parse_len++;
                // check size, if not enough, then realloc
                if(strlen(readbuf)==buf_size){
                    buf_size+=chunk; 
                    readbuf=realloc(readbuf, buf_size*sizeof(char));
                    if(readbuf==NULL){
                        perror("Realloc failure, check the memory.");
                        exit(1);
                    }
                }
        }
    }

    tmp=malloc(parse_len*sizeof(char));
    memset(tmp, 0x0, parse_len);
    strncpy(tmp, readbuf+(buf_idx-parse_len), parse_len);
    // printf("MSG_BODY: %s\n", tmp);
    // printf("Sizeof reabuf: %ld\n%s\n", strlen(readbuf), readbuf);
    printf("Sizeof reabuf: %ld\n", strlen(readbuf));

    // 4. finish, log the response and close the connection.
    close(sockfd);

    return 0;
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