#include "http.h"
#include "logger.h"

struct header_token_list_t {
    char *token;
    struct header_token_list_t *next;
};

/**
 * - recast http_obj back into rawdata string, ease for sending
 */
int http_recast(http_t *http_packet, char **rawdata)
{
    // default req size is 32, using realloc to scale
    int limit=32;
    if(*rawdata!=NULL){
        free(*rawdata);
    } 
    *rawdata=malloc(limit*sizeof(char));
    
    /**************************************************** start line ****************************************************/
    int size_start_line=0;
    if(http_packet->type==REQ || http_packet->type==UNDEFINED){
        // REQUEST/UNDEFINED
        char *method=get_http_method_token(http_packet->req.method_token),
            *req_target=http_packet->req.req_target,
            *http_ver=get_http_version(http_packet->version);
        // check req size (+9: other SP & HTTP)
        size_start_line=strlen(method)+strlen(req_target)+strlen(http_ver)+4;
        syslog("DEBUG", __func__, "Recasting a HTTP request. Start-line: ", method, " ", req_target, " ", http_ver);
        if(limit<=size_start_line){
            limit=size_start_line;
            *rawdata=realloc(*rawdata, (limit)*sizeof(char));
        }
        // snprintf(req, size_start_line, "%s %s HTTP/%s\r\n", method, req_target, http_ver);
        sprintf(*rawdata, "%s %s %s\r\n", method, req_target, http_ver);

    } else if(http_packet->type==RES) {
        char *status=get_http_status_code(http_packet->res.status_code),
            *rea_phrase=http_packet->res.rea_phrase,
            *http_ver=get_http_version(http_packet->version);
        // check req size (+9: other SP & HTTP)
        size_start_line=strlen(status)+strlen(rea_phrase)+strlen(http_ver)+4;
        syslog("DEBUG", __func__, "Recasting a HTTP response. Start-line: ", http_ver, " ", status, " ", rea_phrase);
        if(limit<=size_start_line){
            limit=size_start_line;
            *rawdata=realloc(*rawdata, (limit)*sizeof(char));
        }
        // snprintf
        sprintf(*rawdata, "%s %s %s\r\n", http_ver, status, rea_phrase);
    }
    
    // printf("HTTP header: %s (%p)\n", req, &req);
    /**************************************************** header fields ****************************************************/
    while(http_packet->headers!=NULL && http_packet->headers->field_name!=NULL)
    {
        int header_size=(strlen(http_packet->headers->field_name)+strlen(http_packet->headers->field_value)+4);
        char *buf=malloc(header_size*sizeof(char));
        // snprintf(buf, header_size, "%s: %s\r\n", http_request->headers->field_name, http_request->headers->field_value);
        sprintf(buf, "%s: %s\r\n", http_packet->headers->field_name, http_packet->headers->field_value);
        syslog("DEBUG", __func__, http_packet->headers->field_name, ": ", http_packet->headers->field_value);
        // printf("Buf: %s\n", buf);
        // check size, if not enough, scale it
        if(limit<=(strlen(buf)+strlen(*rawdata))){
            limit+=(strlen(buf));
            *rawdata=realloc(*rawdata, limit*sizeof(char));
        }
        // snprintf(req, limit, "%s%s", req, buf);
        sprintf(*rawdata, "%s%s", *rawdata, buf);
        http_packet->headers=http_packet->headers->next;
        // printf("HTTP header: %s (%p)\n", req, &req);
        free(buf);
    }
    // printf("Size: %d, strlen(req): %ld\n", limit, strlen(req));
    // printf("HTTP header: %s (%p)\n", req, &req);
    *rawdata=realloc(*rawdata, (limit+=2)*sizeof(char));
    // snprintf(req, limit, "%s\r\n", req);
    sprintf(*rawdata, "%s\r\n", *rawdata);

    // print for debug
    // printf("HTTP header:\n%s\n", rawdata);
}

/** (Need optimized & refactor) 
 * - parse plain-text rawdata into http_t 
 * - linked-list need to fix the memory abuse (1~2 unused nodes)
 */
int http_parser(char *rawdata, http_t *http_packet)
{
    char *token, *start, *second, *third, *headers, *msg_body, 
        *del="\r\n", *del_body="\r\n\r\n";
    /* msg body */
    msg_body=strstr(rawdata, del_body);
    http_packet->msg_body=msg_body;
    // printf("Msg Body: %s\n", http_packet->msg_body);
    // printf("Sizeof(rawdata): %ld; strlen(rawdata): %ld\n", sizeof(rawdata), strlen(rawdata));

    /* header */
    struct header_token_list_t *header_lists=malloc(sizeof(struct header_token_list_t)),
        *temp=header_lists;
    header_lists->next=NULL;
    headers=malloc((strlen(rawdata)-strlen(msg_body))*sizeof(char));
    memcpy(headers, rawdata, strlen(rawdata)-strlen(msg_body));
    // printf("%s\n", headers);
    // line -> parse by \r\n
    token = strtok(headers, del);
    // start-line (process it later)
    start = token;

    // split header-fields
    token = strtok(NULL, del);
    while(token!=NULL){
        // printf("%s\n", token);
        if(temp!=NULL){
            temp->token=token;
            temp->next=malloc(sizeof(struct header_token_list_t));
            temp=temp->next;
            temp->next=NULL;
        }
        token = strtok(NULL, del);
    }

    /* parse header-fields */
    http_packet->headers=malloc(sizeof(http_header_t));
    http_header_t *htemp=http_packet->headers;
    while(header_lists!=NULL){
        // printf("Header: %s\n", header_lists->token);
        // header-field -> parse by :
        headers=strtok(header_lists->token, ":");
        if(htemp!=NULL){
            htemp->field_name=headers;
            headers=strtok(NULL, ":");
            htemp->field_value=headers;
            htemp->next=malloc(sizeof(http_header_t));
            htemp=htemp->next;
        }
        header_lists=header_lists->next;
    }

    /*
    while(http_packet->headers!=NULL){
        printf("[Field- %s][Value- %s]\n", http_packet->headers->field_name, http_packet->headers->field_value);
        http_packet->headers=http_packet->headers->next;
    }
    */

    /* parse start-line */
    start = strtok(start, " ");
    second = strtok(NULL, " ");
    third = strtok(NULL, " ");
    
    if(encap_http_method_token(start)>0){
        // request
        http_packet->type=REQ;
        http_packet->req.method_token=encap_http_method_token(start);
        http_packet->req.req_target=second;
        http_packet->version=encap_http_version(third);

        /*printf("[Start-line (REQ)]: %s %s %s\n", 
            get_http_method_token(http_packet->req.method_token), 
            http_packet->req.req_target,
            get_http_version(http_packet->version));*/
    } else {
        // response (remind the sequence)
        http_packet->type=RES;
        http_packet->res.status_code=encap_http_status_code(atoi(second));
        http_packet->res.rea_phrase=third;
        http_packet->version=encap_http_version(start);

        /*printf("[Start-line (RES)]: %s %s %s\n", 
            get_http_version(http_packet->version), 
            get_http_status_code(http_packet->res.status_code),
            http_packet->res.rea_phrase);*/
    }
}