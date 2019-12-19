#include "err_handler.h"

int sock_sent_err_handler(
    void *obj)
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
        case EAGAIN: /* Resource temporarily unavailable */ {
            conn_mgnt_t *mgnt=(conn_mgnt_t *)obj;
            // cannot send, need to adjust the num_gap 
            // puts("Sent EAGAIN");
            mgnt->num_gap>>=1; // ÷2 
            mgnt->num_gap=(mgnt->num_gap==0)?1:mgnt->num_gap; // (+1, prevent 0)
            break;
        }
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

int poll_err_handler()
{
    // need to handle
    switch(errno){
        case EFAULT:    // The array given as argument was not contained in the calling program's address space 
            break;
        case EINTR:     // A signal occurred before any requested event
            break;
        case EINVAL:    // The nfds value exceeds the RLIMIT_NOFILE value.
            break;
        case ENOMEM:    // There was no space to allocate file descriptor tables.
            break;
    }
}