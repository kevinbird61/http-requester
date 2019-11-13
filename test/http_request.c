#include "http.h"

int main(int argc, char **argv)
{
    if(argc<3){
        fprintf(stderr, "Usage: ./http_request.out TARGET_IP PORT REQ_TARGET\n");
        exit(1);
    }

    http_t *http_get_request=malloc(sizeof(http_t));

    char *req_target=argv[3];
    char *target_ip=argv[1];
    char *port=argv[2];
    int sockfd=create_tcp_conn(target_ip, port);
    // header fields
    // char *host="20.20.102.1";

    // FIXME: need to check return value
    http_get_request->req.method_token=encap_http_method_token("GET");
    http_get_request->req.req_target=req_target;
    http_get_request->target=target_ip;
    http_get_request->port=atoi(port);
    http_get_request->version=ONE_ONE;

    http_get_request->headers=malloc(sizeof(http_header_t));
    http_get_request->headers->field_name="Host";
    http_get_request->headers->field_value=target_ip;
    http_get_request->headers->next=NULL;
    http_get_request->msg_body=NULL;

    char *rawdata=malloc(10240*sizeof(char));

    int numbytes=http_request(sockfd, http_get_request, rawdata);

    if(numbytes==-1)
    {
        perror("error recv");
        close(sockfd);
        exit(1);
    }
    printf("client: received %d bytes\n%s\n", numbytes, rawdata);
    
    close(sockfd);
    return 0;
}
