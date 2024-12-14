#include <stdlib.h>

// TODO: fill out win32 lol

// TODO: figure out whether color works the same on windows or not

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
