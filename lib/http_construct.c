#include "http.h"

int http_req_create_start_line(char **rawdata, char *method, char *target, u8 http_version)
{
    int h_len=strlen(method)+strlen(get_http_version(http_version))+strlen(target)+4; // + 2 SP + CRLF
    *rawdata=malloc(h_len);
    if(*rawdata==NULL){
        return ERR_MEMORY;
    }
    sprintf(*rawdata, "%s %s %s\r\n", method, target, get_http_version(http_version));

    return ERR_NONE;
}

int http_req_ins_header(char **rawdata, char *field_name, char *field_value)
{
    if(*rawdata==NULL){
        // you need to call http_req_create_start_line first
        return ERR_INVALID;
    }
    int h_len=strlen(field_name)+strlen(field_value)+4; // + 1 SP, 1 colon, CRLF
    *rawdata=realloc(*rawdata, strlen(*rawdata)+h_len);
    if(*rawdata==NULL){
        return ERR_MEMORY;
    }
    sprintf(*rawdata, "%s%s: %s\r\n", *rawdata, field_name, field_value); // concat with previous data

    return ERR_NONE;
}

int http_req_finish(char **rawdata)
{
    int h_len=2; // CRLF
    *rawdata=realloc(*rawdata, strlen(*rawdata)+h_len);
    if(*rawdata==NULL){
        return ERR_MEMORY;
    }
    sprintf(*rawdata, "%s\r\n", *rawdata);

    return ERR_NONE;
}
