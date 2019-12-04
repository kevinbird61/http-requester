#include "argparse.h"
#include "types.h"
#include "conn.h"
#include "http.h"

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
        /* forge http request header */
        char *http_request;
        // start-line
        http_req_create_start_line(&http_request, args->method, args->path, HTTP_1_1);
        // header fields
        http_req_obj_ins_header_by_idx(&args->http_req, REQ_HOST, args->host);
        http_req_obj_ins_header_by_idx(&args->http_req, REQ_CONN, ((args->conn > 1)? "keep-alive": "close"));
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
            /*char *total_reqs=malloc(args->conn*(strlen(http_request)+4));
            sprintf(total_reqs, "%s\r\n", http_request);
            // copy 
            for(int i=1; i<args->conn; i++){
                sprintf(total_reqs, "%s%s\r\n", total_reqs, http_request);
            }*/
            char *total_reqs=copy_str_n_times(http_request, args->conn);

            // return value
            void *res;
            send(sockfd, total_reqs, strlen(total_reqs), 0);
            
            /** FIXME: 
             * - dealing with result object
             * - if not using keep-alive, then we need to create a new socket for it
             */
            for(int i=0; i<args->conn; i++){
                LOG(INFO, "[Pipelining (recv): %d/%d]", i+1, args->conn);
                // each request need a parsing state machine to handle!
                int ret=http_rcv_state_machine(sockfd, &res);
                ret=http_handle_state_machine_ret(ret, args, &sockfd, &res);
                if(ret){ // send redirect (FIXME: if using pipeline with redirection, previous connection need to resend)
                    char *redirect_req;
                    http_req_create_start_line(&redirect_req, args->method, args->path, HTTP_1_1);
                    http_req_finish(&redirect_req, args->http_req);
                    printf("New HTTP request:***************************************************************\n");
                    printf("%s\n", redirect_req);
                    printf("================================================================================\n");
                    i--; // refill (for redirection)
                    free(total_reqs);
                    // need to resend the leftover part 
                    total_reqs=copy_str_n_times(redirect_req, args->conn-i);
                    send(sockfd, total_reqs, strlen(total_reqs), 0);
                }
            }
        } else {
            /** without pipelining
             */
            void *res;
            // total connection
            while(args->conn){
                // send request
                send(sockfd, http_request, strlen(http_request), 0);
                // parse once
                int ret=http_rcv_state_machine(sockfd, &res);
                // get the return value to perform furthor operations
                ret=http_handle_state_machine_ret(ret, args, &sockfd, &res);
                // reduce
                args->conn--;
                if(ret){ // send redirect
                    http_request=NULL;
                    http_req_create_start_line(&http_request, args->method, args->path, HTTP_1_1);
                    http_req_finish(&http_request, args->http_req);
                    printf("New HTTP request:***************************************************************\n");
                    printf("%s\n", http_request);
                    printf("================================================================================\n");
                    args->conn++; // refill (for redirection)
                }
            }
        } /* without pipelining */

        // close sockfd 
        close(sockfd);
    } /* USE_URL */

    return 0;
}