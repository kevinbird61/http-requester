#ifndef __HTTP__
#define __HTTP__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logger.h"
#include "utils.h"
#include "types.h"
#include "conn.h"

/** http_state_machine.c
 * - check and send the request(s), if not pass the checking, function will return error code.
 * - send out the request(s)
 * - using HTTP state machine to parse/analyze the data until finish.
 *      - if there has any 3xx status code, we need to send the request again, also require the "Location" field-value.
 *      - if there has any field-name/value violate RFC7230, abort the process and return error code. 
 *      - log everything.
 * - after finish the request, log the response and close the socket, free the memory.
 */
int http_state_machine(int sockfd, http_t *http_request);

/* http_request.c: 
 * - send out the request, and store the return data on which rawdata points to.
 */
int http_request(int sockfd, http_t *http_request, char *rawdata);

/* http_parser.c: 
 * - parsing the rawdata, and fill them into data structure (response).
 */
int http_parser(char *rawdata, http_t *http_packet);
int http_recast(http_t *http_packet, char **rawdata);

/** http_interpret.c:
 * - interpret from file/string, and make up http_t structure
 */
int http_interpret(const char *filename, http_t *http_packet);

/** http_header_conf_check.c: 
 * - check whether the syntax/grammar is conformable with HTTP/1.1 or not.
 * - autocorrect/strip the useless or error part
 */
// create http_header_status_obj
http_header_status_t *create_http_header_status(char *readbuf);
// insert & check
int insert_new_header_field_name(http_header_status_t *status, u32 idx, u32 offset);
int check_header_field_name(http_header_status_t *status, char *field_name);
int insert_new_header_field_value(http_header_status_t *status, u32 idx, u32 offset);

/** http_helper.c:
 * - encap/decap those mapping code (with prefix `get_*`/`encap_*`)
 */
char *get_http_version(http_version_map http_version);
int encap_http_version(char *version);
char *get_http_method_token(method_token_map method_token);
int encap_http_method_token(char *method);
char *get_http_status_code(status_code_map status_code);
int encap_http_status_code(int http_status_code);

/** http_debug.c:
 * - good for debugging our code
 */
void debug_http_obj(http_t *http_obj);

#endif
