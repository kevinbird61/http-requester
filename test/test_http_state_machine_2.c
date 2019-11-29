#include "http.h" 

int main(int argc, char *argv[])
{
    if(argc<4){
        fprintf(stderr, "Usage: ./program [TARGET_IP] [PORT] [TARGET_RESOURCE]\n");
        exit(1);
    }

    // alloc
    http_req_header_status_t *http_req;
    http_req_obj_create(&http_req);
    char *http_request;
    // start-line
    http_req_create_start_line(&http_request, "GET", "/", HTTP_1_1);
    // header fields
    http_req_obj_ins_header_by_idx(&http_req, REQ_HOST, argv[1]);
    http_req_obj_ins_header_by_idx(&http_req, REQ_CONN, "keep-alive");
    http_req_obj_ins_header_by_idx(&http_req, REQ_USER_AGENT, "http-request-c");
    for(int i=1;i<REQ_HEADER_NAME_MAXIMUM;i++){
        if(http_req->dirty_bit_align& (1<<(i-1)) ){
            http_req_ins_header(&http_request, get_req_header_name_by_idx[i], http_req->field_value[i]);
        }
    }
    http_req_finish(&http_request);

    char *http_request2;
    // start-line
    http_req_create_start_line(&http_request2, "GET", argv[3], HTTP_1_1);
    // header fields
    http_req_obj_ins_header_by_idx(&http_req, REQ_HOST, argv[1]);
    http_req_obj_ins_header_by_idx(&http_req, REQ_CONN, "keep-alive");
    http_req_obj_ins_header_by_idx(&http_req, REQ_USER_AGENT, "http-request-c");
    for(int i=1;i<REQ_HEADER_NAME_MAXIMUM;i++){
        if(http_req->dirty_bit_align& (1<<(i-1)) ){
            http_req_ins_header(&http_request2, get_req_header_name_by_idx[i], http_req->field_value[i]);
        }
    }
    http_req_finish(&http_request2);

    char *total_reqs;
    total_reqs=calloc(strlen(http_request2)+strlen(http_request)+8, 1);
    sprintf(total_reqs, "%s\r\n\r\n%s\r\n\r\n", http_request, http_request2);
    printf("HTTP request:*******************************************************************\n");
    printf("%s\n", total_reqs);
    printf("================================================================================\n");

    // create connection
    int sockfd=create_tcp_conn(argv[1], argv[2]);
    if(sockfd<0){
        exit(1);
    }

    void *res;
    send(sockfd, total_reqs, strlen(total_reqs), 0);
    http_rcv_state_machine(sockfd, &res);
    http_rcv_state_machine(sockfd, &res);

    close(sockfd);
    return 0;
}