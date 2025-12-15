#include "core/avd_hashtable.h"
#include "core/avd_utils.h"

static size_t __avdHashTableHash(const AVD_HashTable *table, const void *key)
{
    if (table->stringKey) {
        return avdHashString((const char *)key) % AVD_HASHTABLE_SIZE;
    } else {
        return avdHashBuffer(key, table->keySize) % AVD_HASHTABLE_SIZE;
    }
}

static bool __avdHashTableKeysEqual(const AVD_HashTable *table, const void *key1, const void *key2)
{
    if (table->stringKey) {
        return strcmp((const char *)key1, (const char *)key2) == 0;
    } else {
        return memcmp(key1, key2, table->keySize) == 0;
    }
}

static void __avdHashTableCopyKey(const AVD_HashTable *table, char *dest, const void *src)
{
    if (table->stringKey) {
        strncpy(dest, (const char *)src, AVD_HASHTABLE_KEY_MAX_SIZE - 1);
        dest[AVD_HASHTABLE_KEY_MAX_SIZE - 1] = '\0';
    } else {
        memcpy(dest, src, table->keySize);
    }
}

static AVD_HashTableEntry *__avdHashTableAllocateEntry(AVD_HashTable *table)
{
    for (size_t i = 0; i < table->bankCount; i++) {
        AVD_HashTableBank *bank = &table->banks[i];
        for (size_t j = 0; j < bank->bankCount; j++) {
            AVD_HashTableEntry *entry = &bank->entries[j];
            if (entry->value == NULL) {
                uint8_t *data = (uint8_t *)bank->dataHolder;
                entry->value  = &data[j * table->itemSize];
                return entry;
            }
        }
    }

    for (size_t i = 0; i < table->bankCount; i++) {
        AVD_HashTableBank *bank = &table->banks[i];
        if (bank->bankCount < AVD_HASHTABLE_BANK_CAPACITY) {
            AVD_HashTableEntry *entry = &bank->entries[bank->bankCount];
            uint8_t *data             = (uint8_t *)bank->dataHolder;
            entry->value              = &data[bank->bankCount * table->itemSize];
            bank->bankCount++;
            return entry;
        }
    }

    table->bankCount++;
    table->banks = realloc(table->banks, sizeof(AVD_HashTableBank) * table->bankCount);
    if (!table->banks) {
        return NULL;
    }

    AVD_HashTableBank *newBank = &table->banks[table->bankCount - 1];
    newBank->entries           = malloc(sizeof(AVD_HashTableEntry) * AVD_HASHTABLE_BANK_CAPACITY);
    newBank->dataHolder        = malloc(table->itemSize * AVD_HASHTABLE_BANK_CAPACITY);
    newBank->bankCount         = 1;

    if (!newBank->entries || !newBank->dataHolder) {
        return NULL;
    }

    for (size_t j = 0; j < AVD_HASHTABLE_BANK_CAPACITY; j++) {
        newBank->entries[j].value = NULL;
        newBank->entries[j].next  = NULL;
    }

    AVD_HashTableEntry *entry = &newBank->entries[0];
    entry->value              = newBank->dataHolder;
    return entry;
}

bool avdHashTableCreate(AVD_HashTable *table, size_t keySize, size_t itemSize, size_t initialCapacity, bool stringKey)
{
    if (!table) {
        return false;
    }

    if (itemSize == 0) {
        return false;
    }

    if (!stringKey && keySize == 0) {
        return false;
    }

    memset(table, 0, sizeof(AVD_HashTable));

    table->keySize   = keySize;
    table->itemSize  = itemSize;
    table->stringKey = stringKey;
    table->count     = 0;
    table->bankCount = 0;
    table->banks     = NULL;

    table->table = malloc(sizeof(AVD_HashTableEntry *) * AVD_HASHTABLE_SIZE);
    if (!table->table) {
        return false;
    }
    memset(table->table, 0, sizeof(AVD_HashTableEntry *) * AVD_HASHTABLE_SIZE);

    if (initialCapacity > 0) {
        size_t banksNeeded = (initialCapacity + AVD_HASHTABLE_BANK_CAPACITY - 1) / AVD_HASHTABLE_BANK_CAPACITY;
        table->banks       = malloc(sizeof(AVD_HashTableBank) * banksNeeded);
        if (!table->banks) {
            return false;
        }

        for (size_t i = 0; i < banksNeeded; i++) {
            table->banks[i].entries    = malloc(sizeof(AVD_HashTableEntry) * AVD_HASHTABLE_BANK_CAPACITY);
            table->banks[i].dataHolder = malloc(itemSize * AVD_HASHTABLE_BANK_CAPACITY);
            table->banks[i].bankCount  = 0;

            if (!table->banks[i].entries || !table->banks[i].dataHolder) {
                for (size_t j = 0; j <= i; j++) {
                    free(table->banks[j].entries);
                    free(table->banks[j].dataHolder);
                }
                free(table->banks);
                return false;
            }

            for (size_t j = 0; j < AVD_HASHTABLE_BANK_CAPACITY; j++) {
                table->banks[i].entries[j].value = NULL;
                table->banks[i].entries[j].next  = NULL;
            }
        }
        table->bankCount = banksNeeded;
    }

    return true;
}

