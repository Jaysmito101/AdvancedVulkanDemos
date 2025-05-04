#include "ps_common.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

// FNV-1a 32-bit hash function
// See: http://www.isthe.com/chongo/tech/comp/fnv/
uint32_t psHashBuffer(const void *buffer, size_t size) {
    uint32_t hash = 2166136261u; // FNV offset basis
    const uint8_t *ptr = (const uint8_t *)buffer;
    for (size_t i = 0; i < size; i++) {
        hash ^= (uint32_t)ptr[i];
        hash *= 16777619u; // FNV prime
    }
    return hash;
}

uint32_t psHashString(const char *str) {
    return psHashBuffer(str, strlen(str));
}

const char* psGetTempDirPath(void) {
    static char tempDirPath[1024];
#ifdef _WIN32
    GetTempPathA(1024, tempDirPath);
#else
    snprintf(tempDirPath, sizeof(tempDirPath), "/tmp/");
#endif
    return tempDirPath;
}