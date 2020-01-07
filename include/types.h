#ifndef __TYPES__
#define __TYPES__

#include "control_var.h"
#include "http_types.h"
#include "global.h"
#include "abnf.h"

/* error code enum (argparse) */
typedef enum {
    /* normal or success */
    ERR_NONE=0,                     // success
    ERR_USE_OTHER_PORT,             // [:PORT]
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

/* maintain available url lists */
struct urls {
    char *url;
    struct urls *next;
};

typedef struct _parsed_args_t {
    struct {
        u8                      have_conn: 1,
                                have_reqs: 1,
                                have_file: 1,
                                have_url: 1,
                                have_port: 1,
                                have_method: 1,
                                have_thrd: 1,
                                rsvd: 1;
    }; // user parameters (see argparse.h)
    u16                         port;
    u16                         thrd;           // threads (MAX=1000)
    u32                         conn;           // connections (per thread)
    u32                         reqs;           // total requests
    union {
        char *                  filename;       // template 
        char *                  url;            // selected url
    };
    struct urls*                urls;           // maintain url lists
    char*                       method;         // 
    char*                       scheme;
    char*                       host;
    char*                       path;
    http_req_header_status_t *  http_req;
    struct {
        u8  use_non_block:1,
            use_probe_mode:1,
            enable_pipe:1,
            use_url:1,
            use_template:1,
            reserved:3;
    };
} parsed_args_t;

#endif
