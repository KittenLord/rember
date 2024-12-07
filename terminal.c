#include <stdlib.h>

// TODO: fill out win32 lol

// TODO: figure out whether color works the same on windows or not

void goto00() {
#if __linux__
    printf("\e[H");
#elif _WIN32

#endif

}

void restoreScreen() {
#if __linux__
    printf("\e[?1049l");
#elif _WIN32

#endif
}

void eraseScreen() {
#if __linux__
    printf("\e[2J");
#elif _WIN32

#endif
}

void gotoStartNextLine() {
#if __linux__
    printf("\e[1E");
#elif _WIN32

#endif
}

void saveScreen() {
#if __linux__
    printf("\e[?1049h");
#elif _WIN32

#endif
}

void invisibleCursorOn() {
#if __linux__
    printf("\e[?25l");
#elif _WIN32

#endif
}
