#include "core/avd_core.h"
#include "core/avd_hashtable.h"
#include <string.h>

static bool __avdTestHashTableInit()
{
    AVD_LOG_DEBUG("  Testing HashTable initialization...");

    AVD_HashTable table;

    // Test basic initialization
    bool result = avdHashTableCreate(&table, sizeof(int), sizeof(int), 0, false);
    if (!result) {
        AVD_LOG_ERROR("    ERROR: Failed to create hash table");
        return false;
    }

    if (table.keySize != sizeof(int) || table.itemSize != sizeof(int) ||
        table.stringKey != false || table.count != 0) {
        AVD_LOG_ERROR("    ERROR: Hash table not initialized correctly");
        avdHashTableDestroy(&table);
        return false;
    }

    // Test with initial capacity
    avdHashTableDestroy(&table);
    result = avdHashTableCreate(&table, sizeof(int), sizeof(float), 100, false);
    if (!result || table.bankCount == 0) {
        AVD_LOG_ERROR("    ERROR: Failed to create hash table with initial capacity");
        return false;
    }

    // Test string key initialization
    avdHashTableDestroy(&table);
    result = avdHashTableCreate(&table, 0, sizeof(int), 0, true);
    if (!result || !table.stringKey) {
        AVD_LOG_ERROR("    ERROR: Failed to create string key hash table");
        return false;
    }

    avdHashTableDestroy(&table);
    AVD_LOG_DEBUG("    HashTable initialization PASSED");
    return true;
}

static bool __avdTestHashTableSetGet()
{
    AVD_LOG_DEBUG("  Testing HashTable set/get operations...");

    AVD_HashTable table;
    avdHashTableCreate(&table, sizeof(int), sizeof(int), 0, false);

    // Test basic set/get
    int key1    = 42;
    int value1  = 100;
    bool result = avdHashTableSet(&table, &key1, &value1);
    if (!result) {
        AVD_LOG_ERROR("    ERROR: Failed to set value in hash table");
        avdHashTableDestroy(&table);
        return false;
    }

    int retrievedValue;
    result = avdHashTableGet(&table, &key1, &retrievedValue);
    if (!result || retrievedValue != value1) {
        AVD_LOG_ERROR("    ERROR: Failed to get value from hash table");
        avdHashTableDestroy(&table);
        return false;
    }

    // Test updating existing key
    int newValue = 200;
    result       = avdHashTableSet(&table, &key1, &newValue);
    if (!result) {
        AVD_LOG_ERROR("    ERROR: Failed to update existing key");
        avdHashTableDestroy(&table);
        return false;
    }

    result = avdHashTableGet(&table, &key1, &retrievedValue);
    if (!result || retrievedValue != newValue) {
        AVD_LOG_ERROR("    ERROR: Updated value not retrieved correctly");
        avdHashTableDestroy(&table);
        return false;
    }

    // Test multiple key-value pairs
    for (int i = 0; i < 50; i++) {
        int key   = i;
        int value = i * 10;
        result    = avdHashTableSet(&table, &key, &value);
        if (!result) {
            AVD_LOG_ERROR("    ERROR: Failed to set multiple values");
            avdHashTableDestroy(&table);
            return false;
        }
    }

    // Verify all values
    for (int i = 0; i < 50; i++) {
        int key = i;
        int value;
        result = avdHashTableGet(&table, &key, &value);
        if (!result || value != i * 10) {
            AVD_LOG_ERROR("    ERROR: Failed to retrieve multiple values correctly");
            avdHashTableDestroy(&table);
            return false;
        }
    }

    avdHashTableDestroy(&table);
    AVD_LOG_DEBUG("    HashTable set/get operations PASSED");
    return true;
}

