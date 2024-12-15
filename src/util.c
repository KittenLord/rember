#ifndef __REMBER_UTIL_C
#define __REMBER_UTIL_C


int clamp(int n, int a, int b) {
    if(n < a) return a;
    if(n >= b) return b - 1;
    return n;
}

void ctob(char c, char buf[8]) {
    for(int i = 0; i < 8; i++) { buf[i] = ((c >> (8 - i - 1)) & 1) + '0'; }
}


#endif // __REMBER_UTIL_C
