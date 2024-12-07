#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <termios.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

void ctob(char c, char buf[8]) {
    for(int i = 0; i < 8; i++) { buf[i] = ((c >> (8 - i - 1)) & 1) + '0'; }
}

typedef struct PlanItem PlanItem;

typedef struct PlanItem {
    char *text;
    size_t len;
    bool done;

    size_t capacity;
    bool collapsed;

    PlanItem *children;
    PlanItem *next;
};

void freePlanItem(PlanItem *item, bool freeNext) {
    if(!item) return;
    if(freeNext) freePlanItem(item->next, freeNext);
    freePlanItem(item->children, true);
    free(item);
}

#define NO_ITEM 0
#define NEXT_ITEM 1
#define CHILD_ITEM 2

PlanItem *parsePlanItem(char **data) {
    size_t text_len = *((*data)++);
    if(!text_len) return NULL;

    PlanItem *this = calloc(1, sizeof(PlanItem));

    this->text = malloc(text_len);
    this->len = text_len;
    this->capacity = text_len;
    memcpy(this->text, *data, text_len);
    *data += text_len;

    this->done = !!*((*data)++);
    char control = *((*data)++);

    if(control == NO_ITEM) return this;

    if(control == CHILD_ITEM) { this->children = parsePlanItem(data); control = *((*data)++); }
    if(control == NEXT_ITEM) { this->next = parsePlanItem(data); control = *((*data)++); }
    assert(control == NO_ITEM);
    return this;
}

// len text x 2 ... 0 1 ... 0

void writePlanItem(PlanItem *item, FILE *file) {
    char len = *(char *)(&(item->len));
    fwrite(&len, sizeof(char), 1, file);
    fwrite(item->text, sizeof(char), len, file);
    fwrite(&item->done, sizeof(char), 1, file);

    char noItem = NO_ITEM;
    char nextItem = NEXT_ITEM;
    char childItem = CHILD_ITEM;

    if(item->children) { fwrite(&childItem, sizeof(char), 1, file); writePlanItem(item->children, file); }
    if(item->next) { fwrite(&nextItem, sizeof(char), 1, file); writePlanItem(item->next, file); }
    fwrite(&noItem, sizeof(char), 1, file);
    return;
}

int getAmountOfDoneChildren(PlanItem *item);

int getAmountSameNest(PlanItem *item) {
    if(!item) return 0;
    return 1 + getAmountSameNest(item->next);
}

int getAmountOfChildren(PlanItem *item) {
    return getAmountSameNest(item->children);
}

bool isDone(PlanItem *item) {
    if(!item->children) return item->done;
    int children = getAmountOfChildren(item);
    int doneChildren = getAmountOfDoneChildren(item);
    return doneChildren >= children;
}

int getAmountOfDoneSameNest(PlanItem *item) {
    if(!item) return 0;
    return isDone(item) + getAmountOfDoneSameNest(item->next);
}

int getAmountOfDoneChildren(PlanItem *item) {
    return getAmountOfDoneSameNest(item->children);
}

#define STYLE_COLLAPSED "8"
#define STYLE_PARTIAL "221"
#define STYLE_DONE "34"
#define STYLE_NOTDONE "255"
#define STYLE_VISUAL "75"
#define STYLE_VISUAL_SELECTED "87"

int renderPlanItem(PlanItem *item, int indent, int index, int selectedIndex, bool visual, int vStart, int vEnd, bool init) {
    if(init) { printf("\e[2J"); printf("\e[H"); if(item->children) renderPlanItem(item->children, 0, 0, selectedIndex, visual, vStart, vEnd, false); return 0; }

    for(int i = 0; i < indent; i++) putchar(' ');

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

    printf("\e[1E"); // new line

    if(item->children && !item->collapsed) { index = renderPlanItem(item->children, indent + 4, index + 1, selectedIndex, visual, vStart, vEnd, false); }
    if(item->next) { index = renderPlanItem(item->next, indent, index + 1, selectedIndex, visual, vStart, vEnd, false); }
    return index;
}

int __getPlanItemAtIndex(PlanItem *current, PlanItem **result, int index, int currentIndex) {
    if(!current) return 0;
    if(currentIndex == index) { *result = current; return 0; }
    if(current->children && !current->collapsed) currentIndex = __getPlanItemAtIndex(current->children, result, index, currentIndex + 1);
    if(*result) return 0;
    if(current->next) currentIndex = __getPlanItemAtIndex(current->next, result, index, currentIndex + 1);
    return currentIndex;
}

PlanItem *getPlanItemAtIndex(PlanItem *root, int index) {
    PlanItem *result = NULL;
    __getPlanItemAtIndex(root->children, &result, index, 0);
    return result;
}

int getPlanItemAmount(PlanItem *current) {
    if(!current) return 0;
    int amount = 1;
    amount += getPlanItemAmount(current->children);
    amount += getPlanItemAmount(current->next);
    return amount;
}

int clamp(int n, int a, int b) {
    if(n < a) return a;
    if(n >= b) return b - 1;
    return n;
}

