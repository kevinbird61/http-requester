#include "http.h"

/* parse plain-text rawdata into http_t */
int http_parser(char *rawdata, http_t *http_packet)
{
    char *token, *start, *second, *third, *headers, *msg_body, *del="\r\n", *del_body="\r\n\r\n";
    printf("%s\n", rawdata);
    // line -> parse by \r\n
    token = strtok(rawdata, del);
    // start-line
    start = token;
    // header-fields
    token = strtok(NULL, del);
    while(token!=NULL){
        printf("%s\n", token);
        headers=token;
        // header-field -> parse by :
        /*headers=strtok(headers, ":");
        http_packet->headers=malloc(sizeof(http_header_t));
        http_packet->headers->field_name=headers;
        headers=strtok(NULL, ":");
        http_packet->headers->field_value=headers;
        http_packet->headers=http_packet->headers->next;*/

        token = strtok(NULL, del);
    }

    start = strtok(start, " ");
    second = strtok(NULL, " ");
    third = strtok(NULL, " ");

    if(encap_http_method_token(start)>0){
        // request
        http_packet->type=REQ;
        http_packet->req.method_token=encap_http_method_token(start);
        http_packet->req.req_target=second;
        http_packet->version=encap_http_version(third);
    } else {
        // response
        http_packet->type=RES;
        http_packet->res.status_code=encap_http_status_code(atoi(start));
        http_packet->res.rea_phrase=second;
        http_packet->version=encap_http_version(third);
    }
}