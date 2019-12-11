#include "http.h"

#define DEFAULT_FIELD_VAL_LEN   (255)

/******************************************** req obj ********************************************/
int 
http_req_obj_create(http_req_header_status_t **req)
{
    http_req_header_status_t *http_req=calloc(1, sizeof(http_req_header_status_t));
    // assign dynamic 2d array (e.g. string array)
    http_req->field_value=calloc(REQ_HEADER_NAME_MAXIMUM, sizeof(u8 *));
    for(int i=0;i<REQ_HEADER_NAME_MAXIMUM;i++){
        // default length = 255
        http_req->field_value[i]=calloc(DEFAULT_FIELD_VAL_LEN, sizeof(char));
    }
    *req=http_req;
}

int 
http_req_obj_ins_header_by_idx(
    http_req_header_status_t **this, 
    u8 field_name_idx, char *field_value)
{
    /** TODO: if there have any further operations when some headers have been inserted, 
     *  we need to add those operations here. (e.g. `TE` also must to append its value 
     *  into `Connection` field.)
     */
    if(field_name_idx>0 && field_name_idx<REQ_HEADER_NAME_MAXIMUM){ // check idx range is valid or not
        (*this)->dirty_bit_align|=((u64)1<<(field_name_idx-1)); // -1 (align, because idx=0 is NULL)
        /* TODO: need to check the available length for string */
        (*this)->field_value[field_name_idx]=field_value;
    } else {
        fprintf(stderr, "Invalid field idx range\n");
        // skip or abort?
        exit(1);
    }
}

/******************************************** req rawdata ********************************************/
int 
http_req_create_start_line(
    char **rawdata, char *method, 
    char *target, u8 http_version)
{
    int h_len=strlen(method)+strlen(get_http_version_by_idx[http_version])+strlen(target)+4; // + 2 SP + CRLF
    *rawdata=calloc(h_len, sizeof(char));
    if(*rawdata==NULL){
        return ERR_MEMORY;
    }
    sprintf(*rawdata, "%s %s %s\r\n", method, target, get_http_version_by_idx[http_version]);

    return ERR_NONE;
}

int 
http_req_ins_header(char **rawdata, char *field_name, char *field_value)
{
    if(*rawdata==NULL){
        // you need to call http_req_create_start_line first
        return ERR_INVALID;
    }
    int h_len=strlen(field_name)+strlen(field_value)+4; // + 1 SP, 1 colon, CRLF
    *rawdata=realloc(*rawdata, strlen(*rawdata)+h_len+1);
    if(*rawdata==NULL){
        return ERR_MEMORY;
    }
    sprintf(*rawdata, "%s%s: %s\r\n", *rawdata, field_name, field_value); // concat with previous data

    return ERR_NONE;
}

int 
http_req_finish(
    char **rawdata,
    http_req_header_status_t *req)
{
    // check all the existing header fields, and then insert into rawdata
    for(int i=1;i<REQ_HEADER_NAME_MAXIMUM;i++){
        if(req->dirty_bit_align& (1<<(i-1)) ){
            http_req_ins_header(rawdata, get_req_header_name_by_idx[i], req->field_value[i]);
        }
    }

    // CRLF 
    int h_len=3; // CRLF
    *rawdata=realloc(*rawdata, strlen(*rawdata)+h_len);
    if(*rawdata==NULL){
        return ERR_MEMORY;
    }
    sprintf(*rawdata, "%s\r\n", *rawdata);
    // LOGONLY
    LOG(LOGONLY, "HTTP request header:\n%s", *rawdata);

    return ERR_NONE;
}
