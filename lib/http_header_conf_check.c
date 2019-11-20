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

struct offset_t {
    u16 idx;    // record the starting index(address) of the buffer
    u16 offset; // record the length of the data (you can using memcpy from idx + offset to fetch the entire data)
}__attribute__((packed)); // 4 bytes

typedef struct _http_header_status_t {
    // dirty bit (record existence), using 32 bits to store
    u32 host_dirty: 1,
        user_agent_dirty: 1,
        allow_dirty: 1,
        server_dirty: 1,
        transfer_encoding_dirty: 1,
        te_dirty: 1,
        trailer_dirty: 1,
        date_dirty: 1,
        cache_control_dirty: 1,
        expires_dirty: 1,
        last_modified_dirty: 1,
        etag_dirty: 1,
        connection_dirty: 1,
        keepalive_dirty: 1,
        spare: 2,
        accept_dirty: 1,
        accept_charset_dirty: 1,
        accept_encoding_dirty: 1,
        accept_language_dirty: 1,
        cookie_dirty: 1,
        set_cookie_dirty: 1,
        spare2: 2,
        content_length_dirty: 1,
        content_type_dirty: 1,
        content_encoding_dirty: 1,
        content_language_dirty: 1,
        content_location_dirty: 1,
        spare3: 3;
    u32 curr_bit; // record current bit (to let caller know which header is processing)
    // store actual data 
    struct offset_t host, user_agent, allow, server, date;
    struct offset_t cache_control, expires, last_modified, etag, connection, keepalive;
    struct offset_t accept, accept_charset, accept_encoding, accept_language, cookie, set_cookie; /* FIXME: cookie & set_cookie could be multiple */
    struct offset_t content_type, content_encoding, content_language, content_location;
    // conflict part
    union {
        struct offset_t transfer_encoding, te;
        struct offset_t content_length;
    };
} http_header_status_t;

// init
http_header_status_t create_http_header_status();
// insert & check
int insert_new_header_field_name();
int insert_new_header_field_value();