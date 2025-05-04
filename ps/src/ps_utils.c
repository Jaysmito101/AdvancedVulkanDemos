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

// print the  shader code formatted with line number (only one line at a time, use a temp buffer to extract)
void psPrintShaderWithLineNumbers(const char *shaderCode, const char *shaderName) {
    if (shaderCode == NULL) {
        PS_LOG("Shader code is NULL\n");
        return;
    }

    PS_LOG("Shader: %s\n", shaderName);
    const char *lineStart = shaderCode;
    int lineNumber = 1;
    while (*lineStart) {
        const char *lineEnd = strchr(lineStart, '\n');
        if (lineEnd == NULL) {
            lineEnd = lineStart + strlen(lineStart);
        }

        size_t lineLength = lineEnd - lineStart;
        char *lineBuffer = (char *)malloc(lineLength + 1);
        if (lineBuffer == NULL) {
            PS_LOG("Failed to allocate memory for shader line\n");
            return;
        }

        strncpy(lineBuffer, lineStart, lineLength);
        lineBuffer[lineLength] = '\0';

        PS_LOG("%d: %s\n", lineNumber++, lineBuffer);

        free(lineBuffer);
        lineStart = (*lineEnd == '\0') ? lineEnd : lineEnd + 1;
    }
    PS_LOG("\n");
}