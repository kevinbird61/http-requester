#include <getopt.h>
#include "types.h"
#include "conn.h"
#include "http.h"

#define AGENT   "http-requester-c"

// parse url, and use the result to fill host & path
int parse_url(char *url, char **host, char **path);
// print usage
void print_manual();

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
    struct option options[]={
        {"url", required_argument, NULL, 'u'},
        {"port", required_argument, NULL, 'p'},
        {"conc", required_argument, NULL, 'c'},
        {"conn", required_argument, NULL, 'n'},
        {"file", required_argument, NULL, 'f'},
        {"method", required_argument, NULL, 'm'},
        {0, 0, 0, 0}
    };

    /* user parameters:
     * - using flags to determine using default or user specified value:
     *      0x01: conc
     *      0x02: conn
     *      0x04: file (if this bit is set, then disable url and method)
     *      0x08: url
     *      0x10: port
     *      0x20: method
     */
    u8      flags=0;
    u16     port=80;
    u32     conc=0;
    u32     conn=0;
    char    *filename=NULL;
    char    *url=NULL;
    char    *method=NULL;

    while(1){
        int this_option_optind=optind?optind:1;
        int option_index=0;

        c=getopt_long(argc, argv, "hu:p:m:c:n:f:", options, &option_index);
        if(c==-1){ break; }

        switch(c){
            case 0:
                /* only has long option goes here (other) */
                break;
            case 'c':   // concurrent connections
                conc=atoi(optarg);
                flags|=0x01;
                break;
            case 'n':   // total connections
                conn=atoi(optarg);
                flags|=0x02;
                break;
            case 'f':   // using template file
                filename=optarg;
                flags|=0x04;
                break;
            case 'u':   // url
                url=optarg;
                flags|=0x08;
                break;
            case 'p':   // port
                port=atoi(optarg);
                flags|=0x10;
                break;
            case 'm':   // method
                method=optarg;
                flags|=0x20;
                break;
            case 'h':
            default:
                print_manual();
                exit(1);
        }
    }

    // print the system parameters 
    if(!(flags&0x01)){
        conc=1;
    } 
    if(!(flags&0x02)){
        conn=2;
    }
    if(!(flags&0x04)){
        // use url & method
        filename=NULL;
        // check url
        if(!(flags&0x08)){
            // if not specified URL, then print help info and exit
            fprintf(stderr, "You need to specify `template file` or `url`.\n");
            print_manual();
            exit(1);
        }
        // if not specify port, then using 80 as default
    } else {
        // use template
    }
    if(!(flags)&0x10){
        // port check 
        if((port)>65535 || (port)<0){
            fprintf(stderr, "Invalid port-number %d (Valid range: 0~65535)\n", port);
            exit(1);
        }
    }
    if(!(flags&0x20)){
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
                break;
            case ERR_USE_SSL_PORT:
                port=443;
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
        http_req_ins_header(&http_request, "Host", host);
        http_req_ins_header(&http_request, "Connection", "keep-alive");
        http_req_ins_header(&http_request, "User-Agent", AGENT);
        // finish
        http_req_finish(&http_request);
        printf("HTTP request:\n%s\n", http_request);
        
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
                            break;
                        case ERR_USE_SSL_PORT:
                            port=443;
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
                    // construct new http request
                    http_req_create_start_line(&http_request, method, path, HTTP_1_1);
                    http_req_ins_header(&http_request, "Host", host);
                    http_req_ins_header(&http_request, "Connection", "keep-alive");
                    http_req_ins_header(&http_request, "User-Agent", AGENT);
                    http_req_finish(&http_request);
                    printf("New HTTP request:\n%s\n", http_request);
                    conn++; // refuel
                    // exit(1);
                default:
                    break;
            }
        }
    }

    return 0;
}

int parse_url(char *url, char **host, char **path)
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

void print_manual()
{
    printf("*********************************************************\n");
    printf("A HTTP/1.1 requester which conform with RFC7230.\n");
    printf("\n");
    printf("Usage: [sudo] ./http_requester.exe\n");
    printf("\t-h: Print this helper function.\n");
    printf("\t-c, --conc NUM: Specify number of concurrent connections.\n");
    printf("\t-n, --conn NUM: Specify number of total connections.\n");
    printf("\t-f, --file FILE: Specify input file with HTTP request header template (use to setup those HTTP connections)\n");
    printf("\t-u, --url URL: Specify URL (if --file & --url both exist, url will override the duplicated part in template file)\n");
    printf("\t-p, --port PORT: Specify target port number\n");
    printf("\t-m, --method METHOD: Specify method token\n");
    printf("*********************************************************\n");
}