int __getPlanItemIndex(PlanItem *current, PlanItem *target, int index, int *result) {
    if(!current) return index;
    if(target == current) { *result = index; return 0; }
    if(current->children && !current->collapsed) index = __getPlanItemIndex(current->children, target, index + 1, result);
    if(*result > 0) return 0;
    if(current->next) index = __getPlanItemIndex(current->next, target, index + 1, result);
    return index;
}

int getPlanItemIndex(PlanItem *root, PlanItem *target) {
    int result = -1;
    __getPlanItemIndex(root->children, target, 0, &result);
    return result;
}

PlanItem *__getPreviousItemVisual(PlanItem *current, PlanItem *targetNext) {
    if(!current) return NULL;
    if(current->children == targetNext) return current;
    if(current->next == targetNext) return current;
    PlanItem *next = NULL;
    if(current->children) next = __getPreviousItemVisual(current->children, targetNext);
    if(next) return next;
    if(current->next) next = __getPreviousItemVisual(current->next, targetNext);
    return next;
}

PlanItem *getPreviousItemVisual(PlanItem *root, PlanItem *targetNext) {
    return __getPreviousItemVisual(root->children, targetNext);
}

PlanItem *removeSelection(PlanItem *root, int vStart, int vEnd) {
    PlanItem *item = getPlanItemAtIndex(root, vStart);
    PlanItem *search = item;

    PlanItem *parent = getPreviousItemVisual(root, item);

    PlanItem *replace = NULL;
    while(search->next) {
        int nextIndex = getPlanItemIndex(root, search->next);
        if(nextIndex > vEnd) { replace = search->next; search->next = NULL; break; }
        search = search->next;
    }

    if(parent->next == item) { parent->next = replace; }
    else if(parent->children == item) { parent->children = replace; }
    return item;
}

PlanItem *getLastSameNest(PlanItem *item) {
    if(!item->next) return item;
    return getLastSameNest(item->next);
}

PlanItem *planItemDeepCopy(PlanItem *base, bool cloneNext) {
    PlanItem *clone = malloc(sizeof(PlanItem));
    clone->len = base->len;
    clone->capacity = base->capacity;
    clone->text = malloc(clone->len);
    clone->done = base->done;
    clone->collapsed = false;
    memcpy(clone->text, base->text, clone->len);

    clone->children = planItemDeepCopy(base->children, true);
    if(cloneNext) clone->next = planItemDeepCopy(base->next, true);
    return clone;
}

