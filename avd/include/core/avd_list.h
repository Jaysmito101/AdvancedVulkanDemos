#ifndef AVD_LIST_H
#define AVD_LIST_H

#include "core/avd_base.h"


#ifndef AVD_LIST_INITIAL_CAPACITY
#define AVD_LIST_INITIAL_CAPACITY 8
#endif 

#ifndef AVD_LIST_GROWTH_FACTOR
#define AVD_LIST_GROWTH_FACTOR 2
#endif 

typedef struct {
    size_t itemSize;
    size_t capacity;
    size_t count;
    void *items;
} AVD_List;

void avdLisrtCreate(AVD_List *list, size_t itemSize);
void avdListDestroy(AVD_List *list);
void *avdListPushBack(AVD_List *list, const void *item);
void *avdListPushFront(AVD_List *list, const void *item);
void *avdListPopBack(AVD_List *list);
void *avdListPopFront(AVD_List *list);
void *avdListGet(AVD_List *list, size_t index);
void avdListClear(AVD_List *list);
void avdListResize(AVD_List *list, size_t newSize);
void avdListEnsureCapacity(AVD_List *list, size_t newCapacity);
void avdListRemove(AVD_List *list, size_t index);
void avdListInsert(AVD_List *list, size_t index, const void *item);
void avdListSort(AVD_List *list, int (*compare)(const void *, const void *));
bool avdListIsEmpty(const AVD_List *list);
bool avdListTighten(AVD_List *list);

bool avdListTestsRun(void);

#endif