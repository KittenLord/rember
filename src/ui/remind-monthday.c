#ifndef __REMBER_REMIND_MONTHDAY_C
#define __REMBER_REMIND_MONTHDAY_C

#include "ui.c"

typedef struct {
    UIElement *initial;
    UIElement *selected;

    UIElement jan;
    UIElement feb;
    UIElement mar;
    UIElement apr;
    UIElement may;
    UIElement jun;
    UIElement jul;
    UIElement aug;
    UIElement sep;
    UIElement oct;
    UIElement nov;
    UIElement dec;

    UIElement dAll;

    UIElement d01;
    UIElement d02;
    UIElement d03;
    UIElement d04;
    UIElement d05;
    UIElement d06;
    UIElement d07;
    UIElement d08;
    UIElement d09;
    UIElement d10;
    UIElement d11;
    UIElement d12;
    UIElement d13;
    UIElement d14;
    UIElement d15;
    UIElement d16;
    UIElement d17;
    UIElement d18;
    UIElement d19;
    UIElement d20;
    UIElement d21;
    UIElement d22;
    UIElement d23;
    UIElement d24;
    UIElement d25;
    UIElement d26;
    UIElement d27;
    UIElement d28;
    UIElement d29;
    UIElement d30;
    UIElement d31;

    UIElement timeH;
    UIElement timeMin;
    UIElement timeS;
} RemindWeekdayUI;

void setupRemindMonthdayUI(void *remdp);
void inputRemindMonthdayUI(void *remdp, char input);
void renderRemindMonthdayUI(void *remdp);

#endif // __REMBER_REMIND_MONTHDAY_C
