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
     * - `-h`: print helper function
     * - `-c`, --conc: Specify number of concurrent connections.
     * - `-n`, --conn: Specify number of total connections. 
     *      (So there will need to execute `conn/conc` times to finish all connections)
     * - `-f`, --file: Specify input file with HTTP request header template (use to setup those HTTP connections)
     */
    struct option options[]={
        {"conc", required_argument, NULL, 'c'},
        {"conn", required_argument, NULL, 'n'},
        {"file", required_argument, NULL, 'f'},
        {0, 0, 0, 0}
    };

    /* user parameters:
     * - using flags to determine using default or user specified value:
     *      0x01: conc
     *      0x02: conn
     *      0x04: file
     */
    u8      flags=0;
    u32     conc=0;
    u32     conn=0;
    char    *filename=0;

    while(1){
        int this_option_optind=optind?optind:1;
        int option_index=0;

        c=getopt_long(argc, argv, "hc:n:f:", options, &option_index);
        if(c==-1){ break; }

        switch(c){
            case 0:
                /* only has long option goes here */
                break;
            case 'c':
                conc=atoi(optarg);
                flags|=0x01;
                break;
            case 'n':
                conn=atoi(optarg);
                flags|=0x02;
                break;
            case 'f':
                filename=optarg;
                flags|=0x04;
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
        filename=NULL;
    }

    printf("==========================================================\n");
    printf("%-50s: %d\n", "Number of concurrent connections", conc);
    printf("%-50s: %d\n", "Number of total connections", conn);
    printf("%-50s: %s\n", "Using HTTP header template", filename==NULL?"None":filename);
    printf("==========================================================\n");

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
    printf("*********************************************************\n");
}