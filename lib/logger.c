#include "logger.h"

unsigned char g_log_visible=0;
char *g_log_dir="/tmp/";
char *g_log_filename="kb";
char *g_log_ext=".log";
FILE *g_logfd[MAX_THREAD];

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
log_init(
    u32 thrd_num)
{
    char *logfile = NULL;
    SAVE_ALLOC(logfile, (strlen(g_log_dir)+strlen(g_log_filename)+strlen(g_log_ext)+strlen(itoa(thrd_num))+1), char);

    sprintf(logfile, "%s%s%d%s", g_log_dir, g_log_filename, thrd_num, g_log_ext);
    g_logfd[thrd_num] = fopen(logfile, "w");
    // check g_logfd
    if(g_logfd[thrd_num] < 0){
        perror("Cannot open log file descriptor.");
        exit(1);
    }
    free(logfile);
}

int 
log_close(
    u32 thrd_num)
{
    fclose(g_logfd[thrd_num]);
}

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
    SAVE_ALLOC(loginfo, size, char);
    // read the info args into loginfo
    va_start(ap, info_args);
    size=vsnprintf(loginfo, size, info_args, ap);
    va_end(ap);
    loginfo[size-1]='\n'; // next-line
    if(size<0){
        free(loginfo);
        return -1;
    }

    // write into file
    fwrite(loginfo, sizeof(char), strlen(loginfo), g_logfd[get_thrd_tid_from_id(thread_id)]);
    // currently we do not output to screen (too many info that can't be read immediately)

    // free
    free(loginfo);
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
    SAVE_ALLOC(loginfo, size, char);

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
