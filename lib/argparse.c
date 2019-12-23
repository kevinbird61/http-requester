#include "argparse.h"

u8 verbose=0; // default is false
int total_thrd_num=1000; // default is 1000
int max_req_size=NUM_GAP;
char *program=NULL;
/* option we support:
 * - [x] `-h`:              Print helper function
 * - [x] `-t`, --thread:    Specify number of threads
 * - [x] `-c`, --conc:      Specify number of connections.
 * - [x] `-n`, --conn:      Specify number of requests. 
 *                      (So there will need to execute `conn/conc` times to finish all connections)
 * - [x] `-f`, --file:      Specify input file with HTTP request header template (use to setup those HTTP connections)
 * - [x] `-u`, --url:       Specify URL (if --file & --url both exist, url will override the duplicated part in template file)
 * - [x] `-p`, --port:      Specify target port number
 * - [x] `-m`, --method:    Specify method token
 * - [x] `--pipe`           Enable pipelining
 * - [x] `--log`            Enable logging with level support
 * - [x] `-b, --burst`      Control the num_gap (between send & recv), e.g pipelining size
 * - [x] `--fast`           Reduce the send() syscall
 * - [x] `-v, --verbose`    Output more debug information
 * ...
 * 
 * (Other: HTTP request headers)
 */
// number need to be changed if you want to add new options
struct option options[NUM_PARAMS+REQ_HEADER_NAME_MAXIMUM]={ 
        [0]={"url", required_argument, NULL, 'u'},
        [1]={"port", required_argument, NULL, 'p'},
        [2]={"conc", required_argument, NULL, 'c'},
        [3]={"conn", required_argument, NULL, 'n'},
        [4]={"file", required_argument, NULL, 'f'},
        [5]={"method", required_argument, NULL, 'm'},
        [6]={"pipe", no_argument, NULL, 'i'},
        [7]={"log", required_argument, NULL, 'l'},
        [8]={"burst", required_argument, NULL, 'b'},
        [9]={"fast", no_argument, NULL, 'a'}, /* use 'a' here. */
        [10]={"verbose", no_argument, NULL, 'v'},
        [11]={"thread", required_argument, NULL, 't'}, 
        [12]={"nonblk", no_argument, NULL, 'N'}, /* using non-blocking */
        /* request headers (using itoa(REQ_*) as option name) */    
        [NUM_PARAMS+REQ_HEADER_NAME_MAXIMUM-1]={0, 0, 0, 0}
};

parsed_args_t *
create_argparse()
{
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
            free(name);
        }
    }

    parsed_args_t *new_argparse=calloc(1, sizeof(parsed_args_t));
    new_argparse->port=DEFAULT_PORT;
    new_argparse->urls=NULL;
    // init http_req_obj
    http_req_obj_create(&new_argparse->http_req);

    return new_argparse;
}

