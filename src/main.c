#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "planitems.c"
#include "interactive.c"

int main(int argc, char **argv) {
    
    struct tm currentTime;
    time_t currentTimeT = time(NULL);

    gmtime_r(&currentTimeT, &currentTime);


    // printf("%d-%d-%d, %dh %dn %ds\n", currentTime.tm_year + 1900, currentTime.tm_mon, currentTime.tm_mday, currentTime.tm_hour, currentTime.tm_min, currentTime.tm_sec);

    interactive();
}
