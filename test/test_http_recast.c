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

    // header fields
    // FIXME: need to check return value
    http_get_request->req.method_token=encap_http_method_token("GET");
    http_get_request->req.req_target=req_target;
    http_get_request->target=target_ip;
    http_get_request->port=atoi(port);
    http_get_request->version=HTTP_1_1;

    http_get_request->headers=malloc(sizeof(http_header_t));
    http_get_request->headers->field_name="Host";
    http_get_request->headers->field_value=target_ip;
    http_get_request->headers->next=NULL;
    http_get_request->msg_body=NULL;

    debug_http_obj(http_get_request);

    char *rawdata=malloc(16*sizeof(char));
    http_recast(http_get_request, &rawdata);

    printf("\nHTTP packet (using http_recast):\n%s\n", rawdata);

    return 0;
}
