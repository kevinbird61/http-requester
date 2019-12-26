#ifndef __HTTP__
#define __HTTP__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "argparse.h"
#include "logger.h"
#include "stats.h"
#include "utils.h"
#include "types.h"
#include "abnf.h"
#include "conn.h"

/* poll & parse multi-bytes:
 * - state machine need to be an instance (to retain the parsing state
 *   among multiple different responses.)
 * - input: response instance, new data chunk.
 * - output (return): 
 *   1) when the response parsing is finished
 *   2) or state machine found that need to poll a new chunk
 */
http_state next_http_state(http_state cur_state, char ch); // calculate next http parsing state 
state_machine_t *create_parsing_state_machine();
void reset_parsing_state_machine(state_machine_t *state_m);
control_var_t multi_bytes_http_parsing_state_machine(state_machine_t *state_m, int sockfd, u32 num_reqs);
control_var_t multi_bytes_http_parsing_state_machine_non_blocking(state_machine_t *state_m, int sockfd, u32 num_reqs);
control_var_t http_resp_parser(state_machine_t *state_m);

/** http_request_process.c (http_req_header_status_t)
 *  - construct http request header object
 *      - request
 *          - int http_req_obj_create(http_req_header_status_t **req);
 *          - int http_req_obj_ins_header_by_idx(http_req_header_status_t **this, u8 field_name_idx, char *field_value);
 *  - construct http request header
 *      - request
 *          - int http_req_create_start_line(char *rawdata, char *method, char *target, u8 http_version);
 *              rawdata: output 
 *              method: method token
 *              target: resource target
 *              http_version: using enum `http_version_map`
 *          - int http_req_ins_header(char *rawdata, char *field_name, char *field_value);
 *          - int http_req_finish(char *rawdata);
 *              Append CRLF to the end
 */
// req obj
int http_req_obj_create(http_req_header_status_t **req);
int http_req_obj_ins_header_by_idx(http_req_header_status_t **this, u8 field_name_idx, char *field_value);
// req rawdata
int http_req_create_start_line(char **rawdata, char *method, char *target, u8 http_version);
int http_req_ins_header(char **rawdata, char *field_name, char *field_value);
int http_req_finish(char **rawdata, http_req_header_status_t *req);

/** http_response_process.c: (http_res_header_status_t)
 * - check whether the syntax/grammar is conformable with HTTP/1.1 or not.
 * - autocorrect/strip the useless or error part
 */
// create http_header_status_obj
http_res_header_status_t *create_http_header_status(char *readbuf);
// insert & check if there have any conflict
int insert_new_header_field_name(http_res_header_status_t *status, u32 idx, u32 offset);
int check_res_header_field_name(http_res_header_status_t *status, char *field_name);
int insert_new_header_field_value(http_res_header_status_t *status, u32 idx, u32 offset);
int update_res_header_idx(http_res_header_status_t *status, u32 shift_offset);

/** http_helper.c:
 * - encap/decap those mapping code (with prefix `get_*`/`encap_*`)
 */
int encap_http_version(char *version);
int encap_http_method_token(char *method);
int encap_http_status_code(int http_status_code);
u8  get_req_header_name_enum_by_str(char *req_header_field_name);
extern char *get_http_version_by_idx[];
extern char *get_http_method_token_by_idx[];
extern char *get_http_status_code_by_idx[];
extern char *get_http_reason_phrase_by_idx[];
// req/res header name
extern char *get_req_header_name_by_idx[];
extern char *get_res_header_name_by_idx[];

#endif
