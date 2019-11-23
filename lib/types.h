#ifndef __TYPES__
#define __TYPES__

/**
 * Useful refs:
 * - Response Header Fields: https://tools.ietf.org/html/rfc7231#section-7
 */

typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;

/* TODO: error code enum (each func need to follow) */
typedef enum {
    ERR_NONE=0,                     // success
    ERR_ILLEGAL_CHAR,               // parse illegal char
    ERR_NOT_SUPPORT,
    ERR_UNDEFINED
} error_code;

/* request/response enum */
typedef enum {
    UNDEFINED=0,
    REQ,
    RES
} http_msg_type;

/** status code enum 
 * - https://tools.ietf.org/html/rfc7231#section-6
 */
typedef enum {
    // Information 1xx
    _100_CONTINUE=1,
    _101_SWITCHING_PROTO,
    // Successful 2xx
    _200_OK,
    _201_CREATED,
    _202_ACCEPTED,
    _203_NON_AUTH_INFO,
    _204_NO_CONTENT,
    _205_RESET_CONTENT,
    // Redirection 3xx
    _300_MULTI_CHOICES,
    _301_MOVED_PERMANENTLY,
    _302_FOUND,
    _303_SEE_OTHER,
    _305_USE_PROXY,
    _306_UNUSED,
    _307_TEMP_REDIRECT,
    // Client Error 4xx
    _400_BAD_REQUEST,
    _402_PAYMENT_REQUIRED,
    _403_FORBIDDEN,
    _404_NOT_FOUND,
    _405_METHOD_NOT_ALLOWED,
    _406_NOT_ACCEPTABLE,
    _408_REQUEST_TIMEOUT,
    _409_CONFLICT,
    _410_GONE,
    _411_LENGTH_REQUIRED,
    _413_PAYLOAD_TOO_LARGE,
    _414_URI_TOO_LONG,
    _415_UNSUPPORTED_MEDIA_TYPE,
    _417_EXPECTATION_FAILED,
    _426_UPGRADE_REQUIRED,
    // Server Error 5xx
    _500_INTERNAL_SERV_ERR,
    _501_NOT_IMPL,
    _502_BAD_GW,
    _503_SERVICE_UNAVAILABLE,
    _504_GW_TIMEOUT,
    _505_HTTP_VER_NOT_SUPPORTED
} status_code_map;

/* version enum */
typedef enum {
    ONE_ZERO=1,
    ONE_ONE
} http_version_map;

/* TODO: method token enum */
typedef enum {
    GET=1
} method_token_map;

// http parsing state
typedef enum {
    // start-line states
    START_LINE=1,
    VER,
    CODE_OR_TOKEN,
    REASON_OR_RESOURCE,
    // header fields
    HEADER,
    FIELD_NAME,
    FIELD_VALUE,
    // msg body
    MSG_BODY,
    // chunk
    CHUNKED,
    // terminate successfully
    END,
    // terminate with error 
    ABORT
} http_state;

// character
typedef enum {
    NON,
    CR,
    LF,
    CRLF,
    NEXT
} parse_state;

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
    DATE,
    VARY,
    LOCATION,
    HEADER_NAME_MAXIMUM
} header_name_t;

/* using linked-list to store the header fields */
typedef struct _http_header_t {
    u8                      *field_name;
    u8                      *field_value;
    struct _http_header_t   *next;
} http_header_t;

/** http request/response
 * - current design goal is ease to use
 * - require compact the memory usage
 */
typedef struct _http_t {
    /* statistics */
    u8                      type;               // request/response
    u32                     content_length;
    u8                      *target;            // target/peer IP
    u16                     port;               // port
    /* start-line */
    union {
        struct _req_t {
            u8              method_token;       // method 
            u8              *req_target;        // request-target
        } __attribute__((packed)) req;
        struct _res_t {
            u8              status_code;        // status code
            u8              *rea_phrase;        // reason-phrase
        } __attribute__((packed)) res;
    };
    u8                      version;
    /* header field */
    http_header_t           *headers;
    /* message body */
    u8                      *msg_body;
}__attribute__((packed)) http_t;

struct offset_t {
    u32 idx;    // record the starting index(address) of the buffer
    u16 offset; // record the length of the data (you can using memcpy from idx + offset to fetch the entire data)
}__attribute__((packed)); // 4+4 bytes

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
        vary_dirty: 1,
        cache_control_dirty: 1,
        expires_dirty: 1,
        last_modified_dirty: 1,
        etag_dirty: 1,
        connection_dirty: 1,
        keepalive_dirty: 1,
        location_dirty: 1,
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
    int curr_bit; // record current bit (to let caller know which header is processing)
    // store idx & offset of each header field
    struct offset_t field_value[HEADER_NAME_MAXIMUM];
    /** store buffer ptr (start from http message header) 
     * - assign the actual buffer ptr to here when call create func
    */
    u8 *buff;
} __attribute__((aligned(64))) http_header_status_t;    // align with 64


#endif