bool avdHashTableDestroy(AVD_HashTable *table)
{
    if (!table) {
        return false;
    }

    if (table->table) {
        free(table->table);
    }

    if (table->banks) {
        for (size_t i = 0; i < table->bankCount; i++) {
            free(table->banks[i].entries);
            free(table->banks[i].dataHolder);
        }
        free(table->banks);
    }

    memset(table, 0, sizeof(AVD_HashTable));
    return true;
}

bool avdHashTableSet(AVD_HashTable *table, const void *key, const void *value)
{
    if (!table || !key || !value) {
        return false;
    }

    size_t hash               = __avdHashTableHash(table, key);
    AVD_HashTableEntry *entry = table->table[hash];

    while (entry) {
        if (__avdHashTableKeysEqual(table, entry->key, key)) {
            memcpy(entry->value, value, table->itemSize);
            return true;
        }
        entry = entry->next;
    }

    AVD_HashTableEntry *newEntry = __avdHashTableAllocateEntry(table);
    if (!newEntry) {
        return false;
    }

    __avdHashTableCopyKey(table, newEntry->key, key);
    memcpy(newEntry->value, value, table->itemSize);
    newEntry->next     = table->table[hash];
    table->table[hash] = newEntry;
    table->count++;

    return true;
}

bool avdHashTableGet(AVD_HashTable *table, const void *key, void *outValue)
{
    if (!table || !key) {
        return false;
    }

    size_t hash               = __avdHashTableHash(table, key);
    AVD_HashTableEntry *entry = table->table[hash];

    while (entry) {
        if (__avdHashTableKeysEqual(table, entry->key, key)) {
            if (outValue) {
                memcpy(outValue, entry->value, table->itemSize);
            }
            return true;
        }
        entry = entry->next;
    }

    return false;
}

bool avdHashTableContains(AVD_HashTable *table, const void *key)
{
    return avdHashTableGet(table, key, NULL);
}

bool avdHashTableRemove(AVD_HashTable *table, const void *key)
{
    if (!table || !key) {
        return false;
    }

    size_t hash               = __avdHashTableHash(table, key);
    AVD_HashTableEntry *entry = table->table[hash];
    AVD_HashTableEntry *prev  = NULL;

    while (entry) {
        if (__avdHashTableKeysEqual(table, entry->key, key)) {
            if (prev) {
                prev->next = entry->next;
            } else {
                table->table[hash] = entry->next;
            }

            entry->value = NULL;
            entry->next  = NULL;

            table->count--;
            return true;
        }
        prev  = entry;
        entry = entry->next;
    }

    return false;
}

bool avdHashTableClear(AVD_HashTable *table)
{
    if (!table) {
        return false;
    }

    memset(table->table, 0, sizeof(AVD_HashTableEntry *) * AVD_HASHTABLE_SIZE);
    table->count = 0;

    for (size_t i = 0; i < table->bankCount; i++) {
        AVD_HashTableBank *bank = &table->banks[i];
        for (size_t j = 0; j < bank->bankCount; j++) {
            bank->entries[j].value = NULL;
            bank->entries[j].next  = NULL;
        }
    }

    return true;
}

bool avdHashTableCount(const AVD_HashTable *table, size_t *outCount)
{
    if (!table || !outCount) {
        return false;
    }

    *outCount = table->count;
    return true;
}

bool avdHashTableGetKeys(const AVD_HashTable *table, void **outKeys, size_t *outKeyCount, size_t maxKeys)
{
    if (!table || !outKeys || !outKeyCount) {
        return false;
    }

    size_t keyCount = 0;

    for (size_t i = 0; i < AVD_HASHTABLE_SIZE && keyCount < maxKeys; i++) {
        AVD_HashTableEntry *entry = table->table[i];
        while (entry && keyCount < maxKeys) {
            outKeys[keyCount] = (void *)entry->key;
            keyCount++;
            entry = entry->next;
        }
    }

    *outKeyCount = keyCount;
    return true;
}