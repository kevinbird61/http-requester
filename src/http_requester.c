#include "argparse.h"
#include "types.h"
#include "conn.h"
#include "http.h"

#define AGENT               "http-requester-c"

int main(int argc, char *argv[])
{
    /** step by step construct our http request header from user input:
     *  - parsing url first, if template file is available, 
     *      then url will override the fields in template file.
     *      - URL format: [http:]//<HOST, maximum length not larger than 255>[:port-num]/[pathname]
     *  - forge http header
     *  - create socket and use http_state_machine to send request
     *  
     */
    // using argparse to parse the user-input params
    parsed_args_t *args=create_argparse();
    u8 ret=argparse(&args, argc, argv);
    if(ret==USE_URL){
        /* 2. forge http request header */
        char *http_request;
        // start-line
        http_req_create_start_line(&http_request, args->method, args->path, HTTP_1_1);
        // header fields
        http_req_obj_ins_header_by_idx(&args->http_req, REQ_HOST, args->host);
        http_req_obj_ins_header_by_idx(&args->http_req, REQ_CONN, "keep-alive");
        http_req_obj_ins_header_by_idx(&args->http_req, REQ_USER_AGENT, AGENT);
        // finish
        http_req_finish(&http_request, args->http_req);
        printf("HTTP request:*******************************************************************\n");
        printf("%s\n", http_request);
        printf("================================================================================\n");
        
        /* create sock */
        int sockfd=create_tcp_conn(args->host, itoa(args->port));
        if(sockfd<0){
            exit(1);
        }

        /** TODO: concurrency */

        /* use pipeline or not */
        if(args->enable_pipe){
            // construct multiple request headers (together) 
            char *total_reqs=malloc(args->conn*(strlen(http_request)+4));
            sprintf(total_reqs, "%s\r\n", http_request);
            // copy 
            for(int i=1; i<args->conn; i++){
                sprintf(total_reqs, "%s%s\r\n", total_reqs, http_request);
            }

            void *res;
            send(sockfd, total_reqs, strlen(total_reqs), 0);
            
            /* FIXME: dealing with res */
            for(int i=0; i<args->conn; i++){
                // each request need a parsing state machine to handle!
                http_rcv_state_machine(sockfd, &res);
            }
        } else {
            /** without pipelining
             */
            while(args->conn){
                int ret=http_state_machine(sockfd, (void**)&http_request, --args->conn, 1);
                char *loc;
                switch(ret){
                    case ERR_REDIRECT: /* handle redirection */
                        // Modified http request, and send the request again
                        loc=(char*)http_request;
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
                        sockfd=create_tcp_conn(args->host, itoa(args->port));
                        if(sockfd<0){
                            exit(1);
                        }
                        // change attributes in http_req
                        http_req_obj_ins_header_by_idx(&args->http_req, REQ_HOST, args->host);
                        // construct new http request from modified http_req
                        http_req_create_start_line(&http_request, args->method, args->path, HTTP_1_1);
                        http_req_finish(&http_request, args->http_req);
                        printf("================================================================================\n");
                        printf("%-50s: %s\n", "Target URL: ", (char*)args->url==NULL? "None": (char*)args->url);
                        printf("%-50s: %d\n", "Port number: ", args->port);
                        printf("%-50s: %s\n", "Method: ", args->method);
                        printf("********************************************************************************\n");
                        printf("New HTTP request:***************************************************************\n");
                        printf("%s\n", http_request);
                        printf("================================================================================\n");
                        args->conn++; // refuel
                        // exit(1);
                    default:
                        break;
                }
            }
        }
    } /* USE_URL */

    return 0;
}