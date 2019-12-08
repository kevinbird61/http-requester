#ifndef __HTTP_TYPES__
#define __HTTP_TYPES__

#include "http_enum.h"

typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;

/* using linked-list to store the header fields */
typedef struct _http_header_t {
    u8                      *field_name;
    u8                      *field_value;
    struct _http_header_t   *next;
} http_header_t;

/** http request/response
 * - current design goal is ease to use
 * - require compact the memory usage
 */
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

struct offset_t {
    u32 idx;    // record the starting index(address) of the buffer
    u32 offset; // record the length of the data (you can using memcpy from idx + offset to fetch the entire data)
}__attribute__((packed)); // 4+4 bytes

// request header (send request)
typedef struct _http_req_header_status_t {
    // dirty bit (record existence), using u64
    union {
        u32 dirty_bit_align;
        struct {
            u32 acc_dirty:1,
                acc_charset_dirty:1,
                acc_encoding_dirty:1,
                acc_lang_dirty:1,
                acc_date_dirty:1,
                auth_dirty:1,
                cache_ctrl_dirty:1,
                conn_dirty:1, // 8 
                cookie_dirty:1,
                content_len_dirty:1,
                content_md5_dirty:1,
                content_type_dirty:1,
                date_dirty:1,
                expect_dirty:1,
                from_dirty:1,
                host_dirty:1, // 8
                if_match_dirty:1,
                if_mod_since_dirty:1,
                if_none_match_dirty:1,
                if_range_dirty:1,
                if_unmod_since_dirty:1,
                max_forwards_dirty:1,
                origin_dirty:1,
                pragma_dirty:1, // 8
                proxy_auth_dirty:1,
                range_dirty:1,
                referer_dirty:1,
                te_dirty:1,
                user_agent_dirty:1,
                upgrade_dirty:1,
                via_dirty:1,
                warning_dirty:1; // 8
        }__attribute__((packed));
    }__attribute__((packed));
    u8 **field_value;
} __attribute__((aligned(64))) http_req_header_status_t;

// response header
typedef struct _http_res_header_status_t {
    // essential fields
    u32 total_payload_length;
    // status-line
    u8 http_ver;
    u8 status_code;
    // dirty bit (record existence), using u64
    union {
        u64 dirty_bit_align;
        struct {
            u64 access_ctrl_allow_origin_dirty:1,
                acc_patch_dirty:1,
                acc_range_dirty:1,
                age_dirty:1,
                allow_dirty:1,
                cache_ctrl_dirty:1,
                conn_dirty:1,
                keep_alive_dirty:1,
                content_disposition_dirty:1,    
                content_encoding_dirty:1,
                content_lang_dirty:1,
                content_len_dirty:1,
                content_loc_dirty:1,
                content_md5_dirty:1,
                content_range_dirty:1,
                content_type_dirty:1,
                date_dirty:1,
                etag_dirty:1,                   
                expires_dirty:1,
                last_mod_dirty:1,
                link_dirty:1,
                loc_dirty:1,
                p3p_dirty:1,
                pragma_dirty:1,
                proxy_auth_dirty:1,
                public_key_pins_dirty:1,        
                refresh_dirty:1,
                retry_after_dirty:1,
                server_dirty:1,
                set_cookie_dirty:1,
                status_dirty:1,
                strict_transport_security_dirty:1,
                trailer_dirty:1,
                transfer_encoding_dirty:1,      
                upgrade_dirty:1,
                vary_dirty:1,
                via_dirty:1,
                warning_dirty:1,
                www_auth_dirty:1,
                spare:1,                        
                spare2:24;
        } __attribute__((packed));
    } __attribute__((packed));
    // record current bit (to let caller know which header is processing)
    u64 curr_bit; 
    // store idx & offset of each header field
    struct offset_t field_value[RES_HEADER_NAME_MAXIMUM];
    // chunk extension goes here, format: `token = "...(ext)"` (process the result at the end)
    struct offset_t *chunk_ext;
    /** store buffer ptr (start from http message header) 
     * - assign the actual buffer ptr to here when call create func
    */
    u8 *buff;
} http_res_header_status_t;    // align with 64

/* for state_machine */
typedef struct _state_machine_t {
    /* buf */
    char *buff;
    /* response instances */
    http_res_header_status_t *resp; 
    /* parsing state */
    u8  p_state;
    /* idx */
    u32 buf_idx;
    u32 parsed_len;
    /* size */
    u32 data_size;
    u32 max_buff_size;
    /* flag */
    u8 use_content_length;
    u8 use_chunked;
    union {
        u32 content_length;
        u32 chunked_size;
    };
    union {
        u32 total_content_length;
        u32 total_chunked_size;
    };
    u32 curr_chunked_size;
} state_machine_t;

#endif