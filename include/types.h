#ifndef __TYPES__
#define __TYPES__

#include "control_var.h"
#include "http_types.h"
#include "global.h"
#include "abnf.h"

/* TODO: error code enum (each func need to follow) */
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

/* user input */
struct urls {
    char *url;
    struct urls *next;
};

typedef struct _parsed_args_t {
    u8                          flags;
    struct {
        u8  use_non_block:1,
            use_probe_mode:1,
            enable_pipe:1,
            use_url:1,
            use_template:1,
            reserved:3;
    };
    u16                         port;
    u32                         thrd;
    u32                         conc;
    u32                         conn;
    union {
        char *                  filename;
        char *                  url;
    };
    struct urls*                urls;
    char*                       method;
    char*                       scheme;
    char*                       host;
    char*                       path;
    http_req_header_status_t *  http_req;
} parsed_args_t;

#endif
