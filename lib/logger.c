#include "logger.h"

unsigned char g_log_visible=0;
char *g_log_dir="/tmp/";
char *g_log_filename="kb";
char *g_log_ext=".log";

char *g_log_level_str[]={
    [KB_DEBUG]="DEBUG (Developer)",
    [KB]="UNSORT",
    [KB_EH]="ERR_HANDLE",
    [KB_CM]="CONN_MGNT",
    [KB_SM]="STATE_MACHINE",
    [KB_PS]="PARSER",
    [LOG_ALL]="ALL",
    /* log-only */
    [LOGONLY]="LOGONLY",
    [LOG_LEVEL_MAXIMUM]=NULL
};

int 
syslog(
    u8 loglevel,
    u64 thread_id,
    char *info_args, ...)
{
    // check the log is enable or not (log_visible will be modified in argparse.c)
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

    // write into logfile (FIXME: date)
    char *logfile=malloc((strlen(g_log_dir)+strlen(g_log_filename)+strlen(g_log_ext)+strlen(itoa(get_thrd_tid_from_id(thread_id)))+1)*sizeof(char));
    sprintf(logfile, "%s%s%d%s", g_log_dir, g_log_filename, get_thrd_tid_from_id(thread_id), g_log_ext);
    FILE *logfd=fopen(logfile, "a+");
    fwrite(loginfo, sizeof(char), strlen(loginfo), logfd);

    // write into screen
    if(loglevel<LOGONLY){
        fprintf(stdout, "%s", loginfo);
    } 

    // free
    free(loginfo);
    free(logfile);
    fclose(logfd);
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
