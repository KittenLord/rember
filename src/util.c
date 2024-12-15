#ifndef __REMBER_UTIL_C
#define __REMBER_UTIL_C

#include <stdio.h>

int clamp(int n, int a, int b) {
    if(n < a) return a;
    if(n >= b) return b - 1;
    return n;
}

int wrap(int n, int a, int b) {
    if(n < a) return wrap(b + (n - a), a, b);
    if(n >= b) return wrap(a + (n - b), a, b);
}

void ctob(char c, char buf[8]) {
    for(int i = 0; i < 8; i++) { buf[i] = ((c >> (8 - i - 1)) & 1) + '0'; }
}

typedef int cint;
typedef struct {
    union {
        struct {
            cint x;
            cint y;
        };
        struct {
            cint w;
            cint h;
        };
        struct {
            cint width;
            cint height;
        };
        struct {
            cint col;
            cint row;
        };
    };
} v2;
#define _v2(_x, _y) ((v2){ .x = _x, .y = _y, })
#define v20 _v2(0, 0)

v2 v2add(v2 a, v2 b) {
    return _v2(a.x + b.x, a.y + b.y);
}

v2 v2sub(v2 a, v2 b) {
    return _v2(a.x - b.x, a.y - b.y);
}

void setCursor(v2 p) {
    printf("\e[%d;%dH", p.row + 1, p.col + 1);
}

v2 getScreenRes() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return _v2(w.ws_col, w.ws_row);
}

#endif // __REMBER_UTIL_C
