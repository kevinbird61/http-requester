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
    /* normal or success */
    ERR_NONE=0,                     // success
    ERR_USE_SSL_PORT,               // change to SSL port - 443 (parse url)
    ERR_REDIRECT,                   // redirect (http_state_machine)
    /* invalid or illegal */
    ERR_INVALID,
    ERR_INVALID_HOST_LEN,           // invalid host length (parse url)
    ERR_ILLEGAL_CHAR,               // parse illegal char
    ERR_MEMORY,                     // memory-related error
    ERR_NOT_SUPPORT,                // not support
    /* undefined */
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
    _505_HTTP_VER_NOT_SUPPORTED,
    STATUS_CODE_MAXIMUM
} status_code_map;

/* version enum */
typedef enum {
    HTTP_1_0=1,
    HTTP_1_1,
    VERSION_MAXIMUM
} http_version_map;

/* TODO: method token enum */
typedef enum {
    GET=1,
    METHOD_TOKEN_MAXIMUM
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
    NEXT_CHUNKED,
    // terminate successfully
    END,
    // terminate with error 
    ABORT
} http_state;

// request header fields-names
typedef enum {
    REQ_ACC=1,
    REQ_ACC_CHAR,
    REQ_ACC_ENCODING,
    REQ_ACC_LANG,
    REQ_ACC_DATE,
    REQ_AUTH,
    REQ_CACHE_CTRL,
    REQ_CONN,
    REQ_COOKIE,
    REQ_CONTENT_LEN,
    REQ_CONTENT_MD5,
    REQ_CONTENT_TYPE,
    REQ_DATE,
    REQ_EXPECT,
    REQ_FROM,
    REQ_HOST,
    REQ_IF_MATCH,
    REQ_IF_MOD_SINCE,
    REQ_IF_NONE_MATCH,
    REQ_IF_RANGE,
    REQ_IF_UNMOD_SINCE,
    REQ_MAX_FORWARDS,
    REQ_ORIGIN,
    REQ_PRAGMA,
    REQ_PROXY_AUTH,
    REQ_RANGE,
    REQ_REFERER,
    REQ_TE,
    REQ_USER_AGENT,
    REQ_UPGRADE,
    REQ_VIA,
    REQ_WARNING,
    REQ_HEADER_NAME_MAXIMUM // last one
} req_header_name_t;

// "new" response header field-names
enum {
    RES_ACCESS_CTRL_ALLOW_ORIGIN=1,
    RES_ACC_PATCH,
    RES_ACC_RANGES,
    RES_AGE,
    RES_ALLOW,
    RES_CACHE_CTRL,
    RES_CONN,
    RES_CONTENT_DISPOSITION,
    RES_CONTENT_ENCODING,
    RES_CONTENT_LANG,
    RES_CONTENT_LEN,
    RES_CONTENT_LOC,
    RES_CONTENT_MD5,
    RES_CONTENT_RANGE,
    RES_CONTENT_TYPE,
    RES_DATE,
    RES_ETAG,
    RES_EXPIRES,
    RES_LAST_MOD,
    RES_LINK,
    RES_LOC,
    RES_P3P, 
    RES_PRAGMA,
    RES_PROXY_AUTH,
    RES_PUBLIC_KEY_PINS,
    RES_REFRESH,
    RES_RETRY_AFTER,
    RES_SERVER,
    RES_SET_COOKIE,
    RES_STATUS,
    RES_STRICT_TRANSPORT_SECURITY,
    RES_TRAILER,
    RES_TRANSFER_ENCODING,
    RES_UPGRADE,
    RES_VARY,
    RES_VIA,
    RES_WARNING,
    RES_WWW_AUTH,
    RES_HEADER_NAME_MAXIMUM // last one
};

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

// response header
typedef struct _http_res_header_status_t {
    // dirty bit (record existence)
    // using u64
    union {
        u64 dirty_bit_align;
        u64 access_ctrl_allow_origin_dirty:1,
            acc_patch_dirty:1,
            acc_range_dirty:1,
            age_dirty:1,
            allow_dirty:1,
            cache_ctrl_dirty:1,
            conn_dirty:1,
            content_disposition_dirty:1,    
            content_encoding_dirty:1,
            content_lang_dirty:1,
            content_len_dirty:1,
            content_loc_dirty:1,
            content_md5_dirty:1,
            content_range_dirty:1,
            content_type_dirty:1,
            date_dirty:1,
            etag_dirty:1,                   
            expires_dirty:1,
            last_mod_dirty:1,
            link_dirty:1,
            loc_dirty:1,
            p3p_dirty:1,
            pragma_dirty:1,
            proxy_auth_dirty:1,
            public_key_pins_dirty:1,        
            refresh_dirty:1,
            retry_after_dirty:1,
            server_dirty:1,
            set_cookie_dirty:1,
            status_dirty:1,
            strict_transport_security_dirty:1,
            trailer_dirty:1,
            transfer_encoding_dirty:1,      
            upgrade_dirty:1,
            vary_dirty:1,
            via_dirty:1,
            warning_dirty:1,
            www_auth_dirty:1,
            spare:2,                        
            spare2:24;
    };
    
    u64 curr_bit; // record current bit (to let caller know which header is processing)
    // store idx & offset of each header field
    struct offset_t field_value[RES_HEADER_NAME_MAXIMUM];
    /** store buffer ptr (start from http message header) 
     * - assign the actual buffer ptr to here when call create func
    */
    u8 *buff;
} __attribute__((aligned(64))) http_header_status_t;    // align with 64


#endif
