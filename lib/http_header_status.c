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

char *get_res_header_name_by_idx [] = {
    [0]=NULL,
    [RES_ACCESS_CTRL_ALLOW_ORIGIN]="Access-Control-Allow-Origin",
    [RES_ACC_PATCH]="Accept-Patch",
    [RES_ACC_RANGES]="Accept-Ranges",
    [RES_AGE]="Age",
    [RES_ALLOW]="Allow",
    [RES_CACHE_CTRL]="Cache-Control",
    [RES_CONN]="Connection",
    [RES_CONTENT_DISPOSITION]="Content-Disposition",
    [RES_CONTENT_ENCODING]="Content-Encoding",
    [RES_CONTENT_LANG]="Content-Language",
    [RES_CONTENT_LEN]="Content-Length",
    [RES_CONTENT_LOC]="Content-Location",
    [RES_CONTENT_MD5]="Content-MD5",
    [RES_CONTENT_RANGE]="Content-Range",
    [RES_CONTENT_TYPE]="Content-Type",
    [RES_DATE]="Date",
    [RES_ETAG]="Etag",
    [RES_EXPIRES]="Expires",
    [RES_LAST_MOD]="Last-Modified",
    [RES_LINK]="Link",
    [RES_LOC]="Location",
    [RES_P3P]="P3P",
    [RES_PRAGMA]="Pragma",
    [RES_PROXY_AUTH]="Proxy-Authenticate",
    [RES_PUBLIC_KEY_PINS]="Public-Key-Pins",
    [RES_REFRESH]="Refresh",
    [RES_RETRY_AFTER]="Retry-After",
    [RES_SERVER]="Server",
    [RES_SET_COOKIE]="Set-Cookie",
    [RES_STATUS]="Status",
    [RES_STRICT_TRANSPORT_SECURITY]="Strict-Transport-Security",
    [RES_TRAILER]="Trailer",
    [RES_TRANSFER_ENCODING]="Transfer-Encoding",
    [RES_UPGRADE]="Upgrade",
    [RES_VARY]="Vary",
    [RES_VIA]="Via",
    [RES_WARNING]="Warning",
    [RES_WWW_AUTH]="WWW-Authenticate",
    [RES_HEADER_NAME_MAXIMUM]=NULL
};

// create
http_header_status_t *create_http_header_status(char *readbuf)
{
    http_header_status_t *http_header_status=malloc(sizeof(http_header_status_t));
    memset(http_header_status, 0x00, sizeof(http_header_status_t));
    // assign
    http_header_status->buff=readbuf;

    return http_header_status;
}

int insert_new_header_field_name(http_header_status_t *status, u32 idx, u32 offset)
{
    // search input field-name is supported or not
    int check_header=check_header_field_name(status, status->buff+(idx-offset));
    // FIXME: store the enum or |=<dirty_bit>
    status->curr_bit=check_header;
    
    if(check_header>=0){
        // TODO: check the current header with existed header. (check conformance here)

        syslog("DEBUG", __func__, "[Field-name: ", get_res_header_name_by_idx[check_header], "]", NULL);
    } else {
        /* if not found, then alloc the memory to print */
        char *tmp=malloc((offset)*sizeof(char));
        snprintf(tmp, offset, "%s", status->buff+(idx-offset));
        syslog("DEBUG", __func__, "[Field-name] Not support `", tmp, "` currently", NULL);
        free(tmp);
        
        return ERR_NOT_SUPPORT;
    }

    return ERR_NONE;
}

int insert_new_header_field_value(http_header_status_t *status, u32 idx, u32 offset)
{
    // [debug] fetch field-value
    char *tmp=malloc((offset)*sizeof(char));
    snprintf(tmp, offset, "%s", status->buff+(idx-offset));
    syslog("DEBUG", __func__,"[Field-value: ", tmp, "]", NULL);
    free(tmp);

    // using curr_bit to store field-value
    if(status->curr_bit>=0){
        status->field_value[status->curr_bit].idx=(idx-offset);
        status->field_value[status->curr_bit].offset=offset;
    } else {
        // if field-name not support, then just discard this field
        return ERR_NOT_SUPPORT;
    }

    return ERR_NONE;
}

// insert & check
int check_header_field_name(http_header_status_t *status, char *field_name)
{
    /** FIXME: using dictionary/hash table to optimize searching 
    */ 
    // if found, return enum code 
    // also set the bit to record current idx (for field-value insert)
    for(int i=1;i<RES_HEADER_NAME_MAXIMUM;i++){
        if(!strncasecmp(field_name, get_res_header_name_by_idx[i], strlen(get_res_header_name_by_idx[i]))){
            status->dirty_bit_align|=(1<<(i-1));
            return i;
        }
    }
}