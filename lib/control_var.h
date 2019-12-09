#ifndef __CONTROL_VAR__
#define __CONTROL_VAR__

/**
 * Using rcode to determine that which action it should take.
 */

typedef struct _control_var_t {
    int     rcode;
    int     return_obj_type;
    void*   return_obj;
} control_var_t;

// enum - rcode
enum {
    RCODE_NEXT_RESP=1,
    RCODE_POLL_DATA,
    RCODE_FIN,
    RCODE_REDIRECT,
    RCODE_NOT_SUPPORT,
    RCODE_IMCOMPLETE,
    RCODE_ERROR,
    RCODE_MAX
};

#endif 
