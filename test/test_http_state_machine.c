#include "http.h" 

int main(void)
{
    http_t *http_req=malloc(sizeof(http_t)),
        *http_res=malloc(sizeof(http_t));
    // read & interpret from file and build obj
    http_interpret("test/template/HTTP_GET.txt", http_req);
    // create connection
    int sockfd=create_tcp_conn("kevin.imslab.org", "80");
    // using http state machine to process
    http_state_machine(sockfd, http_req);

    // close(sockfd);
    return 0;
}