#include "http.h"

void debug_http_obj(http_t *http_obj)
{
    printf("[Debug][%s][http_t]\n", __FUNCTION__);
    printf("=============Start-Line=============\n");
    printf("[%s][%s]\n", get_http_version(http_obj->version), http_obj->type==REQ? "REQUEST": "RESPONSE");
    printf("[Connect to %s:%d]\n", http_obj->target, http_obj->port);
    if(http_obj->type==REQ){
        printf("[Method: %s][Request Target: %s]\n", get_http_method_token(http_obj->req.method_token), http_obj->req.req_target);
    } else {
        printf("[Status code: %s][Reason Phrase: %s]\n", get_http_status_code(http_obj->req.method_token), http_obj->req.req_target);
    }
    printf("=============Header-Fields=============\n");
    http_header_t *temp=http_obj->headers;
    while(temp!=NULL){
        printf("[%-20s]:[%s]\n", temp->field_name, temp->field_value);
        temp=temp->next;
    }
    printf("=============Message-Body=============\n");
    if(http_obj->msg_body!=NULL){
        printf("%s\n", http_obj->msg_body);
    }
    printf("=============Debug-Done=============\n");
}