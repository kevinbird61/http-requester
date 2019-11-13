#include "conn.h"

int main(int argc, char *argv[])
{
    if(argc<3){
        fprintf(stderr, "Usage: ./create_sock.out <target IP> <port>\n");
        exit(1);
    }
    int client_sock_fd=create_tcp_conn(argv[1], argv[2]);
    close(client_sock_fd);
    return 0;
}