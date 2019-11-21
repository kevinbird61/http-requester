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

typedef enum {
    CACHE_CTRL=0,
    EXPIRES,
    LAST_MOD,
    ETAG,
    CONN,
    KEEPALIVE,
    ACCEPT,
    ACCEPT_CHAR,
    ACCEPT_ENCODING,
    ACCEPT_LANG,
    COOKIE,
    SET_COOKIE,
    CONTENT_LEN,
    CONTENT_TYPE,
    CONTENT_ENCODING,
    CONTENT_LANG,
    CONTENT_LOC,
    HOST,
    USER_AGENT,
    ALLOW,
    SERVER,
    TRANSFER_ENCODING,
    TE,
    TRAILER,
    DATE
} header_name_t;


// create
http_header_status_t *create_http_header_status(char *readbuf)
{
    http_header_status_t *http_header_status=malloc(sizeof(http_header_status_t));
    memset(http_header_status, 0x00, sizeof(http_header_status_t));
    // assign
    http_header_status->buff=readbuf;

    return http_header_status;
}

int insert_new_header_field_name(http_header_status_t *status, u32 idx, u16 offset)
{
    // search input field-name is supported or not
    // char *tmp=malloc((offset-1)*sizeof(char));
    // snprintf(tmp, offset-1, "%s", status->buff+1+(idx-offset));
    int check_header=check_header_field_name(status, status->buff+1+(idx-offset));
    if(check_header>0){
        syslog("DEBUG", __func__, "[Field-name] Found `", decode_header_field_name(check_header), "`", NULL);
    } else {
        char *tmp=malloc((offset-1)*sizeof(char));
        snprintf(tmp, offset-1, "%s", status->buff+1+(idx-offset));
        syslog("DEBUG", __func__, "[Field-name] Not support `", tmp, "` currently", NULL);
        free(tmp);
    }
    // flag the bit and record current idx (for field-value insert)
}

int insert_new_header_field_value(u32 idx, u16 offset)
{

}

// insert & check
int check_header_field_name(http_header_status_t *status, char *field_name)
{
    /* FIXME: using dictionary to optimize searching */
    // if found, return enum code 
    if(!strncasecmp(field_name, "Date", sizeof("Date")-1)){
        status->date_dirty=1;
        return DATE;
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
    } else if(!strncasecmp(field_name, "Accept", 6)) {
        status->accept_dirty=1;
        return ACCEPT;
    } else if(!strncasecmp(field_name, "Accept-Charset", sizeof("Accept-Charset")-1)) {
        status->accept_charset_dirty=1;
        return ACCEPT_CHAR;
    } else if(!strncasecmp(field_name, "Accept-Encoding", sizeof("Accept-Encoding")-1)) {
        status->accept_encoding_dirty=1;
        return ACCEPT_ENCODING;
    } else if(!strncasecmp(field_name, "Accept-Language", sizeof("Accept-Language")-1)) {
        status->accept_language_dirty=1;
        return ACCEPT_LANG;
    } else {
        // not found, just by pass (send 0)
        return -1;
    }
}

char *decode_header_field_name(int header_codename)
{
    switch(header_codename)
    {
        case CACHE_CTRL:
            return "Cache-Control";
        case EXPIRES:
            return "Expires";
        case LAST_MOD:
            return "Last-Modified";
        case ETAG:
            return "Etag";
        case CONN:
            return "Connection";
        case KEEPALIVE:
            return "Keep-Alive";
        case ACCEPT:
            return "Accept";
        case ACCEPT_CHAR:
            return "Accept-Charset";
        case ACCEPT_ENCODING:
            return "Accept-Encoding";
        case ACCEPT_LANG:
            return "Accept-Language";
        case COOKIE:
            return "Cookie";
        case SET_COOKIE:
            return "Set-Cookie";
        case CONTENT_LEN:
            return "Content-Length";
        case CONTENT_TYPE:
            return "Content-Type";
        case CONTENT_ENCODING:
            return "Content-Encoding";
        case CONTENT_LANG:
            return "Content-Language";
        case CONTENT_LOC:
            return "Content-Location";
        case HOST:
            return "Host";
        case USER_AGENT:
            return "User-Agent";
        case ALLOW:
            return "Allow";
        case SERVER:
            return "Server";
        case TRANSFER_ENCODING:
            return "Transfer-Encoding";
        case TE:
            return "TE";
        case TRAILER:
            return "Trailer";
        case DATE:
            return "Date";
        default:
            return NULL;
    }
}