#include "http.h"

int http_interpret(const char *filename, http_t *http_packet)
{
    FILE *fp=fopen(filename, "r");
    if(fp==NULL){
        exit(1);
    }
    char *buf=NULL, *req=NULL;
    size_t len=0; 
    ssize_t read;
    // read from file, and then parse
    while ((read = getline(&buf, &len, fp)) != -1) {
        //printf("Retrieved line of length %zu:\n", read);
        //printf("%s", buf);
        if(req==NULL){
            req=malloc((read)*sizeof(char));
            if(req==NULL){
                // syslog
                exit(1);
            }
            sprintf(req, "%s", buf);
        } else {
            req=realloc(req, (strlen(req)+read)*sizeof(char));
            if(req==NULL){
                // syslog
                exit(1);
            }
            sprintf(req, "%s%s", req, buf);
        }
    }
    req=realloc(req, (strlen(req)+2)*sizeof(char));
    if(req==NULL){
        // syslog
        exit(1);
    }
    sprintf(req, "%s\r\n", req);

    // parsing 
    http_parser(req, http_packet);

    // free
    free(buf);
    free(req);
    fclose(fp);
}