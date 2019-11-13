#ifndef __TYPES__
#define __TYPES__

typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;

typedef enum {
    REQ,
    RES
} http_msg_type;

typedef enum {
    TWO_O_O=1,
    THREE_O_O,
    FOUR_O_O,
    FOUR_O_FOUR
} status_code_map;

typedef enum {
    ONE_ZERO,
    ONE_ONE
} http_version_map;

typedef enum {
    GET=1
} method_token_map;

/* using linked-list to store the data */
typedef struct _http_header_t {
    u8                      *field_name;
    u8                      *field_value;
    struct _http_header_t   *next;
} http_header_t;

/* http */
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

#endif
