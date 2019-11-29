#include <getopt.h>
#include "types.h"
#include "conn.h"
#include "http.h"

#define NUM_PARAMS          (7)
#define AGENT               "http-requester-c"
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

// parse url, and use the result to fill host & path
int parse_url(char *url, char **host, char **path);
// print usage
void print_manual(u8 detail);

int main(int argc, char *argv[])
{
    int c;
    int digit_optind=0;
    
    /* option we support:
     * - `-h`:              Print helper function
     * - `-c`, --conc:      Specify number of concurrent connections.
     * - `-n`, --conn:      Specify number of total connections. 
     *                      (So there will need to execute `conn/conc` times to finish all connections)
     * - `-f`, --file:      Specify input file with HTTP request header template (use to setup those HTTP connections)
     * - `-u`, --url:       Specify URL (if --file & --url both exist, url will override the duplicated part in template file)
     * - `-p`, --port:      Specify target port number
     * - `-m`, --method:    Specify method token
     */
    struct option options[NUM_PARAMS+REQ_HEADER_NAME_MAXIMUM]={ // number need to be changed if you want to add new options
        [0]={"url", required_argument, NULL, 'u'},
        [1]={"port", required_argument, NULL, 'p'},
        [2]={"conc", required_argument, NULL, 'c'},
        [3]={"conn", required_argument, NULL, 'n'},
        [4]={"file", required_argument, NULL, 'f'},
        [5]={"method", required_argument, NULL, 'm'},
        /* request headers (using itoa(REQ_*) as option name) */    
        [NUM_PARAMS+REQ_HEADER_NAME_MAXIMUM-1]={0, 0, 0, 0}
    };
    /* add request headers */
    for(int i=1; i<(REQ_HEADER_NAME_MAXIMUM); i++){
        // forbidden
        if( (i!=REQ_HOST) ){
            // printf("%d: %s\n", (NUM_PARAMS-2)+i, get_req_header_name_by_idx[i]);
            char *name=strdup(get_req_header_name_by_idx[i]);
            to_lowercase(name);
            options[(NUM_PARAMS-2)+i].name=name;
            options[(NUM_PARAMS-2)+i].has_arg=required_argument;
            options[(NUM_PARAMS-2)+i].flag=NULL;
            options[(NUM_PARAMS-2)+i].val=0;
        }
    }

    u8      flags=0; // enum, represent user specified parameters.
    u32     port=DEFAULT_PORT;
    u32     conc=0;
    u32     conn=0;
    char    *filename=NULL;
    char    *url=NULL;
    char    *method=NULL;
    // alloc
    http_req_header_status_t *http_req;
    http_req_obj_create(&http_req);

    while(1){
        int this_option_optind=optind?optind:1;
        int option_index=0;
        char *field_name, *field_value;
        c=getopt_long(argc, argv, ":hu:p:m:c:n:f:", options, &option_index);
        if(c==-1){ break; }

        switch(c){
            case 0:
                /* http request headers:
                 *  atoi(options[option_index].name) = field-name enum
                 *  optarg = field-value
                 */
                // puts(options[option_index].name);
                // puts(optarg);
                http_req_obj_ins_header_by_idx(&http_req, option_index-(NUM_PARAMS-2), optarg);
                break;
            case 'c':   // concurrent connections
                conc=atoi(optarg);
                flags|=SPE_CONC;
                break;
            case 'n':   // total connections
                conn=atoi(optarg);
                flags|=SPE_CONN;
                break;
            case 'f':   // using template file
                filename=optarg;
                flags|=SPE_FILE;
                break;
            case 'u':   // url
                url=optarg;
                flags|=SPE_URL;
                break;
            case 'p':   // port
                port=atoi(optarg);
                flags|=SPE_PORT;
                break;
            case 'm':   // method
                method=optarg;
                flags|=SPE_METHOD;
                break;
            case '?':
                /** unknown will go to here:
                 *  - argv[this_option_optind]+2 : field-name, need to skip `--` (+2)
                 *  - argv[this_option_optind+1] : field-value
                 */
                printf("Using ambiguous matching on `%s`.\n", argv[this_option_optind]+2);
                
                // printf("%d, %d, %d\n", get_req_header_name_enum_by_str(argv[this_option_optind]+2), argv[this_option_optind+1]!=NULL, get_req_header_name_enum_by_str(argv[this_option_optind+1]));
                if(get_req_header_name_enum_by_str(argv[this_option_optind]+2)>0 
                    && argv[this_option_optind+1]!=NULL){ 
                    // also checking its value is valid & not another req header 
                    if(get_req_header_name_enum_by_str(argv[this_option_optind+1]+2)==0){ 
                        // puts(argv[this_option_optind]);
                        // puts(argv[this_option_optind+1]);
                        http_req_obj_ins_header_by_idx(&http_req, 
                            get_req_header_name_enum_by_str(argv[this_option_optind]+2), 
                            argv[this_option_optind+1]);
                        break;
                    } else {
                        // print debug message
                        fprintf(stderr, "You need to specify a value for header `%s`\n", argv[this_option_optind]);
                    }
                } else {
                    // unknown options
                    if(argv[this_option_optind+1]==NULL){
                        fprintf(stderr, "You need to specify a value for header `%s`\n", argv[this_option_optind]);
                    } else {
                        fprintf(stderr, "[ERROR] Unknown option '%s'.\n", argv[this_option_optind]);
                    }
                }
                fprintf(stderr, "Please using `-h` to learn more about support options.\n");
                print_manual(0);
                exit(1);
            case 'h':
            default:
                print_manual(1);
                exit(1);
        }
    }

    // print the system parameters 
    if(!(flags&SPE_CONC)){
        conc=1;
    } 
    if(!(flags&SPE_CONN)){
        conn=2;
    }
    if(!(flags&SPE_FILE)){
        // use url & method
        filename=NULL;
        // check url
        if(!(flags&SPE_URL)){
            // if not specified URL, then print help info and exit
            fprintf(stderr, "You need to specify `template file` or `url`.\n");
            print_manual(0);
            exit(1);
        }
        // if not specify port, then using 80 as default
    } else {
        // use template
        // also need to check URL
    }
    if(!(flags&SPE_PORT)){
        // if not specified, use port 80
        port=DEFAULT_PORT;
    } else {
        // specified port, check it
        if( (port>65535) || (port<0) ){
            fprintf(stderr, "Invalid port-number %d (Valid range: 0~65535)\n", port);
            exit(1);
        }
    }
    if(!(flags&SPE_METHOD)){
        method="GET";
    } else {
        // TODO: check the method is legal or not
    }

    printf("================================================================================\n");
    printf("%-50s: %d\n", "Number of concurrent connections", conc);
    printf("%-50s: %d\n", "Number of total connections", conn);
    printf("%-50s: %s\n", "Using HTTP header template", filename==NULL?"None":filename);
    printf("%-50s: %s\n", "Target URL: ", url==NULL?"None":url);
    printf("%-50s: %d\n", "Port number: ", port);
    printf("%-50s: %s\n", "Method: ", method);
    printf("================================================================================\n");

    /** step by step construct our http request header from user input:
     *  - parsing url first, if template file is available, 
     *      then url will override the fields in template file.
     *      - URL format: [http:]//<HOST, maximum length not larger than 255>[:port-num]/[pathname]
     *  - forge http header
     *  - create socket and use http_state_machine to send request
     *  
     */

    // using template
    if(filename!=NULL){
        printf("Not support yet!\n");
        exit(1);
    } else if(url!=NULL){
        // use url only
        char *host, *path;
        int ret=parse_url(url, &host, &path);
        switch(ret){
            case ERR_NONE:
                if(!(flags&SPE_PORT)){ // user hasn't specifed port-num
                    port=DEFAULT_PORT;
                }
                break;
            case ERR_USE_SSL_PORT:
                if(!(flags&SPE_PORT)){ // user hasn't specifed port-num
                    port=DEFAULT_SSL_PORT;
                }
                break;
            case ERR_INVALID_HOST_LEN:
            default:
                exit(1);
        }

        /* 2. forge http request header */
        char *http_request;
        // start-line
        http_req_create_start_line(&http_request, method, path, HTTP_1_1);
        // header fields
        http_req_obj_ins_header_by_idx(&http_req, REQ_HOST, host);
        http_req_obj_ins_header_by_idx(&http_req, REQ_CONN, "keep-alive");
        http_req_obj_ins_header_by_idx(&http_req, REQ_USER_AGENT, AGENT);
        // finish
        http_req_finish(http_req, &http_request);
        printf("HTTP request:*******************************************************************\n");
        printf("%s\n", http_request);
        printf("================================================================================\n");
        
        /* send request */
        int sockfd=create_tcp_conn(host, itoa(port));
        if(sockfd<0){
            // FIXME: add syslog
            exit(1);
        }
        // http_t *http_request_obj=malloc(sizeof(http_t));
        // http_parser(http_request, http_request_obj);

        // current only support multi-connections (not concurrent)
        while(conn){
            // http_t *req;
            // memcpy(req, http_request_obj, sizeof(http_t));
            int ret=http_state_machine(sockfd, (void**)&http_request, --conn, 1);
            char *loc;
            switch(ret){
                case ERR_REDIRECT: /* handle redirection */
                    // Modified http request, and send the request again
                    loc=(char*)http_request;
                    // need to parse again 
                    ret=parse_url(loc, &host, &path);
                    switch(ret){
                        case ERR_NONE:
                            if(!(flags&SPE_PORT)){ // user hasn't specifed port-num
                                port=DEFAULT_PORT;
                            }
                            break;
                        case ERR_USE_SSL_PORT:
                            if(!(flags&SPE_PORT)){ // user hasn't specifed port-num
                                puts("***********************Using default SSL Port***********************");
                                port=DEFAULT_SSL_PORT;
                            }
                            break;
                        case ERR_INVALID_HOST_LEN:
                        default:
                            exit(1);
                    }
                    // create new conn
                    sockfd=create_tcp_conn(host, itoa(port));
                    if(sockfd<0){
                        exit(1);
                    }
                    // change attributes in http_req
                    http_req_obj_ins_header_by_idx(&http_req, REQ_HOST, host);
                    // construct new http request from modified http_req
                    http_req_create_start_line(&http_request, method, path, HTTP_1_1);
                    http_req_finish(http_req, &http_request);
                    printf("================================================================================\n");
                    printf("%-50s: %s\n", "Target URL: ", url==NULL?"None":url);
                    printf("%-50s: %d\n", "Port number: ", port);
                    printf("%-50s: %s\n", "Method: ", method);
                    printf("********************************************************************************\n");
                    printf("New HTTP request:***************************************************************\n");
                    printf("%s\n", http_request);
                    printf("================================================================================\n");
                    conn++; // refuel
                    // exit(1);
                default:
                    break;
            }
        }
    }

    return 0;
}

