#ifndef __REMBER_REMIND_EVERY_C
#define __REMBER_REMIND_EVERY_C

#include "ui.c"

typedef struct {
    UIElement *initial;
    UIElement *selected;

    UIElement delayH;
    UIElement delayMin;
    UIElement delayS;

    UIElement delayD;
    UIElement delayM;
    UIElement delayY;

    UIElement offsetH;
    UIElement offsetMin;
    UIElement offsetS;

    UIElement offsetD;
    UIElement offsetM;
    UIElement offsetY;
} RemindEveryUI;

void setupRemindEveryUI(void *reuip) {
    RemindEveryUI *reui = reuip;

    reui->delayH      = createUIInt(0, 24, 0);
    reui->delayMin    = createUIInt(0, 60, 0);
    reui->delayS      = createUIInt(0, 60, 0);

    reui->delayD      = createUIInt(1, 32, 0);
    reui->delayM      = createUIInt(1, 13, 0);
    reui->delayY      = createUIInt(0, 10000, 0);

    reui->offsetH     = createUIInt(0, 24, 0);
    reui->offsetMin   = createUIInt(0, 60, 0);
    reui->offsetS     = createUIInt(0, 60, 0);

    reui->offsetD     = createUIInt(1, 32, 0);
    reui->offsetM     = createUIInt(1, 13, 0);
    reui->offsetY     = createUIInt(0, 10000, 0);

    reui->initial = &(reui->delayH);
    reui->selected = reui->initial;

    verticalUIBond4((*reui), delayH, delayD, offsetH, offsetD);
    verticalUIBond4((*reui), delayMin, delayM, offsetMin, offsetY);
    verticalUIBond4((*reui), delayS, delayY, offsetS, offsetY);

    horizontalUIBond3((*reui), delayH, delayMin, delayS);
    horizontalUIBond3((*reui), delayD, delayM, delayY);
    horizontalUIBond3((*reui), offsetH, offsetMin, offsetS);
    horizontalUIBond3((*reui), offsetD, offsetM, offsetY);
}

void inputRemindEveryUI(void *reuip, char input) {
    RemindEveryUI *reui = reuip;

    if(input == 'h') {
        reui->selected = reui->selected->h;
    }
    if(input == 'j') {
        reui->selected = reui->selected->j;
    }
    if(input == 'k') {
        reui->selected = reui->selected->k;
    }
    if(input == 'l') {
        reui->selected = reui->selected->l;
    }

    if(input == 'J') {
        if(reui->selected->type != UI_INT) return;
        reui->selected->value = wrap(reui->selected->value - 1, reui->selected->min, reui->selected->max);
    }
    if(input == 'K') {
        if(reui->selected->type != UI_INT) return;
        reui->selected->value = wrap(reui->selected->value + 1, reui->selected->min, reui->selected->max);
    }
}

void renderRemindEveryUI(void *reuip) {
    RemindEveryUI *reui = reuip;

    v2 wres = getScreenRes();
    v2 res = _v2(25, 9);
    v2 row = _v2(0, 1);
    v2 pos = v2sub(wres, res);
    setCursor(pos);

    printf("*-------[ 1 / 3 ]-------*");
    pos = v2add(pos, row); setCursor(pos);
    printf("   Delay:                ");
    pos = v2add(pos, row); setCursor(pos);

    // printf("\e[48;5;%sm", STYLE_VISUAL_SELECTED); printf("\e[38;5;0m");
    // printf("\e[0m");

    char *delayHC = reui->selected == &reui->delayH ? "\e[48;5;255m\e[38;5;0m" : "";
    char *delayMinC = reui->selected == &reui->delayMin ? "\e[48;5;255m\e[38;5;0m" : "";
    char *delaySC = reui->selected == &reui->delayS ? "\e[48;5;255m\e[38;5;0m" : "";

    char *delayDC = reui->selected == &reui->delayD ? "\e[48;5;255m\e[38;5;0m" : "";
    char *delayMC = reui->selected == &reui->delayM ? "\e[48;5;255m\e[38;5;0m" : "";
    char *delayYC = reui->selected == &reui->delayY ? "\e[48;5;255m\e[38;5;0m" : "";

    char *offsetHC = reui->selected == &reui->offsetH ? "\e[48;5;255m\e[38;5;0m" : "";
    char *offsetMinC = reui->selected == &reui->offsetMin ? "\e[48;5;255m\e[38;5;0m" : "";
    char *offsetSC = reui->selected == &reui->offsetS ? "\e[48;5;255m\e[38;5;0m" : "";

    char *offsetDC = reui->selected == &reui->offsetD ? "\e[48;5;255m\e[38;5;0m" : "";
    char *offsetMC = reui->selected == &reui->offsetM ? "\e[48;5;255m\e[38;5;0m" : "";
    char *offsetYC = reui->selected == &reui->offsetY ? "\e[48;5;255m\e[38;5;0m" : "";


    printf("    [%s%02d\e[0m]h [%s%02d\e[0m]m [%s%02d\e[0m]s    ", delayHC, reui->delayH.value, delayMinC, reui->delayMin.value, delaySC, reui->delayS.value);
    pos = v2add(pos, row); setCursor(pos);

    printf("    [%s%02d\e[0m]d [%s%02d\e[0m]m [%s%02d\e[0m]y    ", delayDC, reui->delayD.value, delayMC, reui->delayM.value, delayYC, reui->delayY.value);
    pos = v2add(pos, row); setCursor(pos);

    printf("                         ");
    pos = v2add(pos, row); setCursor(pos);
    printf("   Offset:               ");
    pos = v2add(pos, row); setCursor(pos);

    printf("    [%s%02d\e[0m]h [%s%02d\e[0m]m [%s%02d\e[0m]s    ", offsetHC, reui->offsetH.value, offsetMinC, reui->offsetMin.value, offsetSC, reui->offsetS.value);
    pos = v2add(pos, row); setCursor(pos);

    printf("    [%s%02d\e[0m]d [%s%02d\e[0m]m [%s%04d\e[0m]y  ", offsetDC, reui->offsetD.value, offsetMC, reui->offsetM.value, offsetYC, reui->offsetY.value);
    pos = v2add(pos, row); setCursor(pos);

    printf("*-----------------------*");
    pos = v2add(pos, row); setCursor(pos);
}

#endif // __REMBER_REMIND_EVERY_C
