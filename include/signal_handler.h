#ifndef __SIGNAL_HANDLER__
#define __SIGNAL_HANDLER__

#include <signal.h>
#include <unistd.h>
#include <stdio.h>

#include "stats.h"

/** http://man7.org/linux/man-pages/man7/signal.7.html
 */

void sig_handler(int signal){
    switch(signal){
        case SIGPIPE: /* Broken pipe: write to pipe with no reader */
            STATS_DUMP(); 
            break;
        case SIGINT:
            STATS_DUMP();
            printf("Abort from SIGINT (ctrl+c), print out the current statistics.\n");
            exit(1);
            break;
        default:
            break;
    }
}

#define SIG_IGNORE(sig)     (signal(sig, SIGIGN))       // ignore this signal
#define SIG_HANDLE(sig)     (signal(sig, sig_handler))


#endif