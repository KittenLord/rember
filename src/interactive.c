#ifndef __REMBER_INTERACTIVE_C
#define __REMBER_INTERACTIVE_C

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include <assert.h>
#ifdef __linux__
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#elif _WIN32
#include <windows.h>
#include <fcntl.h>
#endif

#include "terminal.c"
#include "planitems.c"
#include "util.c"

#ifdef __linux__
#define KEY_BACKSPACE 0x7F
#elif _WIN32
#define KEY_BACKSPACE 0x08
#endif

#ifdef __linux__
#define KEY_ENTER 0x0A
#elif _WIN32
#define KEY_ENTER 0x0D
#endif

#define KEY_TAB 0x09

// a very bad solution, but idk how to fix this atm
#ifdef __linux__
#define KEY_ESCAPE '\e'
#elif _WIN32
#define KEY_ESCAPE '`'
#endif

#define STYLE_COLLAPSED "8"
#define STYLE_PARTIAL "221"
#define STYLE_DONE "34"
#define STYLE_NOTDONE "255"
#define STYLE_VISUAL "75"
#define STYLE_VISUAL_SELECTED "87"

#define NORMAL_MODE 0
#define INSERT_MODE 1
#define VISUAL_MODE 2

struct InteractiveState {
    PlanItem *root;
    char mode;
    int selectedIndex;

    int vStart;
    int vEnd;
};
struct InteractiveState s;

int renderPlanItem(PlanItem *item, int indent, int index, int selectedIndex, bool visual, int vStart, int vEnd, bool init);

void render() {
    renderPlanItem(s.root, 0, 0, s.selectedIndex, s.mode == VISUAL_MODE, s.vStart, s.vEnd, true);
}

int renderPlanItem(PlanItem *item, int indent, int index, int selectedIndex, bool visual, int vStart, int vEnd, bool init) {
    if(init) { eraseScreen(); goto00(); if(item->children) renderPlanItem(item->children, 0, 0, selectedIndex, visual, vStart, vEnd, false); return 0; }

    for(int i = 0; i < indent; i++) putchar(' ');

    // TODO: refactor into separate function
    char *color = STYLE_NOTDONE;
    int dc = getAmountOfDoneChildren(item);
    if(isDone(item)) color = STYLE_DONE;
    else if(dc > 0) color = STYLE_PARTIAL;
    if(item->collapsed) color = STYLE_COLLAPSED;
    bool isVisualSelected = visual && index >= vStart && index <= vEnd;
    if(isVisualSelected) color = STYLE_VISUAL;
    if(isVisualSelected && index == selectedIndex) color = STYLE_VISUAL_SELECTED;

    if(index == selectedIndex || isVisualSelected) { printf("\e[48;5;%sm", color); printf("\e[38;5;0m"); }
    else { printf("\e[38;5;%sm", color); }

    char collapsedChar = item->collapsed ? '+' : '-';

    if(item->children) {
        int children = getAmountOfChildren(item);
        int doneChildren = getAmountOfDoneChildren(item);

        printf("%c [%d/%d] ", collapsedChar, doneChildren, children);
    }
    else {
        char x = item->done ? 'x' : ' ';
        printf("%c [%c] ", collapsedChar, x);
    }

    write(STDOUT_FILENO, item->text, item->len);

    printf("\e[0m");

    gotoStartNextLine();

    if(item->children && !item->collapsed) { index = renderPlanItem(item->children, indent + 4, index + 1, selectedIndex, visual, vStart, vEnd, false); }
    if(item->next) { index = renderPlanItem(item->next, indent, index + 1, selectedIndex, visual, vStart, vEnd, false); }
    return index;
}

