#include "ps_common.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

// a very very basic hash function, not suitable for cryptography or security
// but good enough for simple hashing needs like detecting hanges in shader!
uint32_t psHashBuffer(const void *buffer, size_t size) {
    uint32_t hash = 5381;
    const uint8_t *ptr = (const uint8_t *)buffer;
    static const size_t offsetTable[16] = {23, 19, 13, 7, 3, 29, 31, 17, 11, 5, 2, 1, 0, 4, 8, 12};
    for (size_t i = 0; i < size; i++) {
        uint32_t a = hash << offsetTable[i % 16];
        uint32_t b = hash >> offsetTable[(i >> 2) % 16];
        uint32_t c = hash << offsetTable[(i >> 1) % 16];
        uint32_t d = hash >> offsetTable[(i >> 3) % 16];
        hash = a ^ b ^ c ^ d ^ (uint32_t)ptr[i];
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