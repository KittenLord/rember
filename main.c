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

int main(int argc, char **argv) {
    struct termios term, restore;
    tcgetattr(STDIN_FILENO, &term);
    tcgetattr(STDIN_FILENO, &restore);
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
    setbuf(stdout, NULL);

    writeTerm("\e[?1049h");

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
        root = calloc(1, sizeof(PlanItem));
        root->text = malloc(4);
        root->len = 4;
        memcpy(root->text, "ROOT", 4);
    }

    char input;
    char buf[8];
    while(true) { 
        read(STDIN_FILENO, &input, 1);

        ctob(input, buf);
        fwrite(buf, sizeof(char), 8, logfile);
        fwrite("\n", sizeof(char), 1, logfile);

        if(input == 'q') break;
    }

    writePlanItem(root, savefile);

    fclose(logfile);
    fclose(savefile);

    writeTerm("\e[?1049l");
    tcsetattr(STDIN_FILENO, TCSANOW, &restore);
    return 0;
}
