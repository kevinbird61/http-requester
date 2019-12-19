#include "err_handler.h"

int sock_sent_err_handler()
{
    switch(errno){
        case EINTR: /* A signal occurred before any data was transmitted */
            puts("Sent EINTR");
            break;
        case EPIPE:
            puts("Sent EPIPE");
            break;
        case ECONNRESET: /* Connection reset by peer */
            puts("Sent ECONNRESET");
            break;
        case EAGAIN: /* Resource temporarily unavailable */
            puts("Sent EAGAIN");
            break;
        default:
            printf("Other socket sent errno: %d\n", errno);
            break;
    }
}

int sock_recv_err_handler()
{
    switch(errno){
        case EINTR:
            puts("Recv EINTR");
            break;
        case ENOMEM: /* Could not allocate memory for recvmsg(). */
            puts("Recv ENOMEM");
            break;
        default:
            break;
    }
}