int main(int argc, char **argv) {
    struct termios term, restore;
    tcgetattr(STDIN_FILENO, &term);
    tcgetattr(STDIN_FILENO, &restore);
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
    setbuf(stdout, NULL);

    printf("\e[?1049h"); // save previous screen
    printf("\e[?25l"); // make cursor invisible

    FILE *logfile = fopen("./log.txt", "w+");
    FILE *savefile = fopen("./planner.txt", "r+");

    bool new = false;
    if(!savefile) { new = true; savefile = fopen("./planner.txt", "w+"); }

    PlanItem *root;
    if(!new) {
        fseek(savefile, 0, SEEK_END);
        size_t len = ftell(savefile);
        fseek(savefile, 0, SEEK_SET);
        char *buffer = malloc(len);
        fread(buffer, sizeof(char), len, savefile);
        char *tempBuf = buffer;
        root = parsePlanItem(&tempBuf);
        free(buffer);
    } 
    else {
        #define ROOT_NAME "ROOT"
        #define ROOT_LEN 4
        root = calloc(1, sizeof(PlanItem));
        root->text = malloc(ROOT_LEN);
        root->len = ROOT_LEN;
        root->capacity = ROOT_LEN; // not necessary
        memcpy(root->text, ROOT_NAME, ROOT_LEN);
        writePlanItem(root, savefile);
    }

    fclose(savefile);

    PlanItem *selected = root;
    int selectedIndex = 0;

    PlanItem *copyBuffer = NULL;

    char input;
    char buf[8];

    int visualSelectionStart = -1;
    int visualSelectionEnd = -1;

    #define NORMAL_MODE 0
    #define INSERT_MODE 1
    #define VISUAL_MODE 2

    int mode = NORMAL_MODE;

    bool doSave = true;

    char *simulatedInput = "";
    size_t simulatedInputLen = 0;

    while(true) { 
        renderPlanItem(root, 0, 0, selectedIndex, mode == VISUAL_MODE, visualSelectionStart, visualSelectionEnd, true);

        if(simulatedInputLen == 0) read(STDIN_FILENO, &input, 1);
        else {
            input = *simulatedInput;
            simulatedInput++;
            simulatedInputLen--;
        }

        ctob(input, buf);
        fwrite(buf, sizeof(char), 8, logfile);
        fwrite("\n", sizeof(char), 1, logfile);

        if(mode == NORMAL_MODE) {
            if(input == 'q') break;
            if(input == 'Q') { doSave = false; break; }
            if(input == 'o' && selected == root) input = 'a';
            if(input == 'a') {
                PlanItem *child = calloc(1, sizeof(PlanItem));
                child->text = calloc(16, sizeof(char));
                child->capacity = 16;

                child->next = selected->children;
                selected->children = child;
                selected = child;
                selectedIndex = getPlanItemIndex(root, selected);
                mode = INSERT_MODE;
            }
            if(input == 'o') {
                PlanItem *nnext = calloc(1, sizeof(PlanItem));
                nnext->text = calloc(16, sizeof(char));
                nnext->capacity = 16;

                nnext->next = selected->next;
                selected->next = nnext;
                selected = nnext;
                selectedIndex = getPlanItemIndex(root, selected);
                mode = INSERT_MODE;
            }
            if(input == 'j') {
                // TODO: This should count ONLY visible ones
                int amount = getPlanItemAmount(root) - 1; // -1 to account for root
                selectedIndex = clamp(selectedIndex + 1, 0, amount);
                selected = getPlanItemAtIndex(root, selectedIndex);
            }
            if(input == 'k') {
                int amount = getPlanItemAmount(root) - 1; // -1 to account for root
                selectedIndex = clamp(selectedIndex - 1, 0, amount);
                selected = getPlanItemAtIndex(root, selectedIndex);
            }
            if(input == 'c') {
                selected->len = 0;
                mode = INSERT_MODE;
            }
            // this is actually identical to "vd", wonder if I can do something about it...
            if(input == 'd') {
                simulatedInput = "vd";
                simulatedInputLen = 2;
            }
            if(input == 'w') {
                savefile = fopen("./planner.txt", "w+");
                writePlanItem(root, savefile);
                fclose(savefile);
            }
            if(input == 'v') {
                mode = VISUAL_MODE;
                visualSelectionStart = selectedIndex;
                visualSelectionEnd = selectedIndex;
            }
            if(input == 'p' && selected == root) { input == 'P'; }
            if(input == 'p') {
                if(!copyBuffer) continue;
                PlanItem *lastSameNest = getLastSameNest(copyBuffer);
                lastSameNest->next = selected->next;
                selected->next = copyBuffer;
                copyBuffer = planItemDeepCopy(copyBuffer, true);
            }
            if(input == 'P') {
                if(!copyBuffer) continue;
                PlanItem *lastSameNest = getLastSameNest(copyBuffer);
                lastSameNest->next = selected->children;
                selected->children = copyBuffer;
                copyBuffer = planItemDeepCopy(copyBuffer, true);
            }
            if(input == 0x09) {
                if(selected->children) selected->collapsed = !selected->collapsed;
                else                   selected->collapsed = false;
            }
            if(input == 0x0A) {
                selected->done = !selected->done;
            }
        }
        // TODO: fix for cyrillic (and unicode in general)
        else if(mode == INSERT_MODE) {
            // if(input == 'q') break;
            if(input == 0x0A) { if(selected->len > 0) { mode = NORMAL_MODE; } continue; }
            if(input == 0x7F) {
                if(selected->len <= 0) continue;
                selected->len--;
                continue;
            }
            else if(selected->len >= selected->capacity) {
                selected->text = realloc(selected->text, selected->capacity * 2);
                selected->capacity *= 2;
            }
            selected->text[(selected->len)++] = input;
        }
        else if(mode == VISUAL_MODE) {
            if(input == '\e') { mode = NORMAL_MODE; }
            if(input == 'j') {
                // TODO: if an item with children gets selected, visually select all children
                if(!selected->next) continue;
                selectedIndex = getPlanItemIndex(root, selected->next);
                selected = selected->next;
                if(selectedIndex > visualSelectionEnd) visualSelectionEnd = selectedIndex;
            }
            if(input == 'k') {
                PlanItem *previous = getPreviousItemVisual(root, selected);
                if(!previous) continue;
                int previousIndex = getPlanItemIndex(root, previous);
                // bool isParent = previous->children == selected;
                selectedIndex = previousIndex;
                selected = previous;
                if(selectedIndex < visualSelectionStart) visualSelectionStart = selectedIndex;
            }
            if(input == 'd') {
                PlanItem *item = getPlanItemAtIndex(root, visualSelectionStart);
                PlanItem *previous = getPreviousItemVisual(root, item);

                PlanItem *removed = removeSelection(root, visualSelectionStart, visualSelectionEnd);
                freePlanItem(copyBuffer, true);
                copyBuffer = removed;
                mode = NORMAL_MODE;

                if(previous) {
                    selected = previous;
                    selectedIndex = getPlanItemIndex(root, previous);
                }
                else {
                    selectedIndex = 0;
                    selected = getPlanItemAtIndex(root, selectedIndex);
                    if(!selected) selected = root;
                }
            }
            // TODO: implement copy 'y'
        }
    }

    if(doSave) { 
        savefile = fopen("./planner.txt", "w+");
        writePlanItem(root, savefile);
        fclose(savefile);
    }

    fclose(logfile);

    printf("\e[?1049l"); // restore screen
    tcsetattr(STDIN_FILENO, TCSANOW, &restore);
    return 0;
}
