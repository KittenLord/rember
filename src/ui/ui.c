#ifndef __REMBER_UI_C
#define __REMBER_UI_C

#include <stdbool.h>

#define UI_BOOL 0
#define UI_INT 1

typedef struct InteractiveState InteractiveState;

typedef struct UIElement UIElement;

struct UIElement {
    UIElement *h;
    UIElement *j;
    UIElement *k;
    UIElement *l;

    char type;
    union {
        struct {
            bool bvalue;
        };
        struct {
            int min;
            int max;
            int value;
        };
    };
};

UIElement createUIInt(int min, int max, int value) {
    UIElement e = {0};
    e.type = UI_INT;
    e.min = min;
    e.max = max;
    e.value = value;
    return e;
}

void verticalUIBond(UIElement *up, UIElement *down) {
    up->j = down;
    down->k = up;
}

void horizontalUIBond(UIElement *left, UIElement *right) {
    left->l = right;
    right->h = left;
}

#define verticalUIBond2(pre, a, b) do { pre.a.k = &pre.a; verticalUIBond(&pre.a, &pre.b); pre.b.j = &pre.j; } while(0)
#define verticalUIBond3(pre, a, b, c) do { pre.a.k = &pre.a; verticalUIBond(&pre.a, &pre.b); verticalUIBond(&pre.b, &pre.c); pre.c.j = &pre.c; } while(0)
#define verticalUIBond4(pre, a, b, c, d) do { pre.a.k = &pre.a; verticalUIBond(&pre.a, &pre.b); verticalUIBond(&pre.b, &pre.c); verticalUIBond(&pre.c, &pre.d); pre.d.j = &pre.d; } while(0)

#define horizontalUIBond2(pre, a, b) do { pre.a.h = &pre.a; horizontalUIBond(&pre.a, &pre.b); pre.b.l = &pre.b; } while(0)
#define horizontalUIBond3(pre, a, b, c) do { pre.a.h = &pre.a; horizontalUIBond(&pre.a, &pre.b); horizontalUIBond(&pre.b, &pre.c); pre.c.l = &pre.c; } while(0)
#define horizontalUIBond4(pre, a, b, c, d) do { pre.a.h = &pre.a; horizontalUIBond(&pre.a, &pre.b); horizontalUIBond(&pre.b, &pre.c); horizontalUIBond(&pre.c, &pre.d); pre.d.l = &pre.d; } while(0)

#endif // __REMBER_UI_C
