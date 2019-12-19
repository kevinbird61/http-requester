#ifndef __TYPES__
#define __TYPES__

#define RECV_BUFF_SCALE     (20)        /* scale x chunk_size = TOTAL_RECV_BUFF */
#define CHUNK_SIZE          (4096)

#include "abnf.h"
#include "control_var.h"
#include "http_types.h"

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

/* user input */
struct urls {
    char *url;
    struct urls *next;
};

typedef struct _parsed_args_t {
    u8                          flags;
    u8                          enable_pipe;
    u8                          use_non_block;
    u32                         port;
    u32                         thrd;
    u32                         conc;
    u32                         conn;
    char*                       filename;
    struct urls*                urls;
    char*                       method;
    char*                       host;
    char*                       path;
    http_req_header_status_t *  http_req;
} parsed_args_t;

#endif
