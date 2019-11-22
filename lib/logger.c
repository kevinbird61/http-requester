#include "logger.h"

int syslog_simple(const char *action_type, const char *func_name, char *info_arg)
{
    int alloc_size=64*(1+((strlen(action_type)+strlen(func_name)+strlen(info_arg)+27)/64));
    char *loginfo=malloc(alloc_size*sizeof(char));
    sprintf(loginfo, "[%s][%s][%s] %s\n", getdate(), action_type, func_name, info_arg);

    // write into logfile
    char *logfile=malloc((strlen(log_dir)+strlen(log_filename)+strlen(log_ext)+1)*sizeof(char));
    sprintf(logfile, "%s%s%s", log_dir, log_filename, log_ext);
    FILE *logfd=fopen(logfile, "a");
    fwrite(loginfo, sizeof(char), strlen(loginfo), logfd);
    return 0;
}

int syslog(const char *action_type, const char *func_name, char *info_args, ...)
{
    /* FIXME: How to reduce system call? */
    va_list ap;
    int alloc_size=64*(1+((strlen(action_type)+strlen(func_name)+25)/64));    // 25= 6(bracket) + 19(getdate)
    char *loginfo=malloc(alloc_size*sizeof(char)), *str;
    memset(loginfo, 0x0, alloc_size*sizeof(char));

    sprintf(loginfo, "[%s][%s][%s]", getdate(), action_type, func_name);

    str=info_args;
    va_start(ap, info_args);
    while(str && *info_args++){
        // printf("%s, %ld\n", str, strlen(str));
        if(!strlen(str)){ break;}
        if(alloc_size <= (strlen(loginfo)+strlen(str))){
            // realloc 
            alloc_size=64*(1+(strlen(loginfo)+strlen(str))/64);
            loginfo=realloc(loginfo, alloc_size*sizeof(char));
            if(loginfo==NULL){
                fprintf(stderr, "Malloc(logger) failure.");
                exit(1);
            }
        }
        sprintf(loginfo, "%s %s", loginfo, str);
        str=(char *)va_arg(ap, char *);
    }
    va_end(ap);
    
    if(strlen(loginfo)==alloc_size){
        alloc_size+=64;
        loginfo=realloc(loginfo, alloc_size*sizeof(char));
    }
    sprintf(loginfo, "%s\n", loginfo);

    // write into logfile
    char *logfile=malloc((strlen(log_dir)+strlen(log_filename)+strlen(log_ext)+1)*sizeof(char));
    sprintf(logfile, "%s%s%s", log_dir, log_filename, log_ext);
    FILE *logfd=fopen(logfile, "a+");
    fwrite(loginfo, sizeof(char), strlen(loginfo), logfd);

    // free(str);
    free(loginfo);
    free(logfile);
    fclose(logfd);

    return 0;
}
