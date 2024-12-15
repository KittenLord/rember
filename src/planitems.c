#ifndef __REMBER_PLANITEMS_C
#define __REMBER_PLANITEMS_C

#include <stdio.h>
#include <stdlib.h>

typedef struct PlanItem PlanItem;

// TODO: implement auto-reset/remind timer with multiple options
// Option A: remind every N seconds (time_t)
// Option B: remind every selected weekday at particular time
// Option C: remind every selected month at every selected date at particular time
struct PlanItem {
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

/*

    File format spec (kinda)

    entry ::= (TextLength :: u8)
              (Text :: String[TextLength]) 
              (IsDone :: u8 (Bool))  
              maybeChild
              maybeNext
              (NO_ITEM :: u8)

    maybeChild ::= (CHILD_ITEM :: u8)
                   entry

                 | e

    maybeNext ::= (NEXT_ITEM :: u8)
                  entry

                | e

*/

#define NO_ITEM 0
#define NEXT_ITEM 1
#define CHILD_ITEM 2

// this will ABSOLUTELY explode if it encounters bad data btw
PlanItem *parsePlanItem(char **data) {
    size_t text_len = *((*data)++); // maybe change to 8 bytes?
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

// FIXME: for some reason after writing on windows this doesn't work
// the only idea I currently have is that when writing \n windows
// automatically writes \r\n instead (cuz I found 0x0D0A in xxd), but
// regardless, I'll look into this later
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

int __getPlanItemAmount(PlanItem *current) {
    if(!current) return 0;
    int amount = 1;
    amount += __getPlanItemAmount(current->children);
    amount += __getPlanItemAmount(current->next);
    return amount;
}

int getPlanItemAmount(PlanItem *root) {
    return __getPlanItemAmount(root) - 1;
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
    if((index == 0 || index == -1) && !root->children) return root;
    PlanItem *result = NULL;
    __getPlanItemAtIndex(root->children, &result, index, 0);
    return result;
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
    if(targetNext == root->children) return root;
    if(targetNext == root->next) return root;
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
    if(!base) return NULL;

    PlanItem *clone = malloc(sizeof(PlanItem));
    clone->len = base->len;
    clone->capacity = base->capacity;
    clone->text = malloc(clone->len);
    clone->done = base->done;
    clone->collapsed = false;
    memcpy(clone->text, base->text, clone->len);

    clone->children = NULL;
    clone->next = NULL;

    clone->children = planItemDeepCopy(base->children, true);
    if(cloneNext) clone->next = planItemDeepCopy(base->next, true);
    return clone;
}

#endif // __REMBER_PLANITEMS_C
