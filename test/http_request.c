#include "http.h"

int main(void)
{
    http_t *http_get_request=malloc(sizeof(http_t));

    char *req_target="/test.html";
    char *target_ip="20.20.102.1";
    char *port="80";
    int sockfd=create_tcp_conn(target_ip, port);
    // header fields
    // char *host="20.20.102.1";

    // FIXME: need to check return value
    http_get_request->req.method_token=encap_http_method_token("GET");
    http_get_request->req.req_target=req_target;
    http_get_request->target=target_ip;
    http_get_request->port=atoi(port);

    http_get_request->headers=malloc(sizeof(http_header_t));
    http_get_request->headers->field_name="Host";
    http_get_request->headers->field_value=target_ip;
    http_get_request->headers->next=NULL;
    http_get_request->msg_body=NULL;

    char *rawdata;

    http_request(sockfd, http_get_request, &rawdata);

    return 0;
}