#include "argparse.h"

u8      g_verbose=0; // default is false
int     g_total_thrd_num=1000; // default is 1000
char    *g_program=NULL;
int     *g_thrd_id_mapping=NULL;

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
 * - [x] `-v, --verbose`    Enable verbose printing
 * - [x] `--log`            Enable logging with level support
 * - [x] `-b, --burst`      Control the num_gap (between send & recv), e.g pipelining size
 * - [x] `--fast`           Reduce the send() syscall
 * - [x] `-v, --verbose`    Output more debug information
 * ...
 */
// number need to be changed if you want to add new options
struct option options[NUM_PARAMS]={ 
        [0]={"url", required_argument, NULL, 'u'},
        [1]={"port", required_argument, NULL, 'p'},
        [2]={"conn", required_argument, NULL, 'c'}, 
        [3]={"num", required_argument, NULL, 'n'}, 
        [4]={"file", required_argument, NULL, 'f'},
        [5]={"method", required_argument, NULL, 'm'},
        [6]={"pipe", no_argument, NULL, 'i'},
        [7]={"log", required_argument, NULL, 'l'},
        [8]={"burst", required_argument, NULL, 'b'},
        [9]={"fast", no_argument, NULL, 'a'}, /* use 'a' here. */
        [10]={"verbose", no_argument, NULL, 'v'},
        [11]={"thread", required_argument, NULL, 't'},
        [12]={"nonblk", no_argument, NULL, 'N'}, /* using non-blocking */
        [13]={"probe", no_argument, NULL, 'P'}, /* probe mode */
        [14]={"custom", required_argument, NULL, 'H'}, /* customized header */
        /* request headers (using itoa(REQ_*) as option name) */    
        [15]={0, 0, 0, 0}
};

