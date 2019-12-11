#include "logger.h"

unsigned char log_visible=0;

char *log_level_str[]={
    [NORMAL]="NORMAL",
    [SHOWONLY]="SHOWONLY",
    [DEBUG]="DEBUG",
    [INFO]="INFO",
    [WARNING]="WARNING",
    [DANGER]="DANGER",
    [ERROR]="ERROR",
    /* log-only */
    [LOGONLY]="LOG-ONLY",
    [LOG_LEVEL_MAXIMUM]=NULL
};

int 
syslog(
    u8 loglevel,
    char *info_args, ...)
{
    // check the log is enable or not (log_visible will be modified in argparse.c)
    if(log_visible){
        int size=0;
        char *loginfo=NULL;
        va_list ap;

        // determined required size
        va_start(ap, info_args);
        size=vsnprintf(loginfo, size, info_args, ap);
        va_end(ap);
        if(size<0){ return -1;}

        // malloc for size
        size++; // for `\n`
        loginfo=malloc(size);
        if(loginfo==NULL){ return -1;}

        // read the info args into loginfo
        va_start(ap, info_args);
        size=vsnprintf(loginfo, size, info_args, ap);
        va_end(ap);
        loginfo[size-1]='\n'; // next-line
        if(size<0){
            free(loginfo);
            return -1;
        }

        // write into logfile
        char *logfile=malloc((strlen(log_dir)+strlen(log_filename)+strlen(log_ext)+1)*sizeof(char));
        sprintf(logfile, "%s%s%s", log_dir, log_filename, log_ext);
        FILE *logfd=fopen(logfile, "a+");
        fwrite(loginfo, sizeof(char), strlen(loginfo), logfd);

        // write into screen
        if(loglevel<LOGONLY){
            fprintf(stdout, "%s\n", loginfo);
        } 

        // free
        free(loginfo);
        free(logfile);
        fclose(logfd);
    }
    return 1;
}


char *
parse_valist(char *info_args, ...)
{
    int size=0;
    char *loginfo=NULL;
    va_list ap;

    // determined required size
    va_start(ap, info_args);
    size=vsnprintf(loginfo, size, info_args, ap);
    va_end(ap);
    if(size<0){ return NULL;}

    // malloc for size
    size++; 
    loginfo=malloc(size);
    if(loginfo==NULL){ return NULL;}

    // read the info args into loginfo
    va_start(ap, info_args);
    size=vsnprintf(loginfo, size, info_args, ap);
    va_end(ap);
 
    if(size<0){
        free(loginfo);
        return NULL;
    }

    return loginfo;
}