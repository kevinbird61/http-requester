#include "http.h"

int main(void)
{
    char *req_target="/test.html";
    char *target_ip="140.116.245.247";
    char *port="80";
    http_t *http_get_request=malloc(sizeof(http_t)),
        *http_get_response=malloc(sizeof(http_t));
    int sockfd=create_tcp_conn(target_ip, port);

    // create request 
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

    // send request
    char *rawdata=malloc(10240*sizeof(char));
    int numbytes=http_request(sockfd, http_get_request, rawdata);

    // parse response
    http_parser(rawdata, http_get_response);

    return 0;
}