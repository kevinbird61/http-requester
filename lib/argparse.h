#ifndef __ARGPARSE__
#define __ARGPARSE__

#include <getopt.h>
#include "http.h"

#define NUM_PARAMS          (7)
#define DEFAULT_PORT        (80)
#define DEFAULT_SSL_PORT    (443)

/* user parameters:
 * - using flags to determine using default or user specified value:
 *      0x01: conc
 *      0x02: conn
 *      0x04: file (if this bit is set, then disable url and method)
 *      0x08: url
 *      0x10: port
 *      0x20: method
 */
enum {
    SPE_CONC=0x01,
    SPE_CONN=0x02,
    SPE_FILE=0x04,
    SPE_URL=0x08,
    SPE_PORT=0x10,
    SPE_METHOD=0x20
};

enum {
    USE_URL=1,
    USE_TEMPLATE
};

/* user input */
typedef struct _parsed_args_t {
    u8                          flags;
    u32                         port;
    u32                         conc;
    u32                         conn;
    char*                       filename;
    char*                       url;
    char*                       method;
    char*                       host;
    char*                       path;
    http_req_header_status_t *  http_req;
} parsed_args_t;

extern struct option options[NUM_PARAMS+REQ_HEADER_NAME_MAXIMUM];

// create argparse object
parsed_args_t *create_argparse();
// parse from argc, argv
int argparse(parsed_args_t **this, int argc, char **argv);
// parse url, and use the result to fill host & path
int parse_url(char *url, char **host, char **path);
// print usage
void print_manual(u8 detail);

#endif