static bool __avdTestHashTableStringKeys()
{
    AVD_LOG_DEBUG("  Testing HashTable with string keys...");

    AVD_HashTable table;
    avdHashTableCreate(&table, 0, sizeof(int), 0, true);

    // Test string key operations
    const char *key1 = "hello";
    const char *key2 = "world";
    const char *key3 = "test";

    int value1 = 100;
    int value2 = 200;
    int value3 = 300;

    bool result = avdHashTableSet(&table, key1, &value1);
    result &= avdHashTableSet(&table, key2, &value2);
    result &= avdHashTableSet(&table, key3, &value3);

    if (!result) {
        AVD_LOG_ERROR("    ERROR: Failed to set string key values");
        avdHashTableDestroy(&table);
        return false;
    }

    int retrievedValue;
    result = avdHashTableGet(&table, key1, &retrievedValue);
    if (!result || retrievedValue != value1) {
        AVD_LOG_ERROR("    ERROR: Failed to get string key value");
        avdHashTableDestroy(&table);
        return false;
    }

    result = avdHashTableGet(&table, key2, &retrievedValue);
    if (!result || retrievedValue != value2) {
        AVD_LOG_ERROR("    ERROR: Failed to get second string key value");
        avdHashTableDestroy(&table);
        return false;
    }

    // Test long string key
    char longKey[200];
    for (int i = 0; i < 199; i++) {
        longKey[i] = 'a' + (i % 26);
    }
    longKey[199] = '\0';

    int longValue = 999;
    result        = avdHashTableSet(&table, longKey, &longValue);
    if (!result) {
        AVD_LOG_ERROR("    ERROR: Failed to set long string key");
        avdHashTableDestroy(&table);
        return false;
    }

    result = avdHashTableGet(&table, longKey, &retrievedValue);
    if (!result || retrievedValue != longValue) {
        AVD_LOG_ERROR("    ERROR: Failed to get long string key value");
        avdHashTableDestroy(&table);
        return false;
    }

    avdHashTableDestroy(&table);
    AVD_LOG_DEBUG("    HashTable string keys PASSED");
    return true;
}

static bool __avdTestHashTableContains()
{
    AVD_LOG_DEBUG("  Testing HashTable contains operations...");

    AVD_HashTable table;
    avdHashTableCreate(&table, sizeof(int), sizeof(int), 0, false);

    int key1   = 42;
    int value1 = 100;

    // Test contains on empty table
    bool contains = avdHashTableContains(&table, &key1);
    if (contains) {
        AVD_LOG_ERROR("    ERROR: Empty table reports containing key");
        avdHashTableDestroy(&table);
        return false;
    }

    // Add key and test contains
    avdHashTableSet(&table, &key1, &value1);
    contains = avdHashTableContains(&table, &key1);
    if (!contains) {
        AVD_LOG_ERROR("    ERROR: Table doesn't report containing existing key");
        avdHashTableDestroy(&table);
        return false;
    }

    // Test contains with non-existing key
    int key2 = 99;
    contains = avdHashTableContains(&table, &key2);
    if (contains) {
        AVD_LOG_ERROR("    ERROR: Table reports containing non-existing key");
        avdHashTableDestroy(&table);
        return false;
    }

    avdHashTableDestroy(&table);
    AVD_LOG_DEBUG("    HashTable contains operations PASSED");
    return true;
}

static bool __avdTestHashTableRemove()
{
    AVD_LOG_DEBUG("  Testing HashTable remove operations...");

    AVD_HashTable table;
    avdHashTableCreate(&table, sizeof(int), sizeof(int), 0, false);

    // Test remove from empty table
    int key1    = 42;
    bool result = avdHashTableRemove(&table, &key1);
    if (result) {
        AVD_LOG_ERROR("    ERROR: Remove from empty table reported success");
        avdHashTableDestroy(&table);
        return false;
    }

    // Add some values
    for (int i = 0; i < 10; i++) {
        int key   = i;
        int value = i * 10;
        avdHashTableSet(&table, &key, &value);
    }

    size_t count;
    avdHashTableCount(&table, &count);
    if (count != 10) {
        AVD_LOG_ERROR("    ERROR: Incorrect count after adding elements");
        avdHashTableDestroy(&table);
        return false;
    }

    // Remove middle element
    int keyToRemove = 5;
    result          = avdHashTableRemove(&table, &keyToRemove);
    if (!result) {
        AVD_LOG_ERROR("    ERROR: Failed to remove existing key");
        avdHashTableDestroy(&table);
        return false;
    }

    // Verify it's removed
    bool contains = avdHashTableContains(&table, &keyToRemove);
    if (contains) {
        AVD_LOG_ERROR("    ERROR: Removed key still exists");
        avdHashTableDestroy(&table);
        return false;
    }

    // Verify count decreased
    avdHashTableCount(&table, &count);
    if (count != 9) {
        AVD_LOG_ERROR("    ERROR: Count not decreased after removal");
        avdHashTableDestroy(&table);
        return false;
    }

    // Verify other elements still exist
    for (int i = 0; i < 10; i++) {
        if (i == 5)
            continue; // Skip removed element

        int key = i;
        int value;
        result = avdHashTableGet(&table, &key, &value);
        if (!result || value != i * 10) {
            AVD_LOG_ERROR("    ERROR: Other elements affected by removal");
            avdHashTableDestroy(&table);
            return false;
        }
    }

    // Test removing non-existing key
    int nonExistingKey = 99;
    result             = avdHashTableRemove(&table, &nonExistingKey);
    if (result) {
        AVD_LOG_ERROR("    ERROR: Remove of non-existing key reported success");
        avdHashTableDestroy(&table);
        return false;
    }

    avdHashTableDestroy(&table);
    AVD_LOG_DEBUG("    HashTable remove operations PASSED");
    return true;
}