// parse user's URL
int 
parse_url(
    char *url, 
    char **host, 
    char **path)
{
    /** 1. check URL's protocol 
     *      - http
     */
    int ret=ERR_NONE;
    if(!strncasecmp(url, "http:", 5)){
        // start from "http://<URL>"
        url=strndup(url+7, strlen(url)-7); // "http://" = 7 bytes   
    } else if(!strncasecmp(url, "https:", 6)){
        url=strndup(url+8, strlen(url)-8); // "https://" = 8 bytes 
        // need to change port!, using return value to do the trick
        ret=ERR_USE_SSL_PORT;
    }
    // Now URL = <HOST>/<Pathname>
    /** FIXME: Do we need to perform parsing [:PORT] 
     *      between processing <Host> and <Path> ?
     */
    // perform same parsing 
    int ori_len=strlen(url);
    char *ori_url, *host_delim="/";
    ori_url=strdup(url);
    // get host
    *host=strtok(url, host_delim);
    // printf("Host: %s\n", host);
    // check host length limitation, refs: https://en.wikipedia.org/wiki/Hostname#Restrictions_on_valid_hostnames
    if(strlen(*host)>=253){
        printf("Invalid Host length: %ld\nRefs: https://en.wikipedia.org/wiki/Hostname#Restrictions_on_valid_hostnames\n", strlen(*host));
        // exit(1);
        return ERR_INVALID_HOST_LEN;
    }
    // get path
    *path=strndup(ori_url+strlen(*host), ori_len-strlen(*host));
    // when user forget to add / at the end of URL
    if(strlen(*path)==0){
        *path="/";
    }
    return ret;
}

