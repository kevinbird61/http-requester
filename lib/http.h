#ifndef __HTTP__
#define __HTTP__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "conn.h"
#include "types.h"

// helper function
char *get_http_version(http_version_map http_version);
int encap_http_version(char *version);
char *get_http_method_token(method_token_map method_token);
int encap_http_method_token(char *method);
char *get_http_status_code(status_code_map status_code);
int encap_http_status_code(int http_status_code);

// send out the request, and store the return data on which rawdata points to
int http_request(int sockfd, http_t *http_request, char **rawdata);

// parsing the rawdata, and fill them into data structure (response)
int http_parser(const char *rawdata, http_t *http_packet);

// http processing 
int http_process(http_t *http_packet);

#endif
