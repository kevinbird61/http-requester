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

struct offset_t {
    u16                     idx;    // record the starting index(address) of the buffer
    u16                     offset; // record the length of the data (you can using memcpy from idx + offset to fetch the entire data)
}; // 4 bytes

// request header (send request)
typedef struct _http_req_header_status_t {
    u8 **field_value;
    u8 **other_field; /* non-standard */
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
} __attribute__((packed)) http_req_header_status_t;

// response header
typedef struct _http_res_header_status_t {
    /** store buffer ptr (start from http message header) 
     * - assign the actual buffer ptr to here when call create func
    */
    u8 *buff;
    // record current bit (to let caller know which header is processing)
    u64 curr_bit;
    u16 msg_hdr_len;
    // store idx & offset of each header field
    struct offset_t field_value[RES_HEADER_NAME_MAXIMUM];
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
    // chunk extension goes here, format: `token = "...(ext)"` (process the result at the end)
    struct offset_t *chunk_ext;
    struct {
        u16         use_chunked:1,
                    http_ver: 3,
                    status_code: 6,
                    is_close: 1,        // notify that we need to close the connection
                    reserved: 5;
    };
} __attribute__((aligned(64))) http_res_header_status_t;    // align with 64

/* for state_machine */
typedef struct _state_machine_t {
    u8                              *buff;                  // buf 
    http_res_header_status_t        *resp;                  // response instances 
    /* idx & offset (size=6*10240, < 16-bits) */
    struct {
        u32                         buf_idx: 16,            // current reading buffer hdr idx
                                    last_fin_idx: 16;       // idx of latest work (resp, or msg hdr, ...)
        u32                         data_size: 16,          // buffer size 
                                    parsed_len: 16;         // parsed data length (e.g. offset)
        u32                         prev_rcv_len: 16,       // previous recv bytes
                                    max_buff_size: 16;
    };
    /* state_m info */
    struct {
        u16                         p_state: 6,             // thrd_num 
                                    thrd_num: 10;           // parsing state
    };
    /* record size */
    union {
        u32                         content_length;
        u32                         chunked_size;
    };
    union {
        u32                         total_content_length;
        u32                         total_chunked_size;
    };
    /* flag */
    struct {
        u32                         use_content_length: 1,
                                    use_chunked: 1,
                                    curr_chunked_size: 30;  /* FIXME: this field should not be here */
    };
} __attribute__((packed)) state_machine_t;

#endif
