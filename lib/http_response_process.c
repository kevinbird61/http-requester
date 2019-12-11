#include "http.h"

/**
 * HTTP header - conformance checking on header field-name/field-value.
 * 
 * Design:
 * - maintain/support several registered header field.
 * - Usage:
 *      - state-machine architecture, update by insert new fields.
 *      - create an http_h_conf_t instance (per connection), each connection will 
 *          own its parsing state; so need to perform syslog here instead of 
 *          http_state_machine()
 *      - insert new header field while parsing, and then check:
 *          - is the new header field-name conflict with existed fields ?
 *          - is the new header field-name/-value using invalid char ?
 *      - using/following enum error code defined in `types.h`
 */
// create
http_res_header_status_t *
create_http_header_status(char *readbuf)
{
    http_res_header_status_t *http_header_status=malloc(sizeof(http_res_header_status_t));
    memset(http_header_status, 0x00, sizeof(http_res_header_status_t));
    // assign
    http_header_status->buff=readbuf;

    return http_header_status;
}

int 
insert_new_header_field_name(
    http_res_header_status_t *status, 
    u32 idx, u32 offset)
{
    // search input field-name is supported or not
    int check_header=check_res_header_field_name(status, status->buff+(idx-offset));
    // store the enum
    status->curr_bit=check_header;
    
    if(!(check_header>0)){
        /* if not found, then alloc the memory to print */
        LOG(ERROR, "[Field-name] Not support `%s` currently",  strndup(status->buff+(idx-offset), offset-1));
        return ERR_NOT_SUPPORT;
    }
    return ERR_NONE;
}

int 
insert_new_header_field_value(
    http_res_header_status_t *status, 
    u32 idx, u32 offset)
{
    // using curr_bit to store field-value
    if(status->curr_bit>0){
        /** TODO: check the current header with existed header. (check conformance here)
         */
        LOG(DEBUG, "[Field-name: %s]", get_res_header_name_by_idx[status->curr_bit]);
        LOG(DEBUG, "[Field-value: %s]", strndup(status->buff+(idx-offset), offset-1));

        status->field_value[status->curr_bit].idx=(idx-offset);
        status->field_value[status->curr_bit].offset=offset-2;
    } else {
        // if field-name not support, then just discard this field
        return ERR_NOT_SUPPORT;
    }
    return ERR_NONE;
}

// insert & check
int 
check_res_header_field_name(
    http_res_header_status_t *status, 
    char *field_name)
{
    /** FIXME: using dictionary/hash table to optimize searching 
    */ 
    // if found, return enum code 
    // also set the bit to record current idx (for field-value insert)
    for(int i=1;i<RES_HEADER_NAME_MAXIMUM;i++){
        if(!strncasecmp(field_name, get_res_header_name_by_idx[i], strlen(get_res_header_name_by_idx[i]))){
            status->dirty_bit_align|=( ((u64)1)<<(i-1) );
            return i;
        }
    }
    return 0;
}

int 
update_res_header_idx(
    http_res_header_status_t *status, 
    u32 shift_offset)
{
    for(int i=1;i<RES_HEADER_NAME_MAXIMUM;i++){
        if( (status->dirty_bit_align & ( ((u64)1)<<(i-1) )) ){
            // this field need to move 
            // printf("Orig idx: %d ",status->field_value[i].idx);
            status->field_value[i].idx-=shift_offset;
            // printf(", New idx: %d \n",status->field_value[i].idx);
        }
    }
}