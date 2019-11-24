#include "http.h"

char *get_http_version(http_version_map http_version)
{
    switch(http_version)
    {
        case HTTP_1_0:
            return "HTTP/1.0";
        case HTTP_1_1:
            return "HTTP/1.1";
        default:
            // default using 1.1 
            return "HTTP/1.1";
    }
}

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

char *get_http_method_token(method_token_map method_token)
{
    switch(method_token)
    {
        case GET: 
            return "GET";
        default:
            return NULL;
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

char *get_http_status_code(status_code_map status_code)
{
    /* TODO: support more status code */
    switch(status_code)
    {
        case _100_CONTINUE:
            return "100";
        case _101_SWITCHING_PROTO:
            return "101";
        case _200_OK:
            return "200";
        case _201_CREATED:
            return "201";
        case _202_ACCEPTED:
            return "202";
        case _203_NON_AUTH_INFO:
            return "203";
        case _204_NO_CONTENT:
            return "204";
        case _205_RESET_CONTENT:
            return "205";
        case _300_MULTI_CHOICES:
            return "300";
        case _301_MOVED_PERMANENTLY:
            return "301";
        case _302_FOUND:
            return "302";
        case _303_SEE_OTHER:
            return "303";
        case _305_USE_PROXY:
            return "305";
        case _306_UNUSED:
            return "306";
        case _307_TEMP_REDIRECT:
            return "307";
        case _400_BAD_REQUEST:
            return "400";
        case _402_PAYMENT_REQUIRED:
            return "402";
        case _403_FORBIDDEN:
            return "403";
        case _404_NOT_FOUND:
            return "404";
        case _405_METHOD_NOT_ALLOWED:
            return "405";
        case _406_NOT_ACCEPTABLE:
            return "406";
        case _408_REQUEST_TIMEOUT:
            return "408";
        case _409_CONFLICT:
            return "409";
        case _410_GONE:
            return "410";
        case _411_LENGTH_REQUIRED:
            return "411";
        case _413_PAYLOAD_TOO_LARGE:
            return "413";
        case _414_URI_TOO_LONG:
            return "414";
        case _415_UNSUPPORTED_MEDIA_TYPE:
            return "415";
        case _417_EXPECTATION_FAILED:
            return "417";
        case _426_UPGRADE_REQUIRED:
            return "426";
        case _500_INTERNAL_SERV_ERR:
            return "500";
        case _501_NOT_IMPL:
            return "501";
        case _502_BAD_GW:
            return "502";
        case _503_SERVICE_UNAVAILABLE:
            return "503";
        case _504_GW_TIMEOUT:
            return "504";
        case _505_HTTP_VER_NOT_SUPPORTED:
            return "505";
        default:
            return NULL;
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
