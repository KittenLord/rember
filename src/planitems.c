#ifndef __REMBER_PLANITEMS_C
#define __REMBER_PLANITEMS_C

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>

#define RemindNone 0
#define RemindEvery 1
#define RemindWeekday 2
#define RemindMonthday 3

// not sure if this is needed
#define W_SUN_BIT 0b00000001
#define W_MON_BIT 0b00000010
#define W_TUE_BIT 0b00000100
#define W_WED_BIT 0b00001000
#define W_THU_BIT 0b00010000
#define W_FRI_BIT 0b00100000
#define W_SAT_BIT 0b01000000

#define M_JAN_BIT 0b000000000001
#define M_FEB_BIT 0b000000000010
#define M_MAR_BIT 0b000000000100
#define M_APR_BIT 0b000000001000
#define M_MAY_BIT 0b000000010000
#define M_JUN_BIT 0b000000100000
#define M_JUL_BIT 0b000001000000
#define M_AUG_BIT 0b000010000000
#define M_SEP_BIT 0b000100000000
#define M_OCT_BIT 0b001000000000
#define M_NOV_BIT 0b010000000000
#define M_DEC_BIT 0b100000000000

typedef char RemindType;

typedef struct {
    RemindType type;
    time_t lastChecked;

    union {
        struct {
            time_t offset;
            time_t delay;
        } every;

        struct {
            char weekdayBitmap;
            time_t timeOfDay;
        } weekday;

        struct {
            uint32_t monthdayBitmap;
            uint16_t monthBitmap;
            time_t timeOfDay;
        } monthday;
    };
} RemindTimeInfo;

typedef struct PlanItem PlanItem;

// TODO: implement auto-reset/remind timer with multiple options
// Option A: remind every N seconds (time_t)
// Option B: remind every selected weekday at particular time
// Option C: remind every selected month at every selected date at particular time
struct PlanItem {
    // Serialized
    size_t len;
    char *text;
    bool done;
    RemindTimeInfo remindInfo;
    PlanItem *children;
    PlanItem *next;

    // Runtime
    size_t capacity;
    bool collapsed;
    time_t lastCheckedStaged;
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
              maybeTime
              maybeChild
              maybeNext
              (NO_ITEM :: u8)

    maybeTime ::= (TIME_FLAG :: u8)
                  time

                | e

    maybeChild ::= (CHILD_ITEM :: u8)
                   entry

                 | e

    maybeNext ::= (NEXT_ITEM :: u8)
                  entry

                | e

*/

#define NO_ITEM 0x00
#define NEXT_ITEM 0x01
#define CHILD_ITEM 0x02

#define TIME_FLAG 0x0A

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
    #define ifControl(flag, block) if(control == flag) { block; control = *((*data)++); }

    // ifControl(NO_ITEM, { return this; });
    ifControl(TIME_FLAG, {
        memcpy(&(this->remindInfo), *data, sizeof(RemindTimeInfo));
        *data += sizeof(RemindTimeInfo);
        this->lastCheckedStaged = this->remindInfo.lastChecked;
    });
    ifControl(CHILD_ITEM, { this->children = parsePlanItem(data); });
    ifControl(NEXT_ITEM, { this->next = parsePlanItem(data); });

    assert(control == NO_ITEM);
    return this;
}

// FIXME: for some reason after writing on windows this doesn't work
// the only idea I currently have is that when writing \n windows
// automatically writes \r\n instead (cuz I found 0x0D0A in xxd), but
// regardless, I'll look into this later
void writePlanItem(PlanItem *item, FILE *file) {
    // Commit all temporary changes
    item->remindInfo.lastChecked = item->lastCheckedStaged;

    char len = *(char *)(&(item->len));
    fwrite(&len, sizeof(char), 1, file);
    fwrite(item->text, sizeof(char), len, file);
    fwrite(&item->done, sizeof(char), 1, file);

    // I wonder if there's a better solution to writing a constant to a file
    char noItem = NO_ITEM;
    char nextItem = NEXT_ITEM;
    char childItem = CHILD_ITEM;
    char timeFlag = TIME_FLAG;

    if(item->remindInfo.type != RemindNone) { fwrite(&timeFlag, sizeof(char), 1, file); fwrite(&item->remindInfo, sizeof(RemindTimeInfo), 1, file); }
    if(item->children) { fwrite(&childItem, sizeof(char), 1, file); writePlanItem(item->children, file); }
    if(item->next) { fwrite(&nextItem, sizeof(char), 1, file); writePlanItem(item->next, file); }
    fwrite(&noItem, sizeof(char), 1, file);
    return;
}