// print out usage/helper page
void 
print_manual(
    u8 detail)
{
    printf("********************************************************************************\n");
    printf("A HTTP/1.1 requester which conform with RFC7230.\n");
    printf("\n");
    printf("Usage: [sudo] ./http_requester.exe\n");
    printf("\t-h: Print this helper function.\n");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'c', "conc", "NUM", "Specify number of concurrent connections");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'n', "conn", "NUM", "Specify number of total connections");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'f', "file", "FILE", "Specify input file with HTTP request header template (use to setup those HTTP connections)");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'u', "url", "URL", "Specify URL (if --file & --url both exist, url will override the duplicated part in template file)");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'p', "port", "PORT", "Specify target port number");
    printf("\t-%-2c, --%-7s %-7s: Specify method token.\n", 'm', "method", "METHOD");
    if(detail){
        printf("[Customized Request Header]-----------------------------------------------------\n");
        for(int i=1;i<REQ_HEADER_NAME_MAXIMUM;i++){
            if( (i!=REQ_HOST) ){
                char *name=strdup(get_req_header_name_by_idx[i]);
                to_lowercase(name);
                printf("--%-20s <VALUE> : Request header field-value of `%s`\n", name, get_req_header_name_by_idx[i]);
            }
        }
    }
    printf("********************************************************************************\n");
}