#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <termios.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

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

int writeTerm(char *thing) {
    write(STDOUT_FILENO, thing, strlen(thing));
}

void setCursor(v2 p) {
    printf("\e[%d;%dH", p.row + 1, p.col + 1);
}

v2 getScreenRes() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return _v2(w.ws_col, w.ws_row);
}

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
    if(freeNext) freePlanItem(item->next, freeNext);
    freePlanItem(item->children, true);
    free(item);
}

#define NO_ITEM 0
#define NEXT_ITEM 1
#define CHILD_ITEM 2

PlanItem *parsePlanItem(char *data) {
    size_t text_len = *(data++);
    if(!text_len) return NULL;

    PlanItem *this = calloc(1, sizeof(PlanItem));

    this->text = malloc(text_len);
    this->len = text_len;
    this->capacity = text_len;
    memcpy(this->text, data, text_len);
    data += text_len;

    this->done = !!*(data++);
    char control = *(data++);

    if(control == NO_ITEM) return this;
    if(control == CHILD_ITEM) this->children = parsePlanItem(data);
    if(control == NEXT_ITEM) this->next = parsePlanItem(data);
    return this;
}

void writePlanItem(PlanItem *item, FILE *file) {
    char len = *(char *)(&item->len);
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

int renderPlanItem(PlanItem *item, int indent, int index, int selectedIndex, bool init) {
    if(init) { writeTerm("\e[2J"); setCursor(v20); renderPlanItem(item->children, 0, 0, selectedIndex, false); return 0; }

    for(int i = 0; i < indent; i++) putchar(' ');

    if(index == selectedIndex) {
        writeTerm("\e[38;5;0m");
        writeTerm("\e[48;5;255m");
    }

    if(item->children) {
        int children = getAmountOfChildren(item);
        int doneChildren = getAmountOfDoneChildren(item);

        printf("- [%d/%d] ", doneChildren, children);
    }
    else {
        char x = item->done ? 'x' : ' ';
        printf("- [%c] ", x);
    }

    write(STDOUT_FILENO, item->text, item->len);

    if(index == selectedIndex) {
        writeTerm("\e[0m");
    }

    writeTerm("\e[1E"); // new line

    if(item->children) { index = renderPlanItem(item->children, indent + 4, index + 1, selectedIndex, false); }
    if(item->next) { index = renderPlanItem(item->next, indent, index + 1, selectedIndex, false); }
    return index;
}

int __getPlanItemAtIndex(PlanItem *current, PlanItem **result, int index, int currentIndex) {
    if(!current) return 0;
    if(currentIndex == index) { *result = current; return 0; }
    if(current->children) currentIndex = __getPlanItemAtIndex(current->children, result, index, currentIndex + 1);
    if(*result) return 0;
    if(current->next) currentIndex = __getPlanItemAtIndex(current->next, result, index, currentIndex + 1);
    return 0;
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

int main(int argc, char **argv) {
    struct termios term, restore;
    tcgetattr(STDIN_FILENO, &term);
    tcgetattr(STDIN_FILENO, &restore);
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
    setbuf(stdout, NULL);

    writeTerm("\e[?1049h"); // save previous screen
    writeTerm("\e[?25l"); // make cursor invisible

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
        root = parsePlanItem(buffer);
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
    }

    PlanItem *selected = root;
    int selectedIndex = 0;

    char input;
    char buf[8];

    #define NORMAL_MODE 0
    #define INSERT_MODE 1

    int mode = NORMAL_MODE;

    while(true) { 
        read(STDIN_FILENO, &input, 1);

        ctob(input, buf);
        fwrite(buf, sizeof(char), 8, logfile);
        fwrite("\n", sizeof(char), 1, logfile);

        if(mode == NORMAL_MODE) {
            if(input == 'q') break;
            if(input == 'o' && selected == root) input = 'a';
            if(input == 'a') {
                PlanItem *child = calloc(1, sizeof(PlanItem));
                child->text = calloc(16, sizeof(char));
                child->capacity = 16;

                child->children = selected->children;
                selected->children = child;
                selected = child;
                int amount = getPlanItemAmount(root) - 1; // -1 to account for root
                selectedIndex = clamp(selectedIndex + 1, 0, amount);
                mode = INSERT_MODE;
            }
            if(input == 'o') {
                PlanItem *nnext = calloc(1, sizeof(PlanItem));
                nnext->text = calloc(16, sizeof(char));
                nnext->capacity = 16;

                nnext->next = selected->next;
                selected->next = nnext;
                selected = nnext;
                // TODO: Obviously do smth about copypaste, but
                // also this doesn't work if the node has children
                int amount = getPlanItemAmount(root) - 1; // -1 to account for root
                selectedIndex = clamp(selectedIndex + 1, 0, amount);
                mode = INSERT_MODE;
            }
            if(input == 'j') {
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
            if(input == 0x0A) {
                selected->done = !selected->done;
            }
        }
        else if(mode == INSERT_MODE) {
            if(input == 'q') break;
            if(input == 0x0A) { if(selected->text > 0) { mode = NORMAL_MODE; } continue; }
            if(input == 0x7F) {
                if(selected->len <= 0) continue;
                selected->len--;
            }
            else if(selected->len >= selected->capacity) {
                selected->text = realloc(selected->text, selected->capacity * 2);
                selected->capacity *= 2;
            }
            selected->text[(selected->len)++] = input;
        }

        renderPlanItem(root, 0, 0, selectedIndex, true);
    }

    // writePlanItem(root, savefile);

    fclose(logfile);
    fclose(savefile);

    writeTerm("\e[?1049l"); // restore screen
    tcsetattr(STDIN_FILENO, TCSANOW, &restore);
    return 0;
}
