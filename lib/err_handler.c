#include "err_handler.h"

/** Errno table:
 * https://www-numi.fnal.gov/offline_software/srt_public_context/WebDocs/Errors/unix_system_errors.html
 */

int sock_sent_err_handler(
    void *obj)
{   
    // printf("Errno: %d\n", errno);
    switch(errno){
        case EINTR: /* A signal occurred before any data was transmitted */
            LOG(0, "Sent: interrupted(EINTR)");
            break;
        case EPIPE: /* Broken pipe (client or server side close its socket) */
            LOG(0, "Sent: broken pipe");
            /* need to create a new connection here? */
            break;
        case ECONNRESET: /* Connection reset by peer */
            LOG(0, "Sent: connection reset by peer");
            goto close_check;
            //break;
        case ECONNREFUSED: { /* Connection refuse, close program */
            LOG(0, "Sent: connection refused");
            goto close_check;
            //break;
        }
        case EAGAIN: /* Resource temporarily unavailable */ {
            // in non-blocking mode will have this errno,
            // means that this operation (e.g. send) would block 
            // may cause by sending too much data 
            // => just let recv part to adjust the num_gap 
            LOG(0, "Sent: resource temporarily unavailable"); 
            // struct _conn_t *conn=(struct _conn_t*)obj;
            // max_req_size/=2;
            break;
        }
        default:
            LOG(0, "Other socket sent errno: %d", errno);
            break;
    }

close_check: {// terminate 
    struct _conn_t *conn=(struct _conn_t*)obj;
    if(conn->retry > MAX_RETRY){
        // LOG(NORMAL, "Cannot retry anymore, close kb");
        printf("Cannot retry anymore, close kb");
        exit(1);
    }
    return 0;
}
}

int sock_recv_err_handler()
{
    switch(errno){
        case EINTR:
            LOG(NORMAL, "Recv: EINTR");
            break;
        case ENOMEM: /* Could not allocate memory for recvmsg(). */
            LOG(NORMAL, "Recv: ENOMEM");
            break;
        default:
            break;
    }
}

int poll_err_handler(void *obj)
{
    // need to handle: https://linux.die.net/man/3/poll, http://man7.org/linux/man-pages/man2/poll.2.html
    LOG(DEBUG, "POLL: Errno=%d", errno);
    switch(errno){
        case EAGAIN:{    // The allocation of internal data structures failed but a subsequent request may succeed.
            //conn_mgnt_t *mgnt=(conn_mgnt_t *)obj;
            /* TODO */
            break;
        }
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