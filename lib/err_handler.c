#include "err_handler.h"

/** Errno table:
 * https://www-numi.fnal.gov/offline_software/srt_public_context/WebDocs/Errors/unix_system_errors.html
 */

int sock_recv_err_handler()
{
    switch(errno){
        case EINTR:
            LOG(KB_EH, "Recv: EINTR");
            break;
        case ENOMEM: /* Could not allocate memory for recvmsg(). */
            LOG(KB_EH, "Recv: ENOMEM");
            break;
        default:
            break;
    }
}

int poll_err_handler(void *obj)
{
    // need to handle: https://linux.die.net/man/3/poll, http://man7.org/linux/man-pages/man2/poll.2.html
    LOG(KB_EH, "POLL: Errno=%d", errno);
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