#include "http.h"

#define DEFAULT_FIELD_VAL_LEN   (255)

/******************************************** req obj ********************************************/
int 
http_req_obj_create(http_req_header_status_t **req)
{
    SAVE_ALLOC((*req), 1, http_req_header_status_t);
    // assign dynamic 2d array (e.g. string array)
    SAVE_ALLOC((*req)->field_value, REQ_HEADER_NAME_MAXIMUM, u8 *);
    for(int i=0;i<REQ_HEADER_NAME_MAXIMUM;i++){
        // default length = 255 
        SAVE_ALLOC((*req)->field_value[i], DEFAULT_FIELD_VAL_LEN, char);
    }
    // other field (only support 10 arbitrary request headers)
    SAVE_ALLOC((*req)->other_field, 10, u8 *);
    for(int i=0; i < MAX_ARBIT_REQS; i++){
        (*req)->other_field[i]=NULL;
    }
    return ERR_NONE;
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
    
    return ERR_NONE;
}

/******************************************** req rawdata ********************************************/
int 
http_req_create_start_line(
    char **rawdata, char *method, 
    char *target, u8 http_version)
{
    int h_len=strlen(method)+strlen(get_http_version_by_idx[http_version])+strlen(target)+4; // + 2 SP + CRLF
    SAVE_ALLOC((*rawdata), h_len, char);
    sprintf(*rawdata, "%s %s %s\r\n", method, target, get_http_version_by_idx[http_version]);

    return ERR_NONE;
}

int 
http_req_ins_header(char **rawdata, char *field_name, char *field_value)
{
    if(*rawdata==NULL){
        // you need to call http_req_create_start_line first
        LOG(KB_EH, "You need to call `http_req_create_start_line` first.");
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

    // insert other request header (format of other_field is HDR:VAL)
    for(int i=0; req->other_field[i]!=NULL; i++){
        // fetch field name & value
        char *field_value=strchr(req->other_field[i], ':');
        if(field_value!=NULL){
            field_value[0]='\0';
            ++ field_value; // need to move 1 byte (because [0] is \0 now)
        } else {
            field_value=" "; // if not specified, using SP instead
        }
        char *field_name=req->other_field[i];
        sscanf(req->other_field[i], "%[^:]:%[^:]", field_name, field_value);
        http_req_ins_header(rawdata, field_name, field_value);
    }

    // CRLF 
    int h_len=3; // CRLF
    *rawdata=realloc(*rawdata, strlen(*rawdata)+h_len);
    if(*rawdata==NULL){
        return ERR_MEMORY;
    }
    sprintf(*rawdata, "%s\r\n", *rawdata);
    // LOGONLY
    LOG(KB, "HTTP request header:\n%s", *rawdata);

    return ERR_NONE;
}
