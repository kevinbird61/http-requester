#ifndef __STATS__
#define __STATS__

/** statistics/counters, provide sysadmin to trace all activities. 
 * - using several results to categorize statistics:
 *      - return code
 *      - status code
 * 
 * - print format:
 *  - connection fail
 *      - [invalid_xxx]: argument error
 *      - ...
 *  - connection establish
 *      - [1xx] international
 *      - [2xx] success
 *      - [3xx] redirect
 *      - [4xx] client error
 *      - [5xx] server error
 */

// essential deps
#include "http.h"
#include "types.h"


#endif