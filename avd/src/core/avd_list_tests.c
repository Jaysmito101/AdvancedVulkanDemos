#include "core/avd_core.h"
#include <string.h>

static bool __avdTestListInit()
{
    AVD_LOG("  Testing List initialization...\n");

    AVD_List list;
    avdListCreate(&list, sizeof(int));

    if (list.itemSize != sizeof(int) ||
        list.count != 0 ||
        list.capacity != AVD_LIST_INITIAL_CAPACITY ||
        list.items == NULL) {
        AVD_LOG("    FAILED: List initialization\n");
        avdListDestroy(&list);
        return false;
    }

    avdListDestroy(&list);
    AVD_LOG("    List initialization PASSED\n");
    return true;
}

static bool __avdTestListPushBack()
{
    AVD_LOG("  Testing List push back operations...\n");

    AVD_List list;
    avdListCreate(&list, sizeof(int));

    // Test pushing single element
    int value1   = 42;
    void *result = avdListPushBack(&list, &value1);

    if (list.count != 1 || result == NULL) {
        AVD_LOG("    FAILED: Push back single element\n");
        avdListDestroy(&list);
        return false;
    }

    int *storedValue = (int *)avdListGet(&list, 0);
    if (storedValue == NULL || *storedValue != 42) {
        AVD_LOG("    FAILED: Push back value verification\n");
        avdListDestroy(&list);
        return false;
    }

    // Test pushing multiple elements to trigger capacity growth
    for (int i = 1; i < 20; i++) {
        int value = i * 10;
        avdListPushBack(&list, &value);
    }

    if (list.count != 20 || list.capacity < 20) {
        AVD_LOG("    FAILED: Push back capacity growth\n");
        avdListDestroy(&list);
        return false;
    }

    // Verify all values
    for (int i = 0; i < 20; i++) {
        int *value   = (int *)avdListGet(&list, i);
        int expected = (i == 0) ? 42 : i * 10;
        if (value == NULL || *value != expected) {
            AVD_LOG("    FAILED: Push back value verification at index %d\n", i);
            avdListDestroy(&list);
            return false;
        }
    }

    avdListDestroy(&list);
    AVD_LOG("    List push back PASSED\n");
    return true;
}

static bool __avdTestListPushFront()
{
    AVD_LOG("  Testing List push front operations...\n");

    AVD_List list;
    avdListCreate(&list, sizeof(int));

    // Push elements to front
    for (int i = 0; i < 5; i++) {
        int value = i + 1;
        avdListPushFront(&list, &value);
    }

    if (list.count != 5) {
        AVD_LOG("    FAILED: Push front count\n");
        avdListDestroy(&list);
        return false;
    }

    // Verify order (should be reversed)
    for (int i = 0; i < 5; i++) {
        int *value   = (int *)avdListGet(&list, i);
        int expected = 5 - i;
        if (value == NULL || *value != expected) {
            AVD_LOG("    FAILED: Push front order verification\n");
            avdListDestroy(&list);
            return false;
        }
    }

    avdListDestroy(&list);
    AVD_LOG("    List push front PASSED\n");
    return true;
}

static bool __avdTestListPopOperations()
{
    AVD_LOG("  Testing List pop operations...\n");

    AVD_List list;
    avdListCreate(&list, sizeof(int));

    // Test pop on empty list
    void *result = avdListPopBack(&list);
    if (result != NULL) {
        AVD_LOG("    FAILED: Pop back on empty list should return NULL\n");
        avdListDestroy(&list);
        return false;
    }

    result = avdListPopFront(&list);
    if (result != NULL) {
        AVD_LOG("    FAILED: Pop front on empty list should return NULL\n");
        avdListDestroy(&list);
        return false;
    }

    // Add elements
    for (int i = 1; i <= 5; i++) {
        avdListPushBack(&list, &i);
    }

    // Test pop back
    result = avdListPopBack(&list);
    if (result == NULL || list.count != 4) {
        AVD_LOG("    FAILED: Pop back operation\n");
        avdListDestroy(&list);
        return false;
    }

    // Test pop front
    result = avdListPopFront(&list);
    if (result == NULL || list.count != 3) {
        AVD_LOG("    FAILED: Pop front operation\n");
        avdListDestroy(&list);
        return false;
    }

    // Verify remaining elements [2, 3, 4]
    for (int i = 0; i < 3; i++) {
        int *value   = (int *)avdListGet(&list, i);
        int expected = i + 2;
        if (value == NULL || *value != expected) {
            AVD_LOG("    FAILED: Pop operations verification\n");
            avdListDestroy(&list);
            return false;
        }
    }

    avdListDestroy(&list);
    AVD_LOG("    List pop operations PASSED\n");
    return true;
}

