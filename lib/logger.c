#include "logger.h"

int syslog(const char *action_type, const char *func_name, char *info_args, ...)
{
    va_list ap;
    int alloc_size=64*(1+(strlen(action_type)+strlen(func_name)+25)/64);    // 25= 6(bracket) + 19(getdate)
    char *loginfo=malloc(alloc_size*sizeof(char)), *str;

    sprintf(loginfo, "[%s][%s][%s]", getdate(), action_type, func_name);

    str=info_args;
    va_start(ap, info_args);
    while(str!=NULL){
        if(strlen(str)<=0){ break;}
        if(alloc_size <= (strlen(loginfo)+strlen(str))){
            // realloc 
            alloc_size+=64*(1+strlen(str)/64);
            loginfo=realloc(loginfo, alloc_size*sizeof(char));
            if(loginfo==NULL){
                fprintf(stderr, "Malloc(logger) failure.");
                exit(1);
            }
        }
        sprintf(loginfo, "%s %s", loginfo, str);
        str=va_arg(ap, char *);
    } 
    va_end(ap);
    
    if(strlen(loginfo)==alloc_size){
        alloc_size+=64;
        loginfo=realloc(loginfo, alloc_size*sizeof(char));
    }
    loginfo[strlen(loginfo)]='\n';

    // write into logfile
    char *logfile=malloc((strlen(log_dir)+strlen(log_filename)+strlen(log_ext)+1)*sizeof(char));
    sprintf(logfile, "%s%s%s", log_dir, log_filename, log_ext);
    FILE *logfd=fopen(logfile, "a+");
    fwrite(loginfo, sizeof(char), strlen(loginfo), logfd);
}