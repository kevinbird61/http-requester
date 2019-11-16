#include "logger.h"

int main(void)
{
    // single thr version
    syslog("MSG BROADCAST", __func__, "Hi! My name is kevin.", "Here is the first log!");
    syslog("BUG FOUND", "crashing_func", "Okay, Houston, we've had a problem here");
    syslog("DEBUG", "base", "This is Houston. Say again, please.");
    syslog("BUG DESCRIBE", "crashing_func", "Houston, we've had a problem. We've had a main B bus undervolt.");
    
    return 0;
}