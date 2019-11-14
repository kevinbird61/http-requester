#include "http.h"

/* http_t to string format */
int http_request(int sockfd, http_t *http_request, char *rawdata)
{
    char *req;
    http_recast(http_request, &req);
    
    // send
    send(sockfd, req, strlen(req), 0);    
    
    // FIXME: polling to fetch all data from RX
    int numbytes=recv(sockfd, rawdata, 10240, 0);

    return numbytes;
}