static bool __avdTestListInsertRemove()
{
    AVD_LOG("  Testing List insert/remove operations...\n");

    AVD_List list;
    avdListCreate(&list, sizeof(int));

    // Add initial elements [1, 2, 3, 4, 5]
    for (int i = 1; i <= 5; i++) {
        avdListPushBack(&list, &i);
    }

    // Test insert at beginning
    int value = 0;
    avdListInsert(&list, 0, &value);
    if (list.count != 6) {
        AVD_LOG("    FAILED: Insert at beginning count\n");
        avdListDestroy(&list);
        return false;
    }

    // Test insert at middle
    value = 99;
    avdListInsert(&list, 3, &value);
    if (list.count != 7) {
        AVD_LOG("    FAILED: Insert at middle count\n");
        avdListDestroy(&list);
        return false;
    }

    // Test insert at end (beyond count)
    value = 100;
    avdListInsert(&list, 1000, &value);
    if (list.count != 8) {
        AVD_LOG("    FAILED: Insert beyond end count\n");
        avdListDestroy(&list);
        return false;
    }

    // Verify list: [0, 1, 2, 99, 3, 4, 5, 100]
    int expected[] = {0, 1, 2, 99, 3, 4, 5, 100};
    for (int i = 0; i < 8; i++) {
        int *value = (int *)avdListGet(&list, i);
        if (value == NULL || *value != expected[i]) {
            AVD_LOG("    FAILED: Insert verification at index %d\n", i);
            avdListDestroy(&list);
            return false;
        }
    }

    // Test remove operations
    avdListRemove(&list, 0);    // Remove first element
    avdListRemove(&list, 2);    // Remove 99
    avdListRemove(&list, 1000); // Remove beyond bounds (should do nothing)

    if (list.count != 6) {
        AVD_LOG("    FAILED: Remove operations count\n");
        avdListDestroy(&list);
        return false;
    }

    // Verify list: [1, 2, 3, 4, 5, 100]
    int expectedAfterRemove[] = {1, 2, 3, 4, 5, 100};
    for (int i = 0; i < 6; i++) {
        int *value = (int *)avdListGet(&list, i);
        if (value == NULL || *value != expectedAfterRemove[i]) {
            AVD_LOG("    FAILED: Remove verification at index %d\n", i);
            avdListDestroy(&list);
            return false;
        }
    }

    avdListDestroy(&list);
    AVD_LOG("    List insert/remove operations PASSED\n");
    return true;
}

static int __compareInts(const void *a, const void *b)
{
    int intA = *(const int *)a;
    int intB = *(const int *)b;
    return (intA > intB) - (intA < intB);
}

static bool __avdTestListSort()
{
    AVD_LOG("  Testing List sort operations...\n");

    AVD_List list;
    avdListCreate(&list, sizeof(int));

    // Test sort on empty list
    avdListSort(&list, __compareInts);
    if (list.count != 0) {
        AVD_LOG("    FAILED: Sort empty list\n");
        avdListDestroy(&list);
        return false;
    }

    // Test sort on single element
    int value = 42;
    avdListPushBack(&list, &value);
    avdListSort(&list, __compareInts);
    if (list.count != 1 || *(int *)avdListGet(&list, 0) != 42) {
        AVD_LOG("    FAILED: Sort single element\n");
        avdListDestroy(&list);
        return false;
    }

    avdListClear(&list);

    // Test sort on multiple elements
    int unsorted[] = {5, 2, 8, 1, 9, 3};
    int sorted[]   = {1, 2, 3, 5, 8, 9};

    for (int i = 0; i < 6; i++) {
        avdListPushBack(&list, &unsorted[i]);
    }

    avdListSort(&list, __compareInts);

    for (int i = 0; i < 6; i++) {
        int *value = (int *)avdListGet(&list, i);
        if (value == NULL || *value != sorted[i]) {
            AVD_LOG("    FAILED: Sort verification at index %d\n", i);
            avdListDestroy(&list);
            return false;
        }
    }

    avdListDestroy(&list);
    AVD_LOG("    List sort operations PASSED\n");
    return true;
}