// parse from argc, argv
int 
argparse(
    parsed_args_t **this, 
    int argc, 
    char **argv)
{
    int c;
    int digit_optind=0;
    log_visible=0;
    (*this)->use_non_block=0;

    // get prog name
    program=argv[0];

    while(1){
        int this_option_optind=optind?optind:1;
        int option_index=0;
        char *field_name, *field_value;
        c=getopt_long(argc, argv, ":iavhNl:b:u:p:m:c:n:f:t:", options, &option_index);
        if(c==-1){ break; }

        switch(c){
            case 0:
                /* http request headers:
                 *  atoi(options[option_index].name) = field-name enum
                 *  optarg = field-value
                 */
                // puts(options[option_index].name);
                // puts(optarg);
                http_req_obj_ins_header_by_idx(&((*this)->http_req), option_index-(NUM_PARAMS-2), optarg);
                break;
            case 'a':   // fast (cooperate with -b, burst length)
                printf("Enable fast transmit (for http pipelining).\n");
                (*this)->enable_pipe=1; /* using fast, also enable pipe */
                fast=1;
                break;
            case 'b':   // burst length
                printf("Enable HTTP pipelining.\n");
                (*this)->enable_pipe=1; /* also enable pipe */
                burst_length=atoi(optarg);
                burst_length=(burst_length<=0)?NUM_GAP:burst_length;
                max_req_size=burst_length;
                printf("Configure burst length = %d (for http pipelining).\n", burst_length);
                break;
            case 'N':   // using non-blocking
                (*this)->use_non_block=1;
                printf("Using non-blocking mode.\n");
                break;
            case 'c':   // connections (socket)
                (*this)->conc=atoi(optarg);
                (*this)->conc=(*this)->conc>0?(*this)->conc:1; // default value (when illegal)
                (*this)->flags|=SPE_CONC;
                break;
            case 'n':   // total connections (requests)
                (*this)->conn=atoi(optarg);
                (*this)->conn=(*this)->conn>0?(*this)->conn:1; // default value (when illegal)
                (*this)->flags|=SPE_CONN;
                break;
            case 't':   // thread number
                (*this)->thrd=atoi(optarg);
                (*this)->thrd=(*this)->thrd>0?(*this)->thrd:1; // default value (set to 1 if value is 0)
                (*this)->flags|=SPE_THRD;
                break;
            case 'f':   // using template file
                (*this)->filename=optarg;
                (*this)->flags|=SPE_FILE;
                break;
            case 'u':   // url
                // (*this)->url=optarg;
                (*this)->flags|=SPE_URL;
                /* multi url */
                int url_cnt=0;
                optind--;
                for( ;optind < argc && *argv[optind] != '-'; optind++){
                    // append each url
                    
                    if((*this)->urls==NULL){
                        printf("URL(%d): %s\n", url_cnt++, argv[optind]);
                        (*this)->urls=calloc(1, sizeof(struct urls));
                        (*this)->urls->url=argv[optind];
                        (*this)->urls->next=NULL;
                    } else {
                        printf("URL(%d): %s\n", url_cnt++, argv[optind]);
                        struct urls *root=(*this)->urls;
                        while(root->next!=NULL){
                            root=root->next;
                        }
                        root->next=calloc(1, sizeof(struct urls));
                        root->next->url=argv[optind];
                        root->next->next=NULL;
                    }
                }
                break;
            case 'v':   // verbose
                verbose=1;
                printf("Enable verbose mode.\n");
                break;
            case 'p':   // port
                (*this)->port=atoi(optarg);
                (*this)->flags|=SPE_PORT;
                break;
            case 'm':   // method
                (*this)->method=optarg;
                (*this)->flags|=SPE_METHOD;
                break;
            case 'l':
                printf("Enable logging, level is %s\n", optarg); 
                log_visible=atoi(optarg); /* set log-level */
                break;
            case 'i':   // pipeline
                printf("Enable HTTP pipelining.\n");
                (*this)->enable_pipe=1;
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
                        http_req_obj_ins_header_by_idx(&((*this)->http_req), 
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
    if(!((*this)->flags&SPE_CONC)){
        (*this)->conc=1;
    } 
    if(!((*this)->flags&SPE_CONN)){
        (*this)->conn=2;
    }
    if(!((*this)->flags&SPE_FILE)){
        // use url & method
        (*this)->filename=NULL;
        // check url
        if(!((*this)->flags&SPE_URL)){
            // if not specified URL, then print help info and exit
            fprintf(stderr, "You need to specify `template file` (-f) or `url` (-u).\n");
            print_manual(0);
            exit(1);
        }
        // if not specify port, then using 80 as default
    } else {
        // use template
        // also need to check URL
    }
    if(!((*this)->flags&SPE_PORT)){
        // if not specified, use port 80
        (*this)->port=DEFAULT_PORT;
    } else {
        // specified port, check it
        if( ((*this)->port>65535) || ((*this)->port<0) ){
            fprintf(stderr, "Invalid port-number %d (Valid range: 0~65535)\n", (*this)->port);
            exit(1);
        }
    }
    if(!((*this)->flags&SPE_METHOD)){
        (*this)->method="GET";
    } else {
        // TODO: check the method is legal or not
    }
    if(!((*this)->flags&SPE_THRD)){ // if not set, using default value
        (*this)->thrd=1;
        total_thrd_num=1;
    } else {
        // check upperbound
        (*this)->thrd=((*this)->thrd>MAX_THREAD)? MAX_THREAD: (*this)->thrd;
        total_thrd_num=(*this)->thrd;
    }

    printf("================================================================================\n");
    printf("%-50s: %d\n", "Number of threads", (*this)->thrd);
    printf("%-50s: %d\n", "Number of connections", (*this)->conc);
    printf("%-50s: %d\n", "Number of total requests", (*this)->conn);
    printf("%-50s: %d\n", "Max-requests size", burst_length);
    printf("%-50s: %s\n", "Using HTTP header template", (*this)->filename==NULL? "None": (char*)(*this)->filename);
    /*printf("%-50s: %s\n", "Target URL: ", (*this)->url==NULL? "None": (char*)(*this)->url);*/
    struct urls *url_trav=(*this)->urls;
    printf("%-50s:\n", "Target URL(s): ");
    while(url_trav!=NULL){
        printf(" -> %s\n", url_trav->url==NULL? "None": (char*)url_trav->url);
        url_trav=url_trav->next;
    }
    printf("%-50s: %d\n", "Port number: ", (*this)->port);
    printf("%-50s: %s\n", "Method: ", (*this)->method);
    printf("================================================================================\n");

    // using template
    if((*this)->filename!=NULL){
        printf("Not support yet!\n");
        exit(1);
        // return USE_TEMPLATE
    } else if((*this)->urls!=NULL){
        // check urls' number 
        int url_num=0;
        url_trav=(*this)->urls;
        while(url_trav!=NULL){
            url_num++;
            url_trav=url_trav->next;
        }
        /** need to parse those urls when needed 
         * - currently update randomly
         */
        return update_url_info_rand(this);
    }

    return USE_URL;
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

// randomly pick a url and then update
int 
update_url_info_rand(
    parsed_args_t **this)
{
    // randomly pick an URL to update relative information
    char *picked_url;
    int num_url=0;
    struct urls *url_trav=(*this)->urls;
    while(url_trav!=NULL){
        num_url++;
        url_trav=url_trav->next;
    }
    url_trav=(*this)->urls;
    srand(time(NULL));
    int picked_id=(rand()%num_url);
    printf("Picked id: %d\n", picked_id);
    while(picked_id--){
        url_trav=url_trav->next;
    }
    picked_url=url_trav->url;
    printf("Num of urls: %d, Randomly pick: %s\n", num_url, picked_url);
    // only one url, use url only
    int ret=parse_url(picked_url, &((*this)->host), &((*this)->path));
    switch(ret){
        case ERR_NONE:
            if(!((*this)->flags&SPE_PORT)){ // user hasn't specifed port-num
                (*this)->port=DEFAULT_PORT;
            }
            break;
        case ERR_USE_SSL_PORT:
            if(!((*this)->flags&SPE_PORT)){ // user hasn't specifed port-num
                (*this)->port=DEFAULT_SSL_PORT;
            }
            break;
        case ERR_INVALID_HOST_LEN:
        default:
            exit(1);
    }

    free(url_trav);

    return USE_URL;
}

// print out usage/helper page
void 
print_manual(
    u8 detail)
{
    printf("********************************************************************************\n");
    printf("A HTTP/1.1 requester which conform with RFC7230.\n");
    printf("Author: Kevin Cyu (scyu@a10networks.com).\n");
    // printf("%s\n", a10logo);
    printf("\n");
    printf("Usage: [sudo] %s\n", program);
    printf("\t-h: Print this helper function.\n");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 't', "thread", "NUM", "Specify number of threads, total requests will distribute to each thread");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'c', "conc", "NUM", "Specify number of connections (per thread)");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'n', "conn", "NUM", "Specify number of requests (per thread), distribute to each connection");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'f', "file", "FILE", "Specify input file with HTTP request header template (use to setup those HTTP connections)");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'u', "url", "URL", "Specify URL (if --file & --url both exist, url will override the duplicated part in template file)");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'p', "port", "PORT", "Specify target port number");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'm', "method", "METHOD", "Specify method token");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'l', "log", "LEVEL", "Enable logging (0~7)"); 
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'i', "pipe", " ", "Enable HTTP pipelining");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'b', "burst", "LENGTH", "Configure burst length for HTTP pipelining (default is 500), enable pipe too.");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'a', "fast", " ", "Packed several http requests together for HTTP pipelining (default is false), enable pipe too.");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'N', "nonblk", " ", "Enable non-blocking connect, send and recv.");

    if(detail){
        printf("[Customized Request Header]-----------------------------------------------------\n");
        for(int i=1;i<REQ_HEADER_NAME_MAXIMUM;i++){
            if( (i!=REQ_HOST) ){
                char *name=strdup(get_req_header_name_by_idx[i]);
                to_lowercase(name);
                // printf("--%-20s <VALUE> : Request header field-value of `%s`\n", name, get_req_header_name_by_idx[i]);
                printf("--%-20s <VALUE>\n", name);
            }
        }
        printf("[Example]-----------------------------------------------------------------------\n");
        printf("%s -u http://httpd.apache.org -n 1000 -c 10 -t 1 -N\n", program);
        printf("  : using 1 thread and open 10 connections to deliver 1000 requests with non-blocking mode.\n");
    }
    printf("********************************************************************************\n");
}
