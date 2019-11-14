#include "http.h"

struct header_token_list_t {
    char *token;
    struct header_token_list_t *next;
};

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
    printf("Msg Body: %s\n", msg_body);
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
        printf("Header: %s\n", header_lists->token);
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

    while(http_packet->headers!=NULL){
        printf("[Field- %s][Value- %s]\n", http_packet->headers->field_name, http_packet->headers->field_value);
        http_packet->headers=http_packet->headers->next;
    }

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

        printf("[Start-line (REQ)]: %s %s %s\n", 
            get_http_method_token(http_packet->req.method_token), 
            http_packet->req.req_target,
            get_http_version(http_packet->version));
    } else {
        // response (remind the sequence)
        http_packet->type=RES;
        http_packet->res.status_code=encap_http_status_code(atoi(second));
        http_packet->res.rea_phrase=third;
        http_packet->version=encap_http_version(start);

        printf("[Start-line (RES)]: %s %s %s\n", 
            get_http_version(http_packet->version), 
            get_http_status_code(http_packet->res.status_code),
            http_packet->res.rea_phrase);
    }
}