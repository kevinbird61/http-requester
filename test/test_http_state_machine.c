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
    // create connection
    int sockfd=create_tcp_conn(argv[2], argv[3]);
    if(sockfd<0){
        exit(1);
    }
    // using http state machine to process
    http_state_machine(sockfd, (void**)&http_req, 0, 0);

    // close(sockfd);
    return 0;
}