void interactive() {

#ifdef __linux__
    struct termios term, restore;
    tcgetattr(STDIN_FILENO, &term);
    tcgetattr(STDIN_FILENO, &restore);
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
#elif _WIN32
    DWORD term, restore;
    HANDLE console = GetStdHandle(STD_INPUT_HANDLE);

    GetConsoleMode(console, &term);
    GetConsoleMode(console, &restore);
    term &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
    SetConsoleMode(console, term);
    _setmode( _fileno( stdin), _O_BINARY); // enter is read properly
#endif
    setbuf(stdout, NULL);
    setbuf(stdin, NULL);


    saveScreen();
    invisibleCursorOn();

    FILE *logfile = fopen("./log.txt", "w+");
    if(logfile) fclose(logfile); \

    FILE *savefile = fopen("./planner.txt", "rb+");

    #define log(text, ...) do { \
        logfile = fopen("./log.txt", "a+"); \
        if(logfile) { \
            fprintf(logfile, text, __VA_ARGS__); \
            fclose(logfile); \
        } \
    } while(0)

    bool new = false;
    if(!savefile) { new = true; savefile = fopen("./planner.txt", "wb+"); }

    if(!new) {
        fseek(savefile, 0, SEEK_END);
        size_t len = ftell(savefile);
        fseek(savefile, 0, SEEK_SET);
        char *buffer = malloc(len);
        fread(buffer, sizeof(char), len, savefile);
        char *tempBuf = buffer;
        s.root = parsePlanItem(&tempBuf);
        free(buffer);
    } 
    else {
        #define ROOT_NAME "ROOT"
        #define ROOT_LEN 4
        s.root = calloc(1, sizeof(PlanItem));
        s.root->text = malloc(ROOT_LEN);
        s.root->len = ROOT_LEN;
        s.root->capacity = ROOT_LEN; // not necessary
        memcpy(s.root->text, ROOT_NAME, ROOT_LEN);
        writePlanItem(s.root, savefile);
    }

    fclose(savefile);

    PlanItem *selected = s.root;
    s.selectedIndex = 0;

    PlanItem *copyBuffer = NULL;

    char input;
    char buf[9] = "xxxxxxxx\0";

    s.vStart = -1;
    s.vEnd = -1;

    s.mode = NORMAL_MODE;

    bool doSave = true;

    char *simulatedInput = "";
    size_t simulatedInputLen = 0;

    #define simulate(input) do { simulatedInput = input; simulatedInputLen = strlen(input); } while(0)

    int iteration = 0;
    while(true) { 
        render();

        if(simulatedInputLen == 0) {
            read(STDIN_FILENO, &input, 1);
        }
        else {
            input = *simulatedInput;
            simulatedInput++;
            simulatedInputLen--;
        }

        ctob(input, buf);
        log("SELECTED INDEX: %d\n", s.selectedIndex);
        log("AMOUNT: %d\n", getAmountOfChildren(s.root));
        log("ROOT NEXT: %p\n", s.root->next);
        log("KEY: %s\n", buf);
        log("ITERATION: %d\n", ++iteration);

        if(s.mode == NORMAL_MODE) {
            // maybe auto-saving shouldn't be the default?
            if(input == 'q') break;
            if(input == 'Q') { doSave = false; break; }
            if(input == 'o' && selected == s.root) input = 'a';
            if(input == 'a') {
                PlanItem *child = calloc(1, sizeof(PlanItem));
                child->text = calloc(16, sizeof(char));
                child->capacity = 16;

                child->next = selected->children;
                selected->children = child;
                selected = child;
                s.selectedIndex = getPlanItemIndex(s.root, selected);
                s.mode = INSERT_MODE;
            }
            if(input == 'o') {
                PlanItem *nnext = calloc(1, sizeof(PlanItem));
                nnext->text = calloc(16, sizeof(char));
                nnext->capacity = 16;

                nnext->next = selected->next;
                selected->next = nnext;
                selected = nnext;
                s.selectedIndex = getPlanItemIndex(s.root, selected);
                s.mode = INSERT_MODE;
            }
            if(input == 'j') {
                // TODO: This should count ONLY visible ones
                int amount = getTotalAmount(s.root);
                s.selectedIndex = clamp(s.selectedIndex + 1, 0, amount);
                selected = getPlanItemAtIndex(s.root, s.selectedIndex);
            }
            if(input == 'k') {
                int amount = getTotalAmount(s.root);
                s.selectedIndex = clamp(s.selectedIndex - 1, 0, amount);
                selected = getPlanItemAtIndex(s.root, s.selectedIndex);
            }
            if(input == 'c') {
                selected->len = 0;
                s.mode = INSERT_MODE;
            }
            if(input == 'd') {
                simulate("vd");
            }
            if(input == 'w') {
                savefile = fopen("./planner.txt", "wb+");
                writePlanItem(s.root, savefile);
                fclose(savefile);
            }
            if(input == 'v') {
                s.mode = VISUAL_MODE;
                s.vStart = s.selectedIndex;
                s.vEnd = s.selectedIndex;
            }
            if(input == 'p' && selected == s.root) { input = 'P'; }
            if(input == 'p') {
                if(!copyBuffer) continue;
                PlanItem *lastSameNest = getLastSameNest(copyBuffer);
                lastSameNest->next = selected->next;
                selected->next = copyBuffer;

                s.selectedIndex = getPlanItemIndex(s.root, copyBuffer);
                selected = copyBuffer;

                copyBuffer = planItemDeepCopy(copyBuffer, true);
            }
            if(input == 'P') {
                if(!copyBuffer) continue;
                PlanItem *lastSameNest = getLastSameNest(copyBuffer);
                if(lastSameNest) lastSameNest->next = selected->children;
                selected->children = copyBuffer;

                s.selectedIndex = getPlanItemIndex(s.root, copyBuffer);
                selected = copyBuffer;

                copyBuffer = planItemDeepCopy(copyBuffer, true);
            }
            if(input == KEY_TAB) {
                if(selected->children) selected->collapsed = !selected->collapsed;
                else                   selected->collapsed = false;
            }
            if(input == KEY_ENTER) {
                selected->done = !selected->done;

                if(selected->done) {
                    selected->lastCheckedStaged = time(NULL);
                }
                else {
                    selected->lastCheckedStaged = selected->remindInfo.lastChecked;
                }
            }
        }
        // TODO: fix for cyrillic (and unicode in general)

        // TODO: cancel input
        else if(s.mode == INSERT_MODE) {
            if(input == KEY_ENTER) { if(selected->len > 0) { s.mode = NORMAL_MODE; } continue; }
            if(input == KEY_BACKSPACE) { if(selected->len > 0) selected->len--; continue; }

            if(selected->len >= selected->capacity) {
                selected->text = realloc(selected->text, selected->capacity * 2);
                selected->capacity *= 2;
            }
            selected->text[(selected->len)++] = input;
        }
        else if(s.mode == VISUAL_MODE) {
            // currently doesn't work on windows, and i'm not sure why...
            if(input == KEY_ESCAPE) { s.mode = NORMAL_MODE; }
            if(input == 'v') { s.mode = NORMAL_MODE; }
            if(input == 'j') {
                if(!selected->next) continue;
                s.selectedIndex = getPlanItemIndex(s.root, selected->next);
                selected = selected->next;
                int index = s.selectedIndex;
                if(selected->children) index = getPlanItemIndex(s.root, getLastSameNest(selected->children));
                if(s.selectedIndex > s.vEnd) s.vEnd = index;
            }
            // TODO: if an item with children gets selected, visually select all children
            if(input == 'k') {
                PlanItem *previous = getItemBefore(s.root, selected);
                if(!previous) continue;
                if(previous == s.root) continue;
                int previousIndex = getPlanItemIndex(s.root, previous);
                // bool isParent = previous->children == selected;
                s.selectedIndex = previousIndex;
                selected = previous;
                if(s.selectedIndex < s.vStart) s.vStart = s.selectedIndex;
            }
            if(input == 'd') {
                PlanItem *item = getPlanItemAtIndex(s.root, s.vStart);
                if(item == s.root) {
                    simulate("v");
                    continue;
                }

                PlanItem *previous = getItemBefore(s.root, item);

                PlanItem *removed = removeSelection(s.root, s.vStart, s.vEnd);
                freePlanItem(copyBuffer, true);
                copyBuffer = removed;
                s.mode = NORMAL_MODE;

                if(previous) {
                    selected = previous;
                    s.selectedIndex = getPlanItemIndex(s.root, previous);
                }
                else {
                    selected = getFirstItemSafe(s.root);
                    s.selectedIndex = 0;
                }
            }
            // TODO: implement copy 'y'
        }
    }

    if(doSave) { 
        savefile = fopen("./planner.txt", "wb+");
        writePlanItem(s.root, savefile);
        fclose(savefile);
    }

    restoreScreen();

#ifdef __linux
    tcsetattr(STDIN_FILENO, TCSANOW, &restore);
#elif _WIN32
    SetConsoleMode(console, term);
#endif
}

#endif // __REMBER_INTERACTIVE_C