static bool __avdTestHashTableClear()
{
    AVD_LOG_DEBUG("  Testing HashTable clear operations...");

    AVD_HashTable table;
    avdHashTableCreate(&table, sizeof(int), sizeof(int), 0, false);

    // Test clear on empty table
    bool result = avdHashTableClear(&table);
    if (!result) {
        AVD_LOG_ERROR("    ERROR: Clear on empty table failed");
        avdHashTableDestroy(&table);
        return false;
    }

    // Add elements
    for (int i = 0; i < 20; i++) {
        int key   = i;
        int value = i * 5;
        avdHashTableSet(&table, &key, &value);
    }

    size_t count;
    avdHashTableCount(&table, &count);
    if (count != 20) {
        AVD_LOG_ERROR("    ERROR: Incorrect count before clear");
        avdHashTableDestroy(&table);
        return false;
    }

    // Clear table
    result = avdHashTableClear(&table);
    if (!result) {
        AVD_LOG_ERROR("    ERROR: Clear operation failed");
        avdHashTableDestroy(&table);
        return false;
    }

    // Verify table is empty
    avdHashTableCount(&table, &count);
    if (count != 0) {
        AVD_LOG_ERROR("    ERROR: Count not zero after clear");
        avdHashTableDestroy(&table);
        return false;
    }

    // Verify elements are not retrievable
    for (int i = 0; i < 20; i++) {
        int key       = i;
        bool contains = avdHashTableContains(&table, &key);
        if (contains) {
            AVD_LOG_ERROR("    ERROR: Elements still exist after clear");
            avdHashTableDestroy(&table);
            return false;
        }
    }

    // Test that table can be used again after clear
    int newKey   = 100;
    int newValue = 200;
    result       = avdHashTableSet(&table, &newKey, &newValue);
    if (!result) {
        AVD_LOG_ERROR("    ERROR: Cannot use table after clear");
        avdHashTableDestroy(&table);
        return false;
    }

    avdHashTableDestroy(&table);
    AVD_LOG_DEBUG("    HashTable clear operations PASSED");
    return true;
}

static bool __avdTestHashTableMemoryReuse()
{
    AVD_LOG_DEBUG("  Testing HashTable memory reuse after removal...");

    AVD_HashTable table;
    avdHashTableCreate(&table, sizeof(int), sizeof(int), 0, false);

    // Add many elements to trigger multiple banks
    for (int i = 0; i < 200; i++) {
        int key   = i;
        int value = i * 2;
        avdHashTableSet(&table, &key, &value);
    }

    size_t initialBankCount = table.bankCount;

    // Remove half the elements
    for (int i = 0; i < 100; i++) {
        int key = i;
        avdHashTableRemove(&table, &key);
    }

    size_t count;
    avdHashTableCount(&table, &count);
    if (count != 100) {
        AVD_LOG_ERROR("    ERROR: Incorrect count after removal");
        avdHashTableDestroy(&table);
        return false;
    }

    // Add new elements (should reuse freed entries)
    for (int i = 1000; i < 1100; i++) {
        int key   = i;
        int value = i * 3;
        avdHashTableSet(&table, &key, &value);
    }

    // Bank count should not have increased significantly due to reuse
    if (table.bankCount > initialBankCount + 1) {
        AVD_LOG_ERROR("    ERROR: Too many new banks created, memory reuse not working");
        avdHashTableDestroy(&table);
        return false;
    }

    // Verify all remaining and new elements
    for (int i = 100; i < 200; i++) {
        int key = i;
        int value;
        bool result = avdHashTableGet(&table, &key, &value);
        if (!result || value != i * 2) {
            AVD_LOG_ERROR("    ERROR: Remaining element not found correctly");
            avdHashTableDestroy(&table);
            return false;
        }
    }

    for (int i = 1000; i < 1100; i++) {
        int key = i;
        int value;
        bool result = avdHashTableGet(&table, &key, &value);
        if (!result || value != i * 3) {
            AVD_LOG_ERROR("    ERROR: New element not found correctly");
            avdHashTableDestroy(&table);
            return false;
        }
    }

    avdHashTableDestroy(&table);
    AVD_LOG_DEBUG("    HashTable memory reuse PASSED");
    return true;
}

