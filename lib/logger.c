#include "logger.h"

int logger(char *info_args, ...)
{
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

    // free
    free(loginfo);
    free(logfile);
    fclose(logfd);

    return 1;
}

int syslog(const char *action_type, const char *func_name, char *info_args, ...)
{
    /* FIXME: How to reduce system call? */
    va_list ap;
    int alloc_size=64*(1+((strlen(action_type)+strlen(func_name)+38)/64));    // 38 = 6(bracket) + 32(getdate)
    char *loginfo=calloc(alloc_size, sizeof(char)), *str;
    if(loginfo==NULL){
        exit(1);
    }
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

    // print to stdout
    // puts(loginfo);

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