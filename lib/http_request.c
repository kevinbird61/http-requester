#include "http.h"

/* http_t to string format */
int http_request(int sockfd, http_t *http_request, char *rawdata)
{
    // default req size is 64, using realloc to scale
    int limit=32;
    char *req=malloc(limit*sizeof(char));

    /**************************************************** start line ****************************************************/
    char *method=get_http_method_token(http_request->req.method_token),
        *req_target=http_request->req.req_target,
        *http_ver=get_http_version(http_request->version);
    // check req size (+9: other SP & HTTP)
    int size_start_line=strlen(method)+strlen(req_target)+strlen(http_ver)+4;
    if(limit<=size_start_line){
        limit=size_start_line;
        req=realloc(req, (limit)*sizeof(char));
    }
    // string copy
    // snprintf(req, size_start_line, "%s %s HTTP/%s\r\n", method, req_target, http_ver);
    sprintf(req, "%s %s %s\r\n", method, req_target, http_ver);
    // printf("HTTP header: %s (%p)\n", req, &req);
    /**************************************************** header fields ****************************************************/
    while(http_request->headers!=NULL)
    {
        int header_size=(strlen(http_request->headers->field_name)+strlen(http_request->headers->field_value)+4);
        char *buf=malloc(header_size*sizeof(char));
        // snprintf(buf, header_size, "%s: %s\r\n", http_request->headers->field_name, http_request->headers->field_value);
        sprintf(buf, "%s: %s\r\n", http_request->headers->field_name, http_request->headers->field_value);
        // printf("Buf: %s\n", buf);
        // check size, if not enough, scale it
        if(limit<=(strlen(buf)+strlen(req))){
            limit+=(strlen(buf));
            req=realloc(req, limit*sizeof(char));
        }
        // snprintf(req, limit, "%s%s", req, buf);
        sprintf(req, "%s%s", req, buf);
        http_request->headers=http_request->headers->next;
        // printf("HTTP header: %s (%p)\n", req, &req);
        free(buf);
    }
    // printf("Size: %d, strlen(req): %ld\n", limit, strlen(req));
    // printf("HTTP header: %s (%p)\n", req, &req);
    req=realloc(req, (limit+=2)*sizeof(char));
    // snprintf(req, limit, "%s\r\n", req);
    sprintf(req, "%s\r\n", req);

    // print for debug
    printf("HTTP header:\n%s\n", req);

    // send
    send(sockfd, req, strlen(req), 0);    
    
    // FIXME: polling to fetch all data from RX
    int numbytes=recv(sockfd, rawdata, 10240, 0);

    return numbytes;
}