bool isExpired(PlanItem *item, time_t now) {
    struct tm *date;
    time_t t;

    if(item->remindInfo.type == RemindNone) return false;
    else if(item->remindInfo.type == RemindEvery) {
        return (now - item->remindInfo.lastChecked) > item->remindInfo.every.delay;
    }
    else if(item->remindInfo.type == RemindWeekday) {

    }
    else if(item->remindInfo.type == RemindMonthday) {

    }

    return false;
}

// I HECKIN LOVE RECURSION!!!
void foreachPlanItem(PlanItem *item, void (*fn)(PlanItem *, void *), void *data) {
    if(!item) return;
    fn(item, data);
    foreachPlanItem(item->children, fn, data);
    foreachPlanItem(item->next, fn, data);
}

void foreachChildPlanItem(PlanItem *item, void (*fn)(PlanItem *, void *), void *data) {
    if(!item) return;
    foreachPlanItem(item->children, fn, data);
}

void foreachNextPlanItem(PlanItem *item, void (*fn)(PlanItem *, void *), void *data) {
    if(!item) return;
    fn(item, data);
    foreachNextPlanItem(item->next, fn, data);
}

void foreachDirectChildPlanItem(PlanItem *item, void (*fn)(PlanItem *, void *), void *data) {
    if(!item) return;
    foreachNextPlanItem(item->children, fn, data);
}

PlanItem *findPlanItem(PlanItem *item, bool (*pred)(PlanItem *, void *), void *data) {
    if(!item) return NULL;
    if(pred(item, data)) return item;

    PlanItem *result = NULL;
    result = findPlanItem(item->children, pred, data);
    if(!result) result = findPlanItem(item->next, pred, data);
    return result;
}

bool isDone(PlanItem *item);

void fe_constant(PlanItem *item, void *amount) { item = item; (*(int *)amount)++; }
void fe_ifDone(PlanItem *item, void *amount) { if(isDone(item)) (*(int *)amount)++; }

int getAmountOfChildren(PlanItem *item) {
    int amount = 0;
    foreachDirectChildPlanItem(item, fe_constant, &amount);
    return amount;
}

int getTotalAmount(PlanItem *root) {
    int amount = 0;
    foreachChildPlanItem(root, fe_constant, &amount);
    return amount;
}

int getAmountOfDoneChildren(PlanItem *item) {
    int amount = 0;
    foreachDirectChildPlanItem(item, fe_ifDone, &amount);
    return amount;
}

bool isDone(PlanItem *item) {
    if(!item->children) return item->done;
    int children = getAmountOfChildren(item);
    int doneChildren = getAmountOfDoneChildren(item);
    return doneChildren >= children;
}

bool find_isBefore(PlanItem *item, void *vtarget) { 
    PlanItem *target = vtarget;
    return item->children == target || item->next == target;
}

PlanItem *getItemBefore(PlanItem *root, PlanItem *target) {
    return findPlanItem(root, find_isBefore, target);
}

PlanItem *getFirstItemSafe(PlanItem *root) {
    if(!root) return NULL;
    if(root->children) return root->children;
    return root;
}

PlanItem *getLastSameNest(PlanItem *item) {
    if(!item) return item;
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

int getIndexOfLastChild(PlanItem *root, PlanItem *item) {
    if(item && item->children) item = getLastSameNest(item->children);
    return getPlanItemIndex(root, item);
}

PlanItem *removeSelection(PlanItem *root, int vStart, int vEnd) {
    PlanItem *item = getPlanItemAtIndex(root, vStart);
    PlanItem *search = item;

    PlanItem *parent = getItemBefore(root, item);

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

#endif // __REMBER_PLANITEMS_C
