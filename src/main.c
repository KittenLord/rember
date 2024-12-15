#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "planitems.c"
#include "interactive.c"

#define SUB_INTERACTIVE "interactive"

int main(int argc, char **argv) {
    char *subcommand = SUB_INTERACTIVE;

    for(int i = 1; i < argc; i++) {
        printf("%s\n", argv[i]);
    }

    if(!strcmp(subcommand, SUB_INTERACTIVE)) {
        interactive();
    }
}
