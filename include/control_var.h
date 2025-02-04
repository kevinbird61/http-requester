#ifndef __CONTROL_VAR__
#define __CONTROL_VAR__

#include <stddef.h>
#include "types.h"

/**
 * Using rcode to determine that which action it should take.
 */

typedef struct _control_var_t {
    unsigned char   rcode;              // type of return 
    int             num_resp;           // number of finished responses
    int             return_obj_type;    // type of return obj (use to type casting)
    void*           return_obj;         // store the value/obj we want to return 
} control_var_t;

// enum - rcode
enum {
    RCODE_NEXT_RESP=1,          // finish one response, parse next response
    RCODE_POLL_DATA,            // require new data chunk
    RCODE_FIN,                  // finish parsing
    RCODE_CLOSE,                // connection is close-wait/close
    RCODE_REDIRECT,             // 3xx, require redirection
    RCODE_CLIENT_ERR,           // 4xx
    RCODE_SERVER_ERR,           // 5xx
    RCODE_NOT_SUPPORT,          // parsing error - not support
    RCODE_INCOMPLETE,           // parsing error - incomplete
    RCODE_ERROR,                // error
    RCODE_MAXIMUM
};

static char *rcode_str[]={
    [RCODE_NEXT_RESP]=      "RCODE: Parse next response",
    [RCODE_POLL_DATA]=      "RCODE: Require new data chunk",
    [RCODE_FIN]=            "RCODE: Finish parsing",
    [RCODE_CLOSE]=          "RCODE: Connection state is close-wait/close",
    [RCODE_REDIRECT]=       "RCODE: Require redirection",
    [RCODE_CLIENT_ERR]=     "RCODE: Client side error (4xx)",
    [RCODE_SERVER_ERR]=     "RCODE: Server side error (5xx)",
    [RCODE_NOT_SUPPORT]=    "RCODE: Parsing error(Not support)",
    [RCODE_INCOMPLETE]=     "RCODE: Parsing error(Incomplete)",
    [RCODE_ERROR]=          "RCODE: Error",
    [RCODE_MAXIMUM]=        NULL
};

#endif 
