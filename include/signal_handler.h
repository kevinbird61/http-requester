#ifndef __SIGNAL_HANDLER__
#define __SIGNAL_HANDLER__

#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>

#include "stats.h"

/** http://man7.org/linux/man-pages/man7/signal.7.html
 */
struct _protect_t {
    void*               protect_obj;
    struct _protect_t*   next;
};

static struct _protect_t* protect_conn_mgnt; // conn_mgnt list 

static inline void sig_handler(int signal){
    switch(signal){
        case SIGPIPE: /* Broken pipe: write to pipe with no reader */
            STATS_DUMP(); 
            printf("Broken pipe\n");
            exit(1);
        case SIGINT:{
            /* Release all protected obj */
            int free_cm=0;
            while(protect_conn_mgnt!=NULL){
                STATS_CONN((struct _conn_mgnt_t *)protect_conn_mgnt->protect_obj); // store all conn stats from each conn_mgnt
                free(protect_conn_mgnt->protect_obj);
                protect_conn_mgnt=protect_conn_mgnt->next;
                free_cm++;
            }
            STATS_TIME_END(); // calculate total execution time (otherwise will get wrong result: only calculate the starting timestamp)
            STATS_DUMP(); // print statistics
            printf("Abort from SIGINT (ctrl+c), print out the current statistics.\n");
            printf("Release %d conn_mgnt gracefully.\n", free_cm);
            exit(1);
        }
        case SIGSEGV: /* segmentation fault */
            perror("SIGSEGV:");
            printf("File: %s, Line: %d\n", __FILE__, __LINE__);
            exit(1);
        default:
            break;
    }
}

void push_protect(struct _protect_t **protect_list, void *obj){
    if((*protect_list)==NULL){
        (*protect_list)=calloc(1, sizeof(struct _protect_t));
        (*protect_list)->protect_obj=obj;
        (*protect_list)->next=NULL;
    } else {
        struct _protect_t *trav=(*protect_list);
        while(trav->next!=NULL){
            trav=trav->next;
        }
        trav->next=calloc(1, sizeof(struct _protect_t));
        trav->next->protect_obj=obj;
        trav->next->next=NULL;
    }
}

#define SIG_IGNORE(sig)     (signal(sig, SIGIGN))       // ignore this signal
#define SIG_HANDLE(sig)     (signal(sig, sig_handler))
#define SIG_PROTECT_CM(cm)  (push_protect(&protect_conn_mgnt, cm))

#endif