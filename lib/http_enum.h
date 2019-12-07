#ifndef __HTTP_UTILS__
#define __HTTP_UTILS__

/* request/response enum */
typedef enum {
    UNDEFINED=0,
    REQ,
    RES
} http_msg_type;

/* version enum */
typedef enum {
    HTTP_1_0=1,
    HTTP_1_1,
    VERSION_MAXIMUM
} http_version_map;

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
    CHUNKED_EXT,
    CHUNKED_DATA,
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
typedef enum {
    RES_ACCESS_CTRL_ALLOW_ORIGIN=1,
    RES_ACC_PATCH,
    RES_ACC_RANGES,
    RES_AGE,
    RES_ALLOW,
    RES_CACHE_CTRL,
    RES_CONN,
    RES_KEEPALIVE,
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
} res_header_name_t;

#endif 