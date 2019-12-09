#include "http.h"

#define MAXDATASIZE 20000

int main(int argc, char *argv[])
{
    if(argc<4){
        fprintf(stderr, "Usage: ./program <target IP> <port> <NUM_REQUEST>\n");
        exit(1);
    }

    int client_sock_fd=create_tcp_conn(argv[1], argv[2]);

    // send GET request
    char sendbuf[1024], buf[MAXDATASIZE];
    // test several times
    snprintf(sendbuf, 1024, "GET / HTTP/1.1\r\nHost: %s\r\nConnection: keep-alive\r\n\r\n", argv[1]);
    for(int i=0;i< atoi(argv[3]);i++){
        send(client_sock_fd, sendbuf, strlen(sendbuf), 0);
    }
    //snprintf(sendbuf, 1024, "GET /missing HTTP/1.1\r\nHost: %s\r\n\r\n", argv[1]);  // 4xx will let server-side close the connection
    //send(client_sock_fd, sendbuf, strlen(sendbuf), 0);
    //snprintf(sendbuf, 1024, "GET / HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", argv[1]);
    //send(client_sock_fd, sendbuf, strlen(sendbuf), 0);
    

    control_var_t *control_var=multi_bytes_http_parsing_state_machine(client_sock_fd);
    

    return 0;
}
