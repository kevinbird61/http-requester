#include "conn.h"

int create_tcp_conn(const char *target, const char *port)
{
    struct addrinfo *p, hints, *res;
    int sockfd, rv;
    char s[INET6_ADDRSTRLEN];
    memset(&hints, 0, sizeof(hints));

    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_protocol=6;

    if((rv=getaddrinfo(target, port, &hints, &res))!=0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for(p=res; p!=NULL; p=p->ai_next)
    {
        if((sockfd=socket(p->ai_family, p->ai_socktype, p->ai_protocol))==-1)
        {
            perror("invalid socket");
            continue;
        }
        
        if(connect(sockfd, p->ai_addr, p->ai_addrlen)==-1)
        {
            close(sockfd);
            perror("connection failed.");
            continue;
        }
        break;
    }

    if(p==NULL)
    {
        fprintf(stderr, "Failed to connect.\n");
        return 1;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof(s));
    printf("client: connecting to %s\n", s);
    freeaddrinfo(res);

    return sockfd;
}
