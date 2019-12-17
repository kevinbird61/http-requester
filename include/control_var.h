#ifndef __CONTROL_VAR__
#define __CONTROL_VAR__

/**
 * Using rcode to determine that which action it should take.
 */

typedef struct _control_var_t {
    int     rcode;
    int     num_resp;
    int     failed_resp;
    int     return_obj_type;
    void*   return_obj;
} control_var_t;

// enum - rcode
enum {
    RCODE_NEXT_RESP=1,          // finish one response, parse next response
    RCODE_POLL_DATA,            // require new data chunk
    RCODE_FIN,                  // finish parsing
    RCODE_CLOSE,                // connection is close-wait/close
    RCODE_REDIRECT,             // 3xx, require redirection
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
    [RCODE_NOT_SUPPORT]=    "RCODE: Parsing error(Not support)",
    [RCODE_INCOMPLETE]=     "RCODE: Parsing error(Incomplete)",
    [RCODE_ERROR]=          "RCODE: Error",
    [RCODE_MAXIMUM]=        NULL
};

#endif 
