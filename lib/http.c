#include "http.h"

int http_request(int sockfd, http_t *http_request, char **rawdata)
{
    /* http_t to string format */

    // default req size is 32, using realloc to scale
    int limit=128;
    char *req=malloc(limit*sizeof(char));
    
    // start line
    sprintf(req, "%s %s HTTP/%s\r\n", 
        get_http_method_token(http_request->req.method_token), http_request->req.req_target,
        get_http_version(http_request->version));
    // header fields
    while(http_request->headers!=NULL)
    {
        char buf[128];
        sprintf(buf, "%s: %s\r\n", http_request->headers->field_name, http_request->headers->field_value);
        // check size, if not enough, scale it
        if(limit<=(strlen(buf)+strlen(req))){
            req=realloc(req, limit*2*sizeof(char));
            limit+=limit;
        }
        // snprintf(req, strlen(req)+strlen(buf), "%s%s", req, buf);
        sprintf(req, "%s%s", req, buf);
        http_request->headers=http_request->headers->next;
    }
    req=realloc(req, limit+2);
    // snprintf(req, strlen(req)+2,"%s\r\n", req);
    sprintf(req, "%s\r\n", req);

    printf("HTTP header: %s\n", req);
}

int http_parser(const char *rawdata, http_t *http_packet)
{
    
}

int http_process(http_t *http_packet)
{

}

char *get_http_version(http_version_map http_version)
{
    switch(http_version)
    {
        case ONE_ZERO:
            return "1.0";
        case ONE_ONE:
            return "1.1";
        default:
            return NULL;
    }
}

char *get_http_method_token(method_token_map method_token)
{
    switch(method_token)
    {
        case GET: 
            return "GET";
        default:
            return NULL;
    }
}

int encap_http_method_token(char *method)
{
    if(!strncmp(method, "GET", 3)){
        return GET;
    } else {
        return 0;
    }
}

char *get_http_status_code(status_code_map status_code)
{
    switch(status_code)
    {
        case TWO_O_O:
            return "200";
        case THREE_O_O:
            return "300";
        case FOUR_O_O:
            return "400";
        case FOUR_O_FOUR:
            return "404";
        default:
            return NULL;
    }
}

int encap_http_status_code(int http_status_code)
{
    switch(http_status_code)
    {
        case 200:
            return TWO_O_O;
        case 300:
            return THREE_O_O;
        case 400:
            return FOUR_O_O;
        case 404:
            return FOUR_O_FOUR;
        default:
            return 0;
    }
}