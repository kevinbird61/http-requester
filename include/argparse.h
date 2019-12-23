#ifndef __ARGPARSE__
#define __ARGPARSE__

#include <getopt.h>
#include "conn_mgnt.h" /* fetch global var */
#include "logger.h"
#include "types.h"
#include "http.h"

#define NUM_PARAMS          (14)
#define MAX_THREAD          (1000)    // max thread number
#define DEFAULT_PORT        (80)
#define DEFAULT_SSL_PORT    (443)
#define __FILENAME__        (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define AGENT               "kevinbird"

static const char *a10logo=
"\
WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWNNXXXXNNWWWWWWWWWWWWWW\n\
WWWMWWWWMWWWWWMWWWWMWWWWMWWWWWMWWWWMWWWWMWWWWWMWWWWMWWWWMWWXXXXXXNNWMWWWWMWWWWWMWWWWM\n\
MMWWWMMWWWWMWWWWMMWWWMMWWWWMWWWWMMWWWMMWWWWMWWWWMMWWWMWNXXXXXNNWWMMWWWMMWWWWMWWWWMMWW\n\
WWWMWWWWMWWWWWMWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWNXXXNNWWWWWWWWWWMWWWWMMWWWWWWWWWM\n\
WWWWWWWWWWWWWWWWWNKKKKKKKKKKKKKXWWWWWWWWWWWWWWWWNXXXXNWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW\n\
MWWWWWMWWWWMWWWWWXo,,,;,,,,;;,,c0WWWWMWWWWWWWNXXXXNWWWMWWWWMMMWWWMMWWWMMWWWWMWWWWMWWW\n\
WWWMWWWWMMWWWMMWNO:''''''''''''':0WMWWWWMWNXXXNNKOkdONWWXOxddddddddddddxKWMWWWMMWWWWM\n\
MWWWWWMWWWWWWWWWk;''''''''''''''':0WWWWWNXXX0kdc;'''lXW0c'''.....'..'''';xNWWWWWWWWWW\n\
WWWWWWWWWWWWWWNk;''';oc'''''''''''c0WNNXNWW0:'''''''lXNo'''''':xxxxl''''':0WWWWWWWWNK\n\
WWWMWWWWWWWWWNx,''';OWKc'''''''''''c0NNWWWW0c,,'''''lXKc''''''dNWWWO;.''.,OWWWWWXK00K\n\
MMWWWMMWWWWMNd,''':OWMM0c'''''''''''lKMWWWWWX0l'''''lXK:.'''''oNWMMO;.''',kWNXK0KXNWW\n\
WWWMWWWWMWWXo,''':0WWWWWKc'''''''''''lKWWWWWWNo'''''lX0:.'''.'dWWWWO;.'';lkK0KNWWWWWM\n\
WWWWWWWWWWKl'''''lxxxxxxxl,'''''''''''lXWWWWWNo'''''lX0:''''.'dWWWW0c:odddONWWWWWWWWW\n\
MWWWWWMWWKc'''''''''.''''''''''''''''''oXWWWMNo''.''lXK:.'''.'dNNNXOdol:,,OMWWWWWMWWW\n\
WWWMWWWW0c'''';:::::::::::::,'''''''''''oKXXXXl'''''cK0:'',;:ckKKKXk;'''.,kWWWMMWWWWM\n\
WWWWWWWO:''''oKNNNNNNNXXXXXXk:'''''''''''oKNNKl'''''lK0ocoooood0KXNx,'''';OMWWWWWMWWW\n\
WWWWWNk;''',dNWWWWWWWNXNNNNNWO:'''''''''',oXMNo.',;cdKKkl:;,'',;:::;'''''cKWWWWWWWWWW\n\
WWWWXd,.''.:KMWWWWWWNNNWWWWWWNo'...''.'''''oKXdlodoldKWKd:,,,,,,,,,,,,,;lkKKXNWWWWWWM\n\
MMWNOddddddkXWWWMMWXXWMWWWWMMNkdddddddoollox0XXNNX000NMWNXK000000KK000KXNNNK00KNWMMWW\n\
WWWMWWWWMMWWWWMWWWWWXXNNWWWNNNNNXXXXXXXXNNNNWMMWWWWMWWWWMWWWWWWMWWWWWWWWWWMNKOOKWWWWM\n\
WWWWWWWWWWWWWWWWWWWWWNNXXKXXXXXXNNNNWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWNX0OOKWWWWW\n\
WWWWWWMWWWWWWWWWWWWWWWWWNNNXXXXNNWWWWWMWWWWWWWWWWWWWWWWWWWWWMWWWWMWWWWNNXK000KNWWWWWW\n\
WWWMWWWWMMWWWMMWWWWMWWWWMMWWNNNXXXXXXXXXNNXXNNNNNNNWNNNNWNNXXXNNXKKKKKKKKXXNNWMMWWWWM\n\
MWWWWWMWWWWMWWWWMMWWWWMWWWWMWWWWMMWNNWNNXXXXXXXXXXKKKXXKKKXXXXKKXXNXNNWWWWWWMWWWWMWWW\n\
WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW\n\
NWWMMWNWMMWNWMMWWWMMMWWWMMWNWMMWWWMMMWNWMMWNWMMWWWWMWWWWMMWWWWMMWNWWMWWWWMMWNWMMWWWMM\n\
";

/* user parameters:
 * - using flags to determine using default or user specified value:
 *      0x01: conc
 *      0x02: conn
 *      0x04: file (if this bit is set, then disable url and method)
 *      0x08: url
 *      0x10: port
 *      0x20: method
 *      0x40: thrd
 */
enum {
    SPE_CONC=0x01,
    SPE_CONN=0x02,
    SPE_FILE=0x04,
    SPE_URL=0x08,
    SPE_PORT=0x10,
    SPE_METHOD=0x20,
    SPE_THRD=0x40
};

enum {
    USE_URL=1,
    USE_TEMPLATE
};

extern struct option options[NUM_PARAMS+REQ_HEADER_NAME_MAXIMUM];
/* flag for verbose print */
extern u8 verbose;
extern char *program;
extern int total_thrd_num;
extern int max_req_size;

// create argparse object
parsed_args_t *create_argparse();
// parse from argc, argv
int argparse(parsed_args_t **this, int argc, char **argv);
// parse url, and use the result to fill host & path
int parse_url(char *url, char **host, char **path);
// randomly pick a url and then update
int update_url_info_rand(parsed_args_t **this);
// print usage
void print_manual(u8 detail);

#endif