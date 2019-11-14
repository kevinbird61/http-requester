#include "http.h" 

int main(void)
{
    http_t *http_req=malloc(sizeof(http_t)),
        *http_res=malloc(sizeof(http_t));
    // read & interpret from file and build obj
    http_interpret("test/template/HTTP_GET.txt", http_req);
    debug_http_obj(http_req);
    // create connection
    int sockfd=create_tcp_conn("kevin.imslab.org", "80");
    char *rawdata=malloc(10240*sizeof(char));
    // use the http obj to send request
    http_request(sockfd, http_req, rawdata);
    // parse the response, and store into http obj
    http_parser(rawdata, http_res);
    debug_http_obj(http_res);
    return 0;
}