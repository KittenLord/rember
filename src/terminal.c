#ifndef __REMBER_TERMINAL_C
#define __REMBER_TERMINAL_C

#include <stdlib.h>

void goto00() {
    printf("\e[H");
}

void restoreScreen() {
    printf("\e[?1049l");
}

void eraseScreen() {
    printf("\e[2J");
}

void gotoStartNextLine() {
    printf("\e[1E");
}

void saveScreen() {
    printf("\e[?1049h");
}

void invisibleCursorOn() {
    printf("\e[?25l");
}

#endif // __REMBER_TERMINAL_C
