#include "http.h"

/* http_t to string format */
int http_request(int sockfd, http_t *http_request, char *rawdata)
{
    // need to set NULL (http_parser)
    char *req=NULL; 
    http_recast(http_request, &req);    
    // printf("strlen(req): %ld\n", strlen(req));
    // send
    send(sockfd, req, strlen(req), 0);
    // FIXME: polling to fetch all data from RX
    int numbytes=recv(sockfd, rawdata, 10240, 0);
    return numbytes;
}