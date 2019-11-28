#include "logger.h"

int main(void)
{
    // single thr version
    LOG(NORMAL, "%s\n","Hi! My name is kevin.", "Here is the first log!");
    LOG(WARNING, "%s\n", "Okay, Houston, we've had a problem here");
    LOG(INFO, "%s\n", "This is Houston. Say again, please.");
    LOG(ERROR, "%s\n", "Houston, we've had a problem. We've had a main B bus undervolt.");

    printf("Now using `cat %s%s%s` to see the result!\n", log_dir, log_filename, log_ext);

    return 0;
}