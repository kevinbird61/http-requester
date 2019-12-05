#include "conn.h"

int create_tcp_conn(const char *target, const char *port)
{
    struct addrinfo *p, hints, *res;
    int sockfd, rv;
    char s[INET6_ADDRSTRLEN];
    memset(&hints, 0, sizeof(hints));

    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_protocol=6;

    if((rv=getaddrinfo(target, port, &hints, &res))!=0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
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
        return -1;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof(s));
    printf("client: connecting to %s\n", s);
    freeaddrinfo(res);

    return sockfd;
}


int create_tcp_keepalive_conn(const char *target, const char *port)
{
    struct addrinfo *p, hints, *res;
    int sockfd, rv;
    char s[INET6_ADDRSTRLEN];
    memset(&hints, 0, sizeof(hints));

    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_protocol=6;

    if((rv=getaddrinfo(target, port, &hints, &res))!=0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
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
        return -1;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof(s));
    printf("client: connecting to %s\n", s);
    freeaddrinfo(res);

    /* setup SO_KEEPALIVE */
    int optval, optlen;
    
    if(getsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0) {
        perror("getsockopt()");
        close(sockfd);
        exit(EXIT_FAILURE);                     
    }
    printf("SO_KEEPALIVE is %s\n", (optval ? "ON" : "OFF"));
    optval = 1;
    optlen = sizeof(optval);
    if(setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen))
    {
        perror("ERROR: setsocketopt(), SO_KEEPALIVE"); 
        close(sockfd);
        exit(0); 
    }
    printf("SO_KEEPALIVE set on socket\n");
    
    /*
    int keepcnt=5;
    int keepidle=10;
    int keepitval=5;
    if(setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPINTVL, &keepitval, sizeof(int)))
    {
        perror("ERROR: setsocketopt(), TCP_KEEPINTVL"); 
        exit(0); 
    }
    if(setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(int)))
    {
        perror("ERROR: setsocketopt(), TCP_KEEPIDLE"); 
        exit(0); 
    }
    if(setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(int)))
    {
        perror("ERROR: setsocketopt(), TCP_KEEPCNT"); 
        exit(0); 
    }*/


    return sockfd;
}
