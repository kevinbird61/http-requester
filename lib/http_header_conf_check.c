#include "http.h"

/**
 * HTTP header - conformance checking on header field-name/field-value.
 * 
 * Design:
 * - maintain/support several registered header field.
 *      - Authentication (skip)
 *      - Caching
 *          - [ ] Cache-Control
 *          - [ ] Expires
 *      - Conditionals
 *          - [ ] Last-Modified
 *          - [ ] ETag
 *      - Connection Management
 *          - [ ] Connection
 *          - [ ] Keep-Alive
 *      - Content Negotiation
 *          - [ ] Accept
 *          - [ ] Accept-Charset
 *          - [ ] Accept-Encoding
 *          - [ ] Accept-Language
 *      - Controls (skip)
 *      - Cookies
 *          - [ ] Cookie
 *          - [ ] Set-Cookie
 *      - CORS (discuss)
 *      - Message Body Information
 *          - [ ] Content-Length
 *          - [ ] Content-Type
 *          - [ ] Content-Encoding
 *          - [ ] Content-Language
 *          - [ ] Content-Location
 *      - Proxies 
 *      - Redirects
 *      - Request Context
 *          - [ ] Host
 *          - [ ] User-Agent
 *      - Response context
 *          - [ ] Allow
 *          - [ ] Server
 *      - Range Requests
 *      - Security
 *      - Transfer coding
 *          - [ ] Transfer-Encoding
 *          - [ ] TE
 *          - [ ] Trailer
 *      - WebSockets
 *      - Other
 *          - [ ] Date
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

const char *get_header_name_by_idx [] = {
    [CACHE_CTRL]="Cache-Control",
    [EXPIRES]="Expires",
    [LAST_MOD]="Last-Modified",
    [ETAG]="ETag",
    [CONN]="Connection",
    [KEEPALIVE]="Keep-Alive",
    [ACCEPT]="Accept",
    [ACCEPT_CHAR]="Accept-Charset",
    [ACCEPT_ENCODING]="Accept-Encoding",
    [ACCEPT_LANG]="Accept-Language",
    [COOKIE]="Cookie",
    [SET_COOKIE]="Set-Cookie",
    [CONTENT_LEN]="Content-Length",
    [CONTENT_TYPE]="Content-Type",
    [CONTENT_ENCODING]="Content-Encoding",
    [CONTENT_LANG]="Content-Language",
    [CONTENT_LOC]="Content-Location",
    [HOST]="Host",
    [USER_AGENT]="User-Agent",
    [ALLOW]="Allow",
    [SERVER]="Server",
    [TRANSFER_ENCODING]="Transfer-Encoding",
    [TE]="TE",
    [TRAILER]="Trailer",
    [DATE]="Date",
    [VARY]="Vary",
    [HEADER_NAME_MAXIMUM]=NULL
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
        syslog("DEBUG", __func__, "[Field-name: ", get_header_name_by_idx[check_header], "]", NULL);
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
        status->field_value[status->curr_bit].idx=idx;
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
    /* FIXME: using dictionary to optimize searching */
    // if found, return enum code 
    // also set the bit to record current idx (for field-value insert)
    if(!strncasecmp(field_name, "Date", sizeof("Date")-1)){
        status->date_dirty=1;
        return DATE;
    } else if(!strncasecmp(field_name, "Vary", sizeof("Vary")-1)){
        status->vary_dirty=1;
        return VARY;
    } else if(!strncasecmp(field_name, "Host", sizeof("Host")-1)){
        status->host_dirty=1;
        return HOST;
    } else if(!strncasecmp(field_name, "User-Agent", sizeof("User-Agent")-1)){
        status->user_agent_dirty=1;
        return USER_AGENT;
    } else if(!strncasecmp(field_name, "Server", sizeof("Server")-1)) {
        status->server_dirty=1;
        return SERVER;
    } else if(!strncasecmp(field_name, "Allow", sizeof("Allow")-1)) {
        status->allow_dirty=1;
        return ALLOW;
    } else if(!strncasecmp(field_name, "Cache-Control", sizeof("Cache-Control")-1)) {
        status->cache_control_dirty=1;
        return CACHE_CTRL;
    } else if(!strncasecmp(field_name, "Expires", sizeof("Expires")-1)) {
        status->expires_dirty=1;
        return EXPIRES;
    } else if(!strncasecmp(field_name, "Last-Modified", sizeof("Last-Modified")-1)) {
        status->last_modified_dirty=1;
        return LAST_MOD;
    } else if(!strncasecmp(field_name, "ETag", sizeof("ETag")-1)) {
        status->etag_dirty=1;
        return ETAG;
    } else if(!strncasecmp(field_name, "Connection", sizeof("Connection")-1)) {
        status->connection_dirty=1;
        return CONN;
    } else if(!strncasecmp(field_name, "Keep-Alive", sizeof("Keep-Alive")-1)) {
        status->keepalive_dirty=1;
        return KEEPALIVE;
    } else if(!strncasecmp(field_name, "Transfer-Encoding", sizeof("Transfer-Encoding")-1)) {
        status->transfer_encoding_dirty=1;
        return TRANSFER_ENCODING;
    } else if(!strncasecmp(field_name, "TE", 2)) {
        status->te_dirty=1;
        return TE;
    } else if(!strncasecmp(field_name, "Trailer", 7)) {
        status->trailer_dirty=1;
        return TRAILER;
    } else if(!strncasecmp(field_name, "Content-Length", sizeof("Content-Length")-1)) {
        status->content_length_dirty=1;
        return CONTENT_LEN;
    } else if(!strncasecmp(field_name, "Content-Type", sizeof("Content-Type")-1)) {
        status->content_type_dirty=1;
        return CONTENT_TYPE;
    } else if(!strncasecmp(field_name, "Content-Encoding", sizeof("Content-Encoding")-1)) {
        status->content_encoding_dirty=1;
        return CONTENT_ENCODING;
    } else if(!strncasecmp(field_name, "Content-Location", sizeof("Content-Location")-1)) {
        status->content_location_dirty=1;
        return CONTENT_LOC;
    } else if(!strncasecmp(field_name, "Cookie", 6)) {
        status->cookie_dirty=1;
        return COOKIE;
    } else if(!strncasecmp(field_name, "Set-Cookie", 10)) {
        status->set_cookie_dirty=1;
        return SET_COOKIE;
    } else if(!strncasecmp(field_name, "Accept-Charset", sizeof("Accept-Charset")-1)) {
        status->accept_charset_dirty=1;
        return ACCEPT_CHAR;
    } else if(!strncasecmp(field_name, "Accept-Encoding", sizeof("Accept-Encoding")-1)) {
        status->accept_encoding_dirty=1;
        return ACCEPT_ENCODING;
    } else if(!strncasecmp(field_name, "Accept-Language", sizeof("Accept-Language")-1)) {
        status->accept_language_dirty=1;
        return ACCEPT_LANG;
    } else if(!strncasecmp(field_name, "Accept", strlen(field_name))) { // prevent "Accept-*" matching
        status->accept_dirty=1;
        return ACCEPT;
    } else {
        // not found, just by pass (send 0)
        return -1;
    }
}