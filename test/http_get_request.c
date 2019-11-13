#include "conn.h"

#define MAXDATASIZE 10000

int main(int argc, char *argv[])
{
    if(argc<3){
        fprintf(stderr, "Usage: ./create_sock.out <target IP> <port>\n");
        exit(1);
    }

    int client_sock_fd=create_tcp_conn(argv[1], argv[2]);

    char sendbuf[1024], buf[MAXDATASIZE];

    // send GET request
    snprintf(sendbuf, 1024, "GET /test.html HTTP/1.1\r\nHost: %s\r\n\r\n", argv[1]);
    send(client_sock_fd, sendbuf, strlen(sendbuf), 0);
    // recv the result (test.html is small enough)
    int numbytes=recv(client_sock_fd, buf, MAXDATASIZE-1, 0);
    printf("Recv: %d\n", numbytes);
    if(numbytes==-1)
    {
        perror("error recv");
        close(client_sock_fd);
        exit(1);
    }
    buf[numbytes]='\0';
    printf("client: received\n%s\n", buf);

    close(client_sock_fd);
    return 0;
}