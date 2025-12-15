#ifndef AVD_HASHTABLE_H
#define AVD_HASHTABLE_H

#include "avd_core.h"

#ifndef AVD_HASHTABLE_BANK_CAPACITY
#define AVD_HASHTABLE_BANK_CAPACITY 64
#endif

#ifndef AVD_HASHTABLE_KEY_MAX_SIZE
#define AVD_HASHTABLE_KEY_MAX_SIZE 256
#endif

#ifndef AVD_HASHTABLE_SIZE
#define AVD_HASHTABLE_SIZE 1024 * 16
#endif

typedef struct AVD_HashTableEntry {
    void *value;
    char key[AVD_HASHTABLE_KEY_MAX_SIZE];
    struct AVD_HashTableEntry *next;
} AVD_HashTableEntry;

typedef struct {
    void *dataHolder;
    AVD_HashTableEntry *entries;
    size_t bankCount;
} AVD_HashTableBank;

typedef struct {
    AVD_HashTableBank *banks;
    size_t bankCount;

    AVD_HashTableEntry **table;

    size_t count;
    size_t keySize;
    size_t itemSize;
    bool stringKey;
} AVD_HashTable;

bool avdHashTableCreate(AVD_HashTable *table, size_t keySize, size_t itemSize, size_t initialCapacity, bool stringKey);
bool avdHashTableDestroy(AVD_HashTable *table);
bool avdHashTableSet(AVD_HashTable *table, const void *key, const void *value);
bool avdHashTableGet(AVD_HashTable *table, const void *key, void *outValue);
bool avdHashTableContains(AVD_HashTable *table, const void *key);
bool avdHashTableRemove(AVD_HashTable *table, const void *key);
bool avdHashTableClear(AVD_HashTable *table);
bool avdHashTableCount(const AVD_HashTable *table, size_t *outCount);
bool avdHashTableGetKeys(const AVD_HashTable *table, void **outKeys, size_t *outKeyCount, size_t maxKeys);

bool avdHashTableTestsRun(void);

#endif // AVD_HASHTABLE_H