parsed_args_t *
create_argparse()
{
    parsed_args_t *new_argparse=calloc(1, sizeof(parsed_args_t));
    if(new_argparse==NULL){
        perror("Cannot allocate memory for thread information.");
        exit(1);
    }
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
    int c, help=0;
    int digit_optind=0;
    g_log_visible=0;
    (*this)->use_non_block=0; // default is blocking mode

    // get prog name
    g_program=argv[0];

    while(1){
        int this_option_optind=optind? optind: 1;
        int option_index=0;
        char *field_name, *field_value;
        c=getopt_long(argc, argv, ":iavhNPl:b:u:p:m:c:n:f:t:H:", options, &option_index);
        if(c==-1){ break; }

        switch(c){
            case 'H':{ /* customized header */
                int http_req_enum;
                char *field_name=calloc(256, sizeof(char)), *field_value=calloc(256, sizeof(char));
                if(field_name==NULL || field_value==NULL){
                    perror("Memory allocate error.");
                    exit(1);
                }
                sscanf(optarg, "%[^:]:%[^:]", field_name, field_value);
                // printf("Field name: %s, value: %s\n", field_name, field_value);
                if( (http_req_enum=get_req_header_name_enum_by_str(field_name)) > 0 ){
                    http_req_obj_ins_header_by_idx(&((*this)->http_req), http_req_enum, field_value);
                } else {
                    // other customized header
                    int success=0;
                    for(int i=0; i < MAX_ARBIT_REQS; i++){
                        if((*this)->http_req->other_field[i]==NULL){
                            (*this)->http_req->other_field[i]=optarg;
                            success=1;
                            break;
                        }
                    }
                    if(!success){
                        printf("Not enough room for arbitrary header, discard: %s\n", optarg);
                    }
                }
                break;
            }
            case 'a':   // fast (cooperate with -b, burst length)
                (*this)->enable_pipe=1; /* using fast, also enable pipe */
                g_fast=1;
                break;
            case 'b':   // burst length
                (*this)->enable_pipe=1; /* also enable pipe */
                g_burst_length=atoi(optarg);
                g_burst_length=(g_burst_length<=0)? NUM_GAP: g_burst_length;
                break;
            case 'N':   // using non-blocking
                (*this)->use_non_block=1;
                break;
            case 'c':   // connections (socket)
                (*this)->conn=atoi(optarg);
                (*this)->conn=(*this)->conn>0? (*this)->conn: 1; // default value (when illegal)
                (*this)->have_conn=1;
                break;
            case 'n':   // total requests
                (*this)->reqs=atoi(optarg);
                (*this)->reqs=(*this)->reqs>0? (*this)->reqs: 1; // default value (when illegal)
                (*this)->have_reqs=1;
                break;
            case 't':   // thread number
                (*this)->thrd=atoi(optarg);
                (*this)->thrd=(*this)->thrd>0? (*this)->thrd: 1; // default value (set to 1 if value is 0)
                (*this)->have_thrd=1;
                break;
            case 'f':   // using template file
                (*this)->filename=optarg;
                (*this)->have_file=1;
                break;
            case 'u':   // url
                (*this)->have_url=1;
                /* multi url */
                int url_cnt=0;
                optind--;
                for( ;optind < argc && *argv[optind] != '-'; optind++){
                    // append each url
                    if((*this)->urls==NULL){
                        (*this)->urls=calloc(1, sizeof(struct urls));
                        if((*this)->urls==NULL){
                            perror("Cannot allocate memory for thread information.");
                            exit(1);
                        }
                        (*this)->urls->url=argv[optind];
                        (*this)->urls->next=NULL;
                    } else {
                        struct urls *root=(*this)->urls;
                        while(root->next!=NULL){
                            root=root->next;
                        }
                        root->next=calloc(1, sizeof(struct urls));
                        if(root->next==NULL){
                            perror("Cannot allocate memory for thread information.");
                            exit(1);
                        }
                        root->next->url=argv[optind];
                        root->next->next=NULL;
                    }
                }
                break;
            case 'v':   // verbose
                g_verbose=1;
                break;
            case 'p':   // port
                (*this)->port=atoi(optarg);
                (*this)->have_port=1;
                break;
            case 'P':   // probe mode
                (*this)->use_probe_mode=1;
                break;
            case 'm':   // method
                (*this)->method=optarg;
                (*this)->have_method=1;
                break;
            case 'l':
                g_log_visible=atoi(optarg); /* set log-level */
                break;
            case 'i':   // pipeline
                (*this)->enable_pipe=1;
                break;
            case '?':
                /** unknown will go to here:
                 */
                printf("Using ambiguous matching on `%s`.\n", argv[this_option_optind]);
                fprintf(stderr, "Please using `-h` to learn more about support options.\n");
                print_manual(0);
                exit(1);
            case 'h':
            default:
                help=1;
                break;
        }
    }

    if(help){
        print_manual(g_verbose);
        exit(1);
    }

    // print the system parameters 
    if(!((*this)->have_conn)){
        (*this)->conn=1;
    } 
    if(!((*this)->have_reqs)){
        (*this)->reqs=2;
    }
    if(!((*this)->have_file)){
        // use url & method
        (*this)->filename=NULL;
        // check url
        if(!((*this)->have_url)){
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
    if(!((*this)->have_port)){
        // if not specified, use port 80
        (*this)->port=DEFAULT_PORT;
    } else {
        // specified port, check it
        if( ((*this)->port>65535) || ((*this)->port<0) ){
            fprintf(stderr, "Invalid port-number %d (Valid range: 0~65535)\n", (*this)->port);
            exit(1);
        }
    }
    if(!((*this)->have_method)){
        (*this)->method="GET";
    } else {
        // TODO: check the method is legal or not
    }
    if(!((*this)->have_thrd)){ // if not set, using default value
        (*this)->thrd=1;
        g_total_thrd_num=1;
    } else {
        // check upperbound
        (*this)->thrd=((*this)->thrd>MAX_THREAD)? MAX_THREAD: (*this)->thrd;
        g_total_thrd_num=(*this)->thrd;
    }

    /* check total requests & number of connections */ 
    if((*this)->reqs < (((*this)->thrd)*((*this)->conn)) ){
        // then we assign args->conn to args->thrd*args->conc
        // each connection just need to send one requests
        (*this)->reqs=((*this)->thrd)*((*this)->conn);
    }

    int ret=0;
    struct urls *url_trav=(*this)->urls;

    // using template
    if((*this)->filename!=NULL){
        printf("Not support template file yet!\n");
        // ret=USE_TEMPLATE;
        exit(1);
    } else if((*this)->urls!=NULL){
        // check urls' number 
        int url_num=0;
        url_trav=(*this)->urls;
        while(url_trav!=NULL){
            url_num++;
            url_trav=url_trav->next;
        }
        // need to parse those urls when needed - currently update randomly
        ret=update_url_info_rand(this);
    }

    // flags 
    switch(ret){
        case USE_TEMPLATE:
            (*this)->use_template=1;
            break;
        case USE_URL:
        default:
            (*this)->use_url=1;
            break;
    }

    if(!g_verbose){ // if non-verbose, print here
        print_config((*this));
    }

    // return code (type of service)
    return ret;
}

void 
print_config(
    parsed_args_t *this)
{
    struct urls *url_trav=(this)->urls;
    printf("================================================================================\n");
    printf("%-50s: %d\n", "Number of threads", (this)->thrd);
    printf("%-50s: %d\n", "Number of connections", (this)->conn);
    printf("%-50s: %d\n", "Number of total requests", (this)->reqs);
    printf("%-50s: %s\n", "Scheme: ", this->scheme);
    if(this->use_probe_mode){
        /* show probe options */
        printf("%-50s: %s\n", "Probe mode", "Enable");
    } else {
        printf("%-50s: %d\n", "Max-requests size", g_burst_length);
        printf("%-50s: %s\n", "HTTP Pipeline", (this->enable_pipe==1)? "Enable": "-");
        printf("%-50s: %s\n", "Aggressive Pipe", (g_fast==1)? "Enable": "-");
        printf("%-50s: %s\n", "Non-Blocking Mode", (this->use_non_block==1)? "Enable": "-");
        printf("%-50s: %s\n", "Verbose Mode", (g_verbose==1)? "Enable": "-");
        printf("%-50s: %s\n", "Logging Level", g_log_level_str[g_log_visible]);
        printf("%-50s: %s\n", "URI: ", this->path);
    }
    if(this->use_url){ // only enable in non-probe mode
        printf("%-50s:\n", "Available URL(s): ");
        url_trav=this->urls;
        while(url_trav!=NULL){
            printf(" -> %s\n", url_trav->url==NULL? "None": (char*)url_trav->url);
            url_trav=url_trav->next;
        }
        printf("%-50s: %s\n", "Target URL: ", this->url==NULL? "None": (char*)this->url);
    } else {
        printf("%-50s: %s\n", "Using HTTP header template", this->filename==NULL? "None": (char*)this->filename);
    }
    printf("%-50s: %d\n", "Port number: ", this->port);
    printf("%-50s: %s\n", "Method: ", this->method);
    printf("================================================================================\n");
}

// parse user's URL
int 
parse_url(
    parsed_args_t *this)
{
    /** FIXME: do not using strtok() */

    char *url=(this)->url;
    char *ori_url, *host_delim="/", *port_delim=":", *port_str=NULL;
    int ret=ERR_NONE, ori_len=0;

    if(!strncasecmp((this)->url, "http:", 5)){
        // start from "http://<URL>"
        (this)->url=strndup((this)->url+7, strlen((this)->url)-7); // "http://" = 7 bytes   
        (this)->scheme="http";
    } else if(!strncasecmp(url, "https:", 6)){
        (this)->url=strndup((this)->url+8, strlen((this)->url)-8); // "https://" = 8 bytes 
        (this)->scheme="https";
        // need to change port!, using return value to do the trick
        ret=ERR_USE_SSL_PORT;
    } else {
        // treat as http
        (this)->scheme="http";
    }

    // perform same parsing 
    ori_len=strlen((this)->url);
    ori_url=strdup((this)->url);
    // get host
    (this)->host=strtok((this)->url, host_delim);
    // get port (if available)
    port_str=strtok((this)->host, port_delim); // now port_str=host
    port_str=strtok(NULL, port_delim); // if [:PORT] is existed, then now port_str is the port value
    if(port_str!=NULL){ // [:PORT] is existed
        u16 orig_port=(this)->port;
        (this)->port=atoi(port_str);
        if((this)->port<=0 || (this)->port>65535){ // check this port value, if invalid, then we can't use this value, rollback to origin
            (this)->port==orig_port; // use orig value 
        }
    }

    // check host length limitation, refs: https://en.wikipedia.org/wiki/Hostname#Restrictions_on_valid_hostnames
    if(strlen((this)->host)>=253){
        printf("Invalid Host length: %ld\nRefs: https://en.wikipedia.org/wiki/Hostname#Restrictions_on_valid_hostnames\n", strlen((this)->host));
        return ERR_INVALID_HOST_LEN;
    }
    // get path 
    if(port_str!=NULL){
        (this)->path=strndup(ori_url+strlen((this)->host)+1+strlen(port_str), ori_len-(strlen((this)->host)+1+strlen(port_str))); // ":" occupy 1 byte
        ret=ERR_USE_OTHER_PORT;
    } else {
        (this)->path=strndup(ori_url+strlen((this)->host), ori_len-(strlen((this)->host)));
    }
    // when user forget to add / at the end of URL
    if(strlen((this)->path)==0){
        (this)->path="/";
    }

    free(ori_url);

    return ret;
}

// randomly pick a url and then update
int 
update_url_info_rand(
    parsed_args_t **this)
{
    // randomly pick an URL to update relative information
    int num_url=0, picked_id=0, ret=0;
    struct urls *url_trav=(*this)->urls;
    while(url_trav!=NULL){
        num_url++;
        url_trav=url_trav->next;
    }
    url_trav=(*this)->urls;
    srand(time(NULL));
    picked_id=(rand()%num_url);
    //printf("Picked id: %d\n", picked_id);
    while(picked_id--){
        url_trav=url_trav->next;
    }
    (*this)->url=url_trav->url;

    // only one url, use url only
    ret=parse_url(*this);
    switch(ret){
        case ERR_NONE:
            if(!((*this)->have_port)){ // user hasn't specifed port-num
                (*this)->port=DEFAULT_PORT;
            }
            break;
        case ERR_USE_SSL_PORT:
            if(!((*this)->have_port)){ // user hasn't specifed port-num
                (*this)->port=DEFAULT_SSL_PORT;
            }
            break;
        case ERR_USE_OTHER_PORT: // use [:PORT]
            break;
        case ERR_INVALID_HOST_LEN:
        default:
            exit(1);
    }

    // free(url_trav); // we might need this urls later (picking another url)
    return USE_URL;
}

// print out usage/helper page
void 
print_manual(
    u8 detail)
{
    printf("********************************************************************************\n");
#ifdef VER_MAJ 
    printf("Version %d.%d.%d\n", VER_MAJ, VER_MIN, VER_PAT);
#endif
    printf("Welcome to use KB (Kevin Benchmark tool) !\n");
    printf("A HTTP/1.1 client which conform with RFC7230.\n");
    printf("Author: Kevin Cyu (scyu@a10networks.com).\n");
#ifdef RELEASE
    printf("%s\n", a10logo);
#endif
    printf("\n");
    printf("Usage: [sudo] %s\n", g_program);
    // printf("\t-h: Print this helper function.\n");
    printf("\t-%-2c    %-7s %-7s: %s.\n", 'h', "", "", "Print this helper function");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 't', "thread", "NUM", "Specify number of threads, total requests will distribute to each thread");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'c', "conn", "NUM", "Specify number of connections (per thread)");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'n', "num", "NUM", "Specify number of requests, distribute to each connection");
    // printf("\t-%-2c, --%-7s %-7s: %s.\n", 'f', "file", "FILE", "Specify input file with HTTP request header template (use to setup those HTTP connections)");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'u', "url", "URL", "Specify URL (i.e. `http://kevin.a10networks.com:8080/index.html`)");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'p', "port", "PORT", "Specify target port number (priority is lower than URL's port)");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'm', "method", "METHOD", "Specify method token");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'l', "log", "LEVEL", "Enable logging (1~99)"); 
        printf("\t  %s= %s\n", "1", "Show uncategorized only"); 
        printf("\t  %s= %s\n", "2", "Show error handler only"); 
        printf("\t  %s= %s\n", "3", "Show connection management only");
        printf("\t  %s= %s\n", "4", "Show parsing state machine only");
        printf("\t  %s= %s\n", "5", "Show http response parser only");
        printf("\t  %s= %s\n", "99", "Show & log all");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'v', "verbose", " ", "Enable verbose printing (using `-h -v` to print more helper info)");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'i', "pipe", " ", "Enable HTTP pipelining");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'b', "burst", "LENGTH", "Configure burst length for HTTP pipelining (default is 500), enable pipe too");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'a', "fast", " ", "Enable aggressive HTTP pipelining (default is false), enable pipe too");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'N', "nonblk", " ", "Enable non-blocking connect, send and recv");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'P', "probe", " ", "Using probe mode (different with default mode)");
    printf("\t-%-2c, --%-7s %-7s: %s.\n", 'H', "custom", "HDR:VAL", "Add request header's field name and field value (Support headers: using `-h -v` to find out)");

    if(detail){
        printf("[Support request header]--------------------------------------------------------\n");
        for(int i=1; i<REQ_HEADER_NAME_MAXIMUM; i++){
            printf("%s | ", get_req_header_name_by_idx[i]);
        }
        printf("\n");
        printf("[Example]-----------------------------------------------------------------------\n");
        printf("%s -u http://httpd.apache.org -n 1000 -c 10 -t 1 -N\n", g_program);
        printf("  : using 1 thread and open 10 connections to deliver 1000 requests with non-blocking mode.\n");
    }
    printf("********************************************************************************\n");
}