static bool __avdTestHashTableGetKeys()
{
    AVD_LOG_DEBUG("  Testing HashTable get keys operations...");

    AVD_HashTable table;
    avdHashTableCreate(&table, sizeof(int), sizeof(int), 0, false);

    // Test get keys on empty table
    void *keys[100];
    size_t keyCount;
    bool result = avdHashTableGetKeys(&table, keys, &keyCount, 100);
    if (!result || keyCount != 0) {
        AVD_LOG_ERROR("    ERROR: Get keys on empty table failed");
        avdHashTableDestroy(&table);
        return false;
    }

    // Add some elements
    int testKeys[]   = {1, 5, 10, 15, 20};
    int testValues[] = {10, 50, 100, 150, 200};

    for (int i = 0; i < 5; i++) {
        avdHashTableSet(&table, &testKeys[i], &testValues[i]);
    }

    // Get keys
    result = avdHashTableGetKeys(&table, keys, &keyCount, 100);
    if (!result || keyCount != 5) {
        AVD_LOG_ERROR("    ERROR: Get keys returned wrong count");
        avdHashTableDestroy(&table);
        return false;
    }

    // Verify all test keys are in the result
    bool foundKeys[5] = {false, false, false, false, false};
    for (size_t i = 0; i < keyCount; i++) {
        int *key = (int *)keys[i];
        for (int j = 0; j < 5; j++) {
            if (*key == testKeys[j]) {
                foundKeys[j] = true;
                break;
            }
        }
    }

    for (int i = 0; i < 5; i++) {
        if (!foundKeys[i]) {
            AVD_LOG_ERROR("    ERROR: Not all keys found in get keys result");
            avdHashTableDestroy(&table);
            return false;
        }
    }

    // Test with limited buffer
    result = avdHashTableGetKeys(&table, keys, &keyCount, 3);
    if (!result || keyCount > 3) {
        AVD_LOG_ERROR("    ERROR: Get keys didn't respect max keys limit");
        avdHashTableDestroy(&table);
        return false;
    }

    avdHashTableDestroy(&table);
    AVD_LOG_DEBUG("    HashTable get keys operations PASSED");
    return true;
}

static bool __avdTestHashTableCollisions()
{
    AVD_LOG_DEBUG("  Testing HashTable collision handling...");

    AVD_HashTable table;
    avdHashTableCreate(&table, sizeof(int), sizeof(int), 0, false);

    // Create keys that are likely to cause collisions
    // We'll use a pattern that might cause hash collisions
    int numCollisionKeys = 50;
    for (int i = 0; i < numCollisionKeys; i++) {
        int key   = i * AVD_HASHTABLE_SIZE + 42; // Force same hash bucket
        int value = i + 1000;

        bool result = avdHashTableSet(&table, &key, &value);
        if (!result) {
            AVD_LOG_ERROR("    ERROR: Failed to set collision key");
            avdHashTableDestroy(&table);
            return false;
        }
    }

    // Verify all collision keys can be retrieved
    for (int i = 0; i < numCollisionKeys; i++) {
        int key = i * AVD_HASHTABLE_SIZE + 42;
        int value;

        bool result = avdHashTableGet(&table, &key, &value);
        if (!result || value != i + 1000) {
            AVD_LOG_ERROR("    ERROR: Failed to retrieve collision key");
            avdHashTableDestroy(&table);
            return false;
        }
    }

    // Remove some collision keys and verify others remain
    for (int i = 0; i < numCollisionKeys; i += 2) {
        int key     = i * AVD_HASHTABLE_SIZE + 42;
        bool result = avdHashTableRemove(&table, &key);
        if (!result) {
            AVD_LOG_ERROR("    ERROR: Failed to remove collision key");
            avdHashTableDestroy(&table);
            return false;
        }
    }

    // Verify remaining keys still work
    for (int i = 1; i < numCollisionKeys; i += 2) {
        int key = i * AVD_HASHTABLE_SIZE + 42;
        int value;

        bool result = avdHashTableGet(&table, &key, &value);
        if (!result || value != i + 1000) {
            AVD_LOG_ERROR("    ERROR: Remaining collision key not found");
            avdHashTableDestroy(&table);
            return false;
        }
    }

    avdHashTableDestroy(&table);
    AVD_LOG_DEBUG("    HashTable collision handling PASSED");
    return true;
}

