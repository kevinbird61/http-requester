#include "argparse.h"

/* option we support:
 * - `-h`:              Print helper function
 * - `-c`, --conc:      Specify number of concurrent connections.
 * - `-n`, --conn:      Specify number of total connections. 
 *                      (So there will need to execute `conn/conc` times to finish all connections)
 * - `-f`, --file:      Specify input file with HTTP request header template (use to setup those HTTP connections)
 * - `-u`, --url:       Specify URL (if --file & --url both exist, url will override the duplicated part in template file)
 * - `-p`, --port:      Specify target port number
 * - `-m`, --method:    Specify method token
 * - `--pipe`           Enable pipelining
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
                http_req_obj_ins_header_by_idx(&((*this)->http_req), option_index-(NUM_PARAMS-2), optarg);
                break;
            case 'c':   // concurrent connections
                (*this)->conc=atoi(optarg);
                (*this)->flags|=SPE_CONC;
                break;
            case 'n':   // total connections
                (*this)->conn=atoi(optarg);
                (*this)->flags|=SPE_CONN;
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
            case 'p':   // port
                (*this)->port=atoi(optarg);
                (*this)->flags|=SPE_PORT;
                break;
            case 'm':   // method
                (*this)->method=optarg;
                (*this)->flags|=SPE_METHOD;
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
            fprintf(stderr, "You need to specify `template file` or `url`.\n");
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

    printf("================================================================================\n");
    printf("%-50s: %d\n", "Number of concurrent connections", (*this)->conc);
    printf("%-50s: %d\n", "Number of total connections", (*this)->conn);
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
    return USE_URL;
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