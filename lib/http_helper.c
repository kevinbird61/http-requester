#include "http.h"

char *get_http_version(http_version_map http_version)
{
    switch(http_version)
    {
        case ONE_ZERO:
            return "1.0";
        case ONE_ONE:
            return "1.1";
        default:
            return NULL;
    }
}

int encap_http_version(char *version)
{
    if(!strncmp(version, "1.0", 3)){
        return ONE_ZERO;
    } else if(!strncmp(version, "1.1", 3)){
        return ONE_ONE;
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
    switch(status_code)
    {
        case TWO_O_O:
            return "200";
        case THREE_O_O:
            return "300";
        case FOUR_O_O:
            return "400";
        case FOUR_O_FOUR:
            return "404";
        default:
            return NULL;
    }
}

int encap_http_status_code(int http_status_code)
{
    switch(http_status_code)
    {
        case 200:
            return TWO_O_O;
        case 300:
            return THREE_O_O;
        case 400:
            return FOUR_O_O;
        case 404:
            return FOUR_O_FOUR;
        default:
            return 0;
    }
}