static bool __avdTestHashTableEdgeCases()
{
    AVD_LOG_DEBUG("  Testing HashTable edge cases...");

    // Test NULL pointer handling
    bool result = avdHashTableCreate(NULL, sizeof(int), sizeof(int), 0, false);
    if (result) {
        AVD_LOG_ERROR("    ERROR: Create with NULL table succeeded");
        return false;
    }

    AVD_HashTable table;
    avdHashTableCreate(&table, sizeof(int), sizeof(int), 0, false);

    int key   = 42;
    int value = 100;

    // Test NULL key/value handling
    result = avdHashTableSet(&table, NULL, &value);
    if (result) {
        AVD_LOG_ERROR("    ERROR: Set with NULL key succeeded");
        avdHashTableDestroy(&table);
        return false;
    }

    result = avdHashTableSet(&table, &key, NULL);
    if (result) {
        AVD_LOG_ERROR("    ERROR: Set with NULL value succeeded");
        avdHashTableDestroy(&table);
        return false;
    }

    result = avdHashTableGet(&table, NULL, &value);
    if (result) {
        AVD_LOG_ERROR("    ERROR: Get with NULL key succeeded");
        avdHashTableDestroy(&table);
        return false;
    }

    // Test with zero sizes
    avdHashTableDestroy(&table);
    result = avdHashTableCreate(&table, 0, sizeof(int), 0, false);
    if (result) {
        AVD_LOG_ERROR("    ERROR: Create with zero key size for non-string succeeded");
        return false;
    }

    result = avdHashTableCreate(&table, sizeof(int), 0, 0, false);
    if (result) {
        AVD_LOG_ERROR("    ERROR: Create with zero item size succeeded");
        return false;
    }

    // Test very large initial capacity
    result = avdHashTableCreate(&table, sizeof(int), sizeof(int), SIZE_MAX / 2, false);
    if (result) {
        AVD_LOG_ERROR("    ERROR: Create with huge initial capacity succeeded");
        return false;
    }

    AVD_LOG_DEBUG("    HashTable edge cases PASSED");
    return true;
}

static bool __avdTestHashTableLargeDataTypes()
{
    AVD_LOG_DEBUG("  Testing HashTable with large data types...");

    typedef struct {
        char name[64];
        int id;
        float data[16];
        double timestamp;
    } LargeStruct;

    AVD_HashTable table;
    avdHashTableCreate(&table, sizeof(int), sizeof(LargeStruct), 0, false);

    // Test with large struct
    int key = 1;
    LargeStruct largeValue;
    strcpy(largeValue.name, "Test Structure");
    largeValue.id = 12345;
    for (int i = 0; i < 16; i++) {
        largeValue.data[i] = i * 1.5f;
    }
    largeValue.timestamp = 1234567890.123;

    bool result = avdHashTableSet(&table, &key, &largeValue);
    if (!result) {
        AVD_LOG_ERROR("    ERROR: Failed to set large struct value");
        avdHashTableDestroy(&table);
        return false;
    }

    LargeStruct retrievedValue;
    result = avdHashTableGet(&table, &key, &retrievedValue);
    if (!result) {
        AVD_LOG_ERROR("    ERROR: Failed to get large struct value");
        avdHashTableDestroy(&table);
        return false;
    }

    // Verify struct contents
    if (strcmp(retrievedValue.name, "Test Structure") != 0 ||
        retrievedValue.id != 12345 ||
        retrievedValue.timestamp != 1234567890.123) {
        AVD_LOG_ERROR("    ERROR: Large struct contents not preserved");
        avdHashTableDestroy(&table);
        return false;
    }

    for (int i = 0; i < 16; i++) {
        if (retrievedValue.data[i] != i * 1.5f) {
            AVD_LOG_ERROR("    ERROR: Large struct array data not preserved");
            avdHashTableDestroy(&table);
            return false;
        }
    }

    avdHashTableDestroy(&table);
    AVD_LOG_DEBUG("    HashTable large data types PASSED");
    return true;
}

bool avdHashTableTestsRun(void)
{
    AVD_LOG_DEBUG("Running HashTable tests...");

    bool allPassed = true;

    allPassed &= __avdTestHashTableInit();
    allPassed &= __avdTestHashTableSetGet();
    allPassed &= __avdTestHashTableStringKeys();
    allPassed &= __avdTestHashTableContains();
    allPassed &= __avdTestHashTableRemove();
    allPassed &= __avdTestHashTableClear();
    allPassed &= __avdTestHashTableMemoryReuse();
    allPassed &= __avdTestHashTableGetKeys();
    allPassed &= __avdTestHashTableCollisions();
    allPassed &= __avdTestHashTableEdgeCases();
    allPassed &= __avdTestHashTableLargeDataTypes();

    if (allPassed) {
        AVD_LOG_DEBUG("All HashTable tests PASSED!");
    } else {
        AVD_LOG_DEBUG("Some HashTable tests FAILED!");
    }

    return allPassed;
}
