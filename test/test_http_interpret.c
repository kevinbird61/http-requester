#include "http.h" 

int main(int argc, char *argv[])
{
    if(argc<4){
        fprintf(stderr, "Usage: ./program TEMPLATE_FILE_PATH TARGET_IP PORT\n");
        exit(1);
    }

    http_t *http_req=malloc(sizeof(http_t)),
        *http_res=malloc(sizeof(http_t));
    // read & interpret from file and build obj
    http_interpret(argv[1], http_req);
    debug_http_obj(http_req);
    // create connection
    int sockfd=create_tcp_conn(argv[2], argv[3]);
    char *rawdata=malloc(10240*sizeof(char));
    // use the http obj to send request
    http_request(sockfd, http_req, rawdata);
    // parse the response, and store into http obj
    http_parser(rawdata, http_res);
    debug_http_obj(http_res);
    return 0;
}