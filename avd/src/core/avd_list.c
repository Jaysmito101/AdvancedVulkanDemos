#include "core/avd_list.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void avdLisrtCreate(AVD_List *list, size_t itemSize)
{
    assert(list != NULL);
    assert(itemSize > 0);

    list->itemSize = itemSize;
    list->capacity = AVD_LIST_INITIAL_CAPACITY;
    list->count    = 0;
    list->items    = malloc(list->capacity * itemSize);
    assert(list->items != NULL);
}

void avdListDestroy(AVD_List *list)
{
    if (list && list->items) {
        free(list->items);
        list->items    = NULL;
        list->count    = 0;
        list->capacity = 0;
    }
}

void avdListEnsureCapacity(AVD_List *list, size_t newCapacity)
{
    assert(list != NULL);

    if (newCapacity <= list->capacity) {
        return;
    }

    size_t capacity = list->capacity;
    while (capacity < newCapacity) {
        capacity *= AVD_LIST_GROWTH_FACTOR;
    }

    void *newItems = realloc(list->items, capacity * list->itemSize);
    assert(newItems != NULL);

    list->items    = newItems;
    list->capacity = capacity;
}

void *avdListPushBack(AVD_List *list, const void *item)
{
    assert(list != NULL);
    assert(item != NULL);

    avdListEnsureCapacity(list, list->count + 1);

    void *dest = (char *)list->items + (list->count * list->itemSize);
    memcpy(dest, item, list->itemSize);
    list->count++;

    return dest;
}

void *avdListPushFront(AVD_List *list, const void *item)
{
    assert(list != NULL);
    assert(item != NULL);

    avdListEnsureCapacity(list, list->count + 1);

    // Shift all elements one position to the right
    if (list->count > 0) {
        memmove((char *)list->items + list->itemSize, list->items,
                list->count * list->itemSize);
    }

    memcpy(list->items, item, list->itemSize);
    list->count++;

    return list->items;
}

void *avdListPopBack(AVD_List *list)
{
    assert(list != NULL);

    if (list->count == 0) {
        return NULL;
    }

    list->count--;
    return (char *)list->items + (list->count * list->itemSize);
}

void *avdListPopFront(AVD_List *list)
{
    assert(list != NULL);

    if (list->count == 0) {
        return NULL;
    }

    void *front = list->items;

    // Shift all elements one position to the left
    if (list->count > 1) {
        memmove(list->items, (char *)list->items + list->itemSize,
                (list->count - 1) * list->itemSize);
    }

    list->count--;
    return front;
}

void *avdListGet(AVD_List *list, size_t index)
{
    assert(list != NULL);

    if (index >= list->count) {
        return NULL;
    }

    return (char *)list->items + (index * list->itemSize);
}

void avdListClear(AVD_List *list)
{
    assert(list != NULL);
    list->count = 0;
}

void avdListResize(AVD_List *list, size_t newSize)
{
    assert(list != NULL);

    if (newSize > list->capacity) {
        avdListEnsureCapacity(list, newSize);
    }

    // If growing, zero out new elements
    if (newSize > list->count) {
        void *newElements = (char *)list->items + (list->count * list->itemSize);
        memset(newElements, 0, (newSize - list->count) * list->itemSize);
    }

    list->count = newSize;
}

void avdListRemove(AVD_List *list, size_t index)
{
    assert(list != NULL);

    if (index >= list->count) {
        return;
    }

    // Shift elements after index to the left
    if (index < list->count - 1) {
        void *dest      = (char *)list->items + (index * list->itemSize);
        void *src       = (char *)list->items + ((index + 1) * list->itemSize);
        size_t moveSize = (list->count - index - 1) * list->itemSize;
        memmove(dest, src, moveSize);
    }

    list->count--;
}

void avdListInsert(AVD_List *list, size_t index, const void *item)
{
    assert(list != NULL);
    assert(item != NULL);

    if (index > list->count) {
        index = list->count;
    }

    avdListEnsureCapacity(list, list->count + 1);

    // Shift elements after index to the right
    if (index < list->count) {
        void *dest      = (char *)list->items + ((index + 1) * list->itemSize);
        void *src       = (char *)list->items + (index * list->itemSize);
        size_t moveSize = (list->count - index) * list->itemSize;
        memmove(dest, src, moveSize);
    }

    // Insert new item
    void *insertPos = (char *)list->items + (index * list->itemSize);
    memcpy(insertPos, item, list->itemSize);
    list->count++;
}

void avdListSort(AVD_List *list, int (*compare)(const void *, const void *))
{
    assert(list != NULL);
    assert(compare != NULL);

    if (list->count <= 1) {
        return;
    }

    qsort(list->items, list->count, list->itemSize, compare);
}

bool avdListIsEmpty(const AVD_List *list)
{
    assert(list != NULL);
    return list->count == 0;
}

bool avdListTighten(AVD_List *list)
{
    assert(list != NULL);

    if (list->count == 0) {
        free(list->items);
        list->items    = NULL;
        list->capacity = 0;
        return true;
    }

    void *newItems = realloc(list->items, list->count * list->itemSize);
    if (newItems == NULL) {
        return false; // Reallocation failed
    }

    list->items    = newItems;
    list->capacity = list->count; // Tighten capacity to current count
    return true;
}