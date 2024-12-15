#ifndef __REMBER_REMIND_WEEKDAY_C
#define __REMBER_REMIND_WEEKDAY_C

#include "ui.c"

typedef struct {
    UIElement *initial;
    UIElement *selected;

    UIElement sun;
    UIElement mon;
    UIElement tue;
    UIElement wed;
    UIElement thu;
    UIElement fri;
    UIElement sat;

    UIElement timeH;
    UIElement timeMin;
    UIElement timeS;
} RemindWeekdayUI;

void setupRemindWeekdayUI(void *rewdp);
void inputRemindWeekdayUI(void *rewdp, char input);
void renderRemindWeekdayUI(void *rewdp);

#endif // __REMBER_REMIND_WEEKDAY_C
