#include "conn.h"

char *tcpi_state_str[]={
    [TCP_ESTABLISHED]="TCP-Connection-Established",
    [TCP_SYN_SENT]="TCP-SYN-Sent",
    [TCP_SYN_RECV]="TCP-SYN-Recv",
    [TCP_FIN_WAIT1]="TCP-FIN-Wait-1",
    [TCP_FIN_WAIT2]="TCP-FIN-Wait-2",
    [TCP_TIME_WAIT]="TCP-TIME-Wait",
    [TCP_CLOSE]="TCP-CLOSE",
    [TCP_CLOSE_WAIT]="TCP-CLOSE-Wait",
    [TCP_LISTEN]="TCP-LISTEN",
    [TCP_CLOSING]="TCP-CLOSING",
    /*[TCP_NEW_SYN_RECV]="TCP-NEW-SYN-Recv"*/
};

int 
create_tcp_conn(
    const char *target, 
    const char *port)
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


int 
create_tcp_keepalive_conn(
    const char *target, 
    const char *port,
    int keepcnt,
    int keepidle,
    int keepintvl)
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
        exit(EXIT_FAILURE); 
    }
    printf("SO_KEEPALIVE set on socket\n");
    
    // keepalive interval
    if(setsockopt(sockfd, SOL_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(int)))
    {
        perror("ERROR: setsocketopt(), TCP_KEEPINTVL"); 
        close(sockfd);
        exit(EXIT_FAILURE); 
    }
    printf("TCP_KEEPINTVL: %d sec.\n", keepintvl);
    // keepalive idletime
    if(setsockopt(sockfd, SOL_TCP, TCP_KEEPIDLE, &keepidle, sizeof(int)))
    {
        perror("ERROR: setsocketopt(), TCP_KEEPIDLE"); 
        close(sockfd);
        exit(EXIT_FAILURE); 
    }
    printf("TCP_KEEPIDLE: %d sec.\n", keepidle);
    // keepalive probes
    if(setsockopt(sockfd, SOL_TCP, TCP_KEEPCNT, &keepcnt, sizeof(int)))
    {
        perror("ERROR: setsocketopt(), TCP_KEEPCNT"); 
        close(sockfd);
        exit(0); 
    } 
    printf("TCP_KEEPCNT: %d pkts.\n", keepcnt);
    
    optval=0;
    if(getsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0) {
        perror("getsockopt()");
        close(sockfd);
        exit(EXIT_FAILURE);                     
    }
    printf("SO_KEEPALIVE is %s\n", (optval ? "ON" : "OFF"));

    return sockfd;
}

int 
check_tcp_conn_stat(int sockfd)
{
    struct tcp_info tcp_state;
    int optlen=sizeof(tcp_state);
    if( getsockopt(sockfd, SOL_TCP, TCP_INFO, (void *)&tcp_state, &optlen) < 0){
        perror("getsockopt(...TCP_INFO...)");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    /* Perform the check here */
    printf("[TCP conn state: %s]\n", tcpi_state_str[tcp_state.tcpi_state]);

    return 0;
}

int 
get_tcp_conn_stat(int sockfd)
{
    struct tcp_info tcp_state;
    int optlen=sizeof(tcp_state);
    if( getsockopt(sockfd, SOL_TCP, TCP_INFO, (void *)&tcp_state, &optlen) < 0){
        perror("getsockopt(...TCP_INFO...)");
        return 0;
    }
    
    return tcp_state.tcpi_state;
}