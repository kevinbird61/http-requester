#include <getopt.h>
#include "types.h"
#include "conn.h"
#include "http.h"

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

    /* start our program here */

    return 0;
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