static bool __avdTestListUtilities()
{
    AVD_LOG("  Testing List utility functions...\n");

    AVD_List list;
    avdListCreate(&list, sizeof(int));

    // Test isEmpty on empty list
    if (!avdListIsEmpty(&list)) {
        AVD_LOG("    FAILED: isEmpty on empty list\n");
        avdListDestroy(&list);
        return false;
    }

    // Add element and test isEmpty
    int value = 1;
    avdListPushBack(&list, &value);
    if (avdListIsEmpty(&list)) {
        AVD_LOG("    FAILED: isEmpty on non-empty list\n");
        avdListDestroy(&list);
        return false;
    }

    // Test clear
    avdListClear(&list);
    if (list.count != 0 || !avdListIsEmpty(&list)) {
        AVD_LOG("    FAILED: Clear operation\n");
        avdListDestroy(&list);
        return false;
    }

    // Test resize - grow
    avdListResize(&list, 10);
    if (list.count != 10 || list.capacity < 10) {
        AVD_LOG("    FAILED: Resize grow operation\n");
        avdListDestroy(&list);
        return false;
    }

    // Verify new elements are zeroed
    for (int i = 0; i < 10; i++) {
        int *value = (int *)avdListGet(&list, i);
        if (value == NULL || *value != 0) {
            AVD_LOG("    FAILED: Resize zero initialization\n");
            avdListDestroy(&list);
            return false;
        }
    }

    // Test resize - shrink
    avdListResize(&list, 5);
    if (list.count != 5) {
        AVD_LOG("    FAILED: Resize shrink operation\n");
        avdListDestroy(&list);
        return false;
    }

    // Test tighten
    size_t oldCapacity = list.capacity;
    bool tightenResult = avdListTighten(&list);
    if (!tightenResult || list.capacity != list.count) {
        AVD_LOG("    FAILED: Tighten operation\n");
        avdListDestroy(&list);
        return false;
    }

    // Test tighten on empty list
    avdListClear(&list);
    tightenResult = avdListTighten(&list);
    if (!tightenResult || list.capacity != 0 || list.items != NULL) {
        AVD_LOG("    FAILED: Tighten empty list operation\n");
        avdListDestroy(&list);
        return false;
    }

    avdListDestroy(&list);
    AVD_LOG("    List utility functions PASSED\n");
    return true;
}

static bool __avdTestListWithStrings()
{
    AVD_LOG("  Testing List with string data...\n");

    AVD_List list;
    avdListCreate(&list, sizeof(char[32]));

    // Add string data
    char str1[32] = "Hello";
    char str2[32] = "World";
    char str3[32] = "Test";

    avdListPushBack(&list, str1);
    avdListPushBack(&list, str2);
    avdListPushBack(&list, str3);

    if (list.count != 3) {
        AVD_LOG("    FAILED: String list count\n");
        avdListDestroy(&list);
        return false;
    }

    // Verify string data
    char *stored1 = (char *)avdListGet(&list, 0);
    char *stored2 = (char *)avdListGet(&list, 1);
    char *stored3 = (char *)avdListGet(&list, 2);

    if (strcmp(stored1, "Hello") != 0 ||
        strcmp(stored2, "World") != 0 ||
        strcmp(stored3, "Test") != 0) {
        AVD_LOG("    FAILED: String data verification\n");
        avdListDestroy(&list);
        return false;
    }

    avdListDestroy(&list);
    AVD_LOG("    List with string data PASSED\n");
    return true;
}

static bool __avdTestListEdgeCases()
{
    AVD_LOG("  Testing List edge cases...\n");

    AVD_List list;
    avdListCreate(&list, sizeof(int));

    // Test get with invalid index
    void *result = avdListGet(&list, 0);
    if (result != NULL) {
        AVD_LOG("    FAILED: Get invalid index on empty list\n");
        avdListDestroy(&list);
        return false;
    }

    // Add element and test invalid indices
    int value = 42;
    avdListPushBack(&list, &value);

    result = avdListGet(&list, 1);
    if (result != NULL) {
        AVD_LOG("    FAILED: Get invalid index beyond bounds\n");
        avdListDestroy(&list);
        return false;
    }

    result = avdListGet(&list, SIZE_MAX);
    if (result != NULL) {
        AVD_LOG("    FAILED: Get with very large index\n");
        avdListDestroy(&list);
        return false;
    }

    // Test remove with invalid index
    size_t oldCount = list.count;
    avdListRemove(&list, 100);
    if (list.count != oldCount) {
        AVD_LOG("    FAILED: Remove invalid index should not change count\n");
        avdListDestroy(&list);
        return false;
    }

    avdListDestroy(&list);
    AVD_LOG("    List edge cases PASSED\n");
    return true;
}

static int destructorCallCount           = 0;
static int *destructorCalledValues       = NULL;
static size_t destructorCalledValuesSize = 0;

static void __testDestructor(void *item, void *context)
{
    int *value           = (int *)item;
    int *expectedContext = (int *)context;

    if (destructorCalledValues == NULL) {
        destructorCalledValues     = malloc(sizeof(int) * 100);
        destructorCalledValuesSize = 0;
    }

    if (destructorCalledValuesSize < 100) {
        destructorCalledValues[destructorCalledValuesSize++] = *value;
    }

    destructorCallCount++;

    if (expectedContext != NULL && *expectedContext != 42) {
        AVD_LOG("    WARNING: Destructor context mismatch\n");
    }
}

static void __resetDestructorTracking()
{
    destructorCallCount = 0;
    if (destructorCalledValues) {
        free(destructorCalledValues);
        destructorCalledValues = NULL;
    }
    destructorCalledValuesSize = 0;
}

static bool __avdTestListDestructor()
{
    AVD_LOG("  Testing List destructor functionality...\n");

    __resetDestructorTracking();

    AVD_List list;
    avdListCreate(&list, sizeof(int));

    int contextValue = 42;
    avdListSetDestructor(&list, __testDestructor, &contextValue);

    // Test destructor is not called when list is empty
    avdListDestroy(&list);
    if (destructorCallCount != 0) {
        AVD_LOG("    FAILED: Destructor called on empty list\n");
        __resetDestructorTracking();
        return false;
    }

    // Recreate list for next tests
    avdListCreate(&list, sizeof(int));
    avdListSetDestructor(&list, __testDestructor, &contextValue);

    // Add elements and test destructor on destroy
    for (int i = 1; i <= 5; i++) {
        avdListPushBack(&list, &i);
    }

    avdListDestroy(&list);

    if (destructorCallCount != 5) {
        AVD_LOG("    FAILED: Destructor call count on destroy (expected 5, got %d)\n", destructorCallCount);
        __resetDestructorTracking();
        return false;
    }

    // Verify destructor was called with correct values
    for (size_t i = 0; i < destructorCalledValuesSize; i++) {
        int expectedValue = (int)(i + 1);
        if (destructorCalledValues[i] != expectedValue) {
            AVD_LOG("    FAILED: Destructor called with wrong value at index %zu\n", i);
            __resetDestructorTracking();
            return false;
        }
    }

    __resetDestructorTracking();

    // Test destructor on clear
    avdListCreate(&list, sizeof(int));
    avdListSetDestructor(&list, __testDestructor, &contextValue);

    for (int i = 10; i <= 13; i++) {
        avdListPushBack(&list, &i);
    }

    avdListClear(&list);

    if (destructorCallCount != 4) {
        AVD_LOG("    FAILED: Destructor call count on clear (expected 4, got %d)\n", destructorCallCount);
        avdListDestroy(&list);
        __resetDestructorTracking();
        return false;
    }

    __resetDestructorTracking();

    // Test destructor on remove
    for (int i = 20; i <= 22; i++) {
        avdListPushBack(&list, &i);
    }

    avdListRemove(&list, 1); // Remove middle element (21)

    if (destructorCallCount != 1) {
        AVD_LOG("    FAILED: Destructor call count on remove (expected 1, got %d)\n", destructorCallCount);
        avdListDestroy(&list);
        __resetDestructorTracking();
        return false;
    }

    if (destructorCalledValues[0] != 21) {
        AVD_LOG("    FAILED: Destructor called with wrong value on remove\n");
        avdListDestroy(&list);
        __resetDestructorTracking();
        return false;
    }

    __resetDestructorTracking();

    // Test that destructor is NOT called on pop operations
    void *popped = avdListPopBack(&list);
    if (destructorCallCount != 0 || popped == NULL) {
        AVD_LOG("    FAILED: Destructor should not be called on pop operations\n");
        avdListDestroy(&list);
        __resetDestructorTracking();
        return false;
    }

    popped = avdListPopFront(&list);
    if (destructorCallCount != 0 || popped == NULL) {
        AVD_LOG("    FAILED: Destructor should not be called on pop front operations\n");
        avdListDestroy(&list);
        __resetDestructorTracking();
        return false;
    }

    // Test destructor with NULL context
    avdListSetDestructor(&list, __testDestructor, NULL);
    int value = 99;
    avdListPushBack(&list, &value);
    avdListClear(&list);

    if (destructorCallCount != 1) {
        AVD_LOG("    FAILED: Destructor with NULL context\n");
        avdListDestroy(&list);
        __resetDestructorTracking();
        return false;
    }

    avdListDestroy(&list);
    __resetDestructorTracking();

    AVD_LOG("    List destructor functionality PASSED\n");
    return true;
}

bool avdListTestsRun(void)
{
    AVD_LOG("Running AVD List Tests...\n");

    AVD_CHECK(__avdTestListInit());
    AVD_CHECK(__avdTestListPushBack());
    AVD_CHECK(__avdTestListPushFront());
    AVD_CHECK(__avdTestListPopOperations());
    AVD_CHECK(__avdTestListInsertRemove());
    AVD_CHECK(__avdTestListSort());
    AVD_CHECK(__avdTestListUtilities());
    AVD_CHECK(__avdTestListWithStrings());
    AVD_CHECK(__avdTestListEdgeCases());
    AVD_CHECK(__avdTestListDestructor());

    AVD_LOG("All AVD List Tests passed successfully!\n");
    return true;
}