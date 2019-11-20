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

// create
http_header_status_t *create_http_header_status(char *readbuf)
{
    http_header_status_t *http_header_status=malloc(sizeof(http_header_status_t));
    http_header_status->buff=readbuf;

    return http_header_status;
}

// insert & check
int insert_new_header_field_name(u32 idx, u16 offset)
{
    // search input field-name is supported or not

    // record current idx (for field-value insert)
}

int insert_new_header_field_value(u32 idx, u16 offset)
{

}