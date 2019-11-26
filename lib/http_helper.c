#include "http.h"

char *get_http_method_token_by_idx[]={
    [0]=NULL,
    [GET]="GET",
    [METHOD_TOKEN_MAXIMUM]=NULL
};

char *get_http_version_by_idx[]={
    [0]="HTTP/1.1",
    [HTTP_1_0]="HTTP/1.0",
    [HTTP_1_1]="HTTP/1.1",
    [VERSION_MAXIMUM]=NULL
};

char *get_http_status_code_by_idx[]={
    [0]=NULL,
    [_100_CONTINUE]="100",
    [_101_SWITCHING_PROTO]="101",
    [_200_OK]="200",
    [_201_CREATED]="201",
    [_202_ACCEPTED]="202",
    [_203_NON_AUTH_INFO]="203",
    [_204_NO_CONTENT]="204",
    [_205_RESET_CONTENT]="205",
    [_300_MULTI_CHOICES]="300",
    [_301_MOVED_PERMANENTLY]="301",
    [_302_FOUND]="302",
    [_303_SEE_OTHER]="303",
    [_305_USE_PROXY]="305",
    [_306_UNUSED]="306",
    [_307_TEMP_REDIRECT]="307",
    [_400_BAD_REQUEST]="400",
    [_402_PAYMENT_REQUIRED]="402",
    [_403_FORBIDDEN]="403",
    [_404_NOT_FOUND]="404",
    [_405_METHOD_NOT_ALLOWED]="405",
    [_406_NOT_ACCEPTABLE]="406",
    [_408_REQUEST_TIMEOUT]="408",
    [_409_CONFLICT]="409",
    [_410_GONE]="410",
    [_411_LENGTH_REQUIRED]="411",
    [_413_PAYLOAD_TOO_LARGE]="413",
    [_414_URI_TOO_LONG]="414",
    [_415_UNSUPPORTED_MEDIA_TYPE]="415",
    [_417_EXPECTATION_FAILED]="417",
    [_426_UPGRADE_REQUIRED]="426",
    [_500_INTERNAL_SERV_ERR]="500",
    [_501_NOT_IMPL]="501",
    [_502_BAD_GW]="502",
    [_503_SERVICE_UNAVAILABLE]="503",
    [_504_GW_TIMEOUT]="504",
    [_505_HTTP_VER_NOT_SUPPORTED]="505",
    [STATUS_CODE_MAXIMUM]=NULL
};

char *get_http_reason_phrase_by_idx[]={
    [0]=NULL,
    [_100_CONTINUE]="Continue",
    [_101_SWITCHING_PROTO]="Switching Protocols",
    [_200_OK]="OK",
    [_201_CREATED]="Created",
    [_202_ACCEPTED]="Accepted",
    [_203_NON_AUTH_INFO]="Non-Authoritative Information",
    [_204_NO_CONTENT]="No Content",
    [_205_RESET_CONTENT]="Reset Content",
    [_300_MULTI_CHOICES]="Multiple Choices",
    [_301_MOVED_PERMANENTLY]="Moved Permanently",
    [_302_FOUND]="Found",
    [_303_SEE_OTHER]="See Other",
    [_305_USE_PROXY]="Use Proxy",
    [_306_UNUSED]="(Unused)",
    [_307_TEMP_REDIRECT]="Temporary Redirect",
    [_400_BAD_REQUEST]="Bad Request",
    [_402_PAYMENT_REQUIRED]="Payment Required",
    [_403_FORBIDDEN]="Forbidden",
    [_404_NOT_FOUND]="Not Found",
    [_405_METHOD_NOT_ALLOWED]="Method Not Allowed",
    [_406_NOT_ACCEPTABLE]="Not Acceptable",
    [_408_REQUEST_TIMEOUT]="Request Timeout",
    [_409_CONFLICT]="Conflict",
    [_410_GONE]="Gone",
    [_411_LENGTH_REQUIRED]="Length Required",
    [_413_PAYLOAD_TOO_LARGE]="Payload Too Large",
    [_414_URI_TOO_LONG]="URI Too Long",
    [_415_UNSUPPORTED_MEDIA_TYPE]="Unsupported Media Type",
    [_417_EXPECTATION_FAILED]="Expectation Failed",
    [_426_UPGRADE_REQUIRED]="Upgrade Required",
    [_500_INTERNAL_SERV_ERR]="Internal Server Error",
    [_501_NOT_IMPL]="Not Implemented",
    [_502_BAD_GW]="Bad Gateway",
    [_503_SERVICE_UNAVAILABLE]="Service Unavailable",
    [_504_GW_TIMEOUT]="Gateway Timeout",
    [_505_HTTP_VER_NOT_SUPPORTED]="HTTP Version Not Supported",
    [STATUS_CODE_MAXIMUM]=NULL
};

char *get_req_header_name_by_idx [] = {
    [0]=NULL,
    [REQ_ACC]="Accept",
    [REQ_ACC_CHAR]="Accept-Charset",
    [REQ_ACC_ENCODING]="Accept-Encoding",
    [REQ_ACC_LANG]="Accept-Language",
    [REQ_ACC_DATE]="Accept-Datetime",
    [REQ_AUTH]="Authorization",
    [REQ_CACHE_CTRL]="Cache-Control",
    [REQ_CONN]="Connection",
    [REQ_COOKIE]="Cookie",
    [REQ_CONTENT_LEN]="Content-Length",
    [REQ_CONTENT_MD5]="Content-MD5",
    [REQ_CONTENT_TYPE]="Content-Type",
    [REQ_DATE]="Date",
    [REQ_EXPECT]="Expect",
    [REQ_FROM]="From",
    [REQ_HOST]="Host",
    [REQ_IF_MATCH]="If-Match",
    [REQ_IF_MOD_SINCE]="If-Modified-Since",
    [REQ_IF_NONE_MATCH]="If-None-Match",
    [REQ_IF_RANGE]="If-Range",
    [REQ_IF_UNMOD_SINCE]="If-Unmodified-Since",
    [REQ_MAX_FORWARDS]="Max-Forwards",
    [REQ_ORIGIN]="Origin",
    [REQ_PRAGMA]="Pragma",
    [REQ_PROXY_AUTH]="Proxy-Authorization",
    [REQ_RANGE]="Range",
    [REQ_REFERER]="Referer",
    [REQ_TE]="TE",
    [REQ_USER_AGENT]="User-Agent",
    [REQ_UPGRADE]="Upgrade",
    [REQ_VIA]="Via",
    [REQ_WARNING]="Warning",
    [REQ_HEADER_NAME_MAXIMUM]=NULL
};

char *get_res_header_name_by_idx [] = {
    [0]=NULL,
    [RES_ACCESS_CTRL_ALLOW_ORIGIN]="Access-Control-Allow-Origin",
    [RES_ACC_PATCH]="Accept-Patch",
    [RES_ACC_RANGES]="Accept-Ranges",
    [RES_AGE]="Age",
    [RES_ALLOW]="Allow",
    [RES_CACHE_CTRL]="Cache-Control",
    [RES_CONN]="Connection",
    [RES_KEEPALIVE]="Keep-Alive",
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

int encap_http_version(char *version)
{
    if(!strncmp(version, "HTTP/1.0", 8)){
        return HTTP_1_0;
    } else if(!strncmp(version, "HTTP/1.1", 8)){
        return HTTP_1_1;
    } else {
        return 0;
    }
}

int encap_http_method_token(char *method)
{
    if(!strncmp(method, "GET", 3)){
        return GET;
    } else {
        return 0;
    }
}

int encap_http_status_code(int http_status_code)
{
    switch(http_status_code)
    {
        case 100:
            return _100_CONTINUE;
        case 101:
            return _101_SWITCHING_PROTO;
        case 200:
            return _200_OK;
        case 201:
            return _201_CREATED;
        case 202:
            return _202_ACCEPTED;
        case 203:
            return _203_NON_AUTH_INFO;
        case 204:
            return _204_NO_CONTENT;
        case 205:
            return _205_RESET_CONTENT; 
        case 300:
            return _300_MULTI_CHOICES;
        case 301:
            return _301_MOVED_PERMANENTLY;
        case 302:
            return _302_FOUND;
        case 303:
            return _303_SEE_OTHER;
        case 305:
            return _305_USE_PROXY;
        case 306:
            return _306_UNUSED;
        case 307:
            return _307_TEMP_REDIRECT;
        case 400:
            return _400_BAD_REQUEST;
        case 402:
            return _402_PAYMENT_REQUIRED;
        case 403:
            return _403_FORBIDDEN;
        case 404:
            return _404_NOT_FOUND;
        case 405:
            return _405_METHOD_NOT_ALLOWED;
        case 406:
            return _406_NOT_ACCEPTABLE;
        case 408:
            return _408_REQUEST_TIMEOUT;
        case 409:
            return _409_CONFLICT;
        case 410:
            return _410_GONE;
        case 411: 
            return _411_LENGTH_REQUIRED;
        case 413:
            return _413_PAYLOAD_TOO_LARGE;
        case 414:
            return _414_URI_TOO_LONG;
        case 415:
            return _415_UNSUPPORTED_MEDIA_TYPE;
        case 417:
            return _417_EXPECTATION_FAILED;
        case 426:
            return _426_UPGRADE_REQUIRED;
        case 500:
            return _500_INTERNAL_SERV_ERR;
        case 501:
            return _501_NOT_IMPL;
        case 502:
            return _502_BAD_GW;
        case 503:
            return _503_SERVICE_UNAVAILABLE;
        case 504:
            return _504_GW_TIMEOUT;
        case 505:
            return _505_HTTP_VER_NOT_SUPPORTED;
        default:
            return 0;
    }
}
