#include <getopt.h>
#include "conn.h"
#include "http.h"

int main(int argc, char *argv[])
{
    int c;
    int digit_optind=0;

    // option we support
    struct option options[]={
        {"concurrent", required_argument, NULL, 'c'},
        {"connection", required_argument, NULL, 'i'}
    }

    /* argparse */

    return 0;
}
