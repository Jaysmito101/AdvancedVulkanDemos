
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include "core/avd_utils.h"

// FNV-1a 32-bit hash function
// See: http://www.isthe.com/chongo/tech/comp/fnv/
uint32_t avdHashBuffer(const void *buffer, size_t size)
{
    uint32_t hash      = 2166136261u; // FNV offset basis
    const uint8_t *ptr = (const uint8_t *)buffer;
    for (size_t i = 0; i < size; i++) {
        hash ^= (uint32_t)ptr[i];
        hash *= 16777619u; // FNV prime
    }
    return hash;
}

uint32_t avdHashString(const char *str)
{
    return avdHashBuffer(str, strlen(str));
}

const char *avdGetTempDirPath(void)
{
    static char tempDirPath[2048];
#ifdef _WIN32
    char baseTemp[1024];
    GetTempPathA((DWORD)sizeof(baseTemp), baseTemp);

    snprintf(tempDirPath, sizeof(tempDirPath), "%savd\\", baseTemp);

    DWORD attrs = GetFileAttributesA(tempDirPath);
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        char dirNoSlash[2048];
        strncpy(dirNoSlash, tempDirPath, sizeof(dirNoSlash));
        dirNoSlash[sizeof(dirNoSlash) - 1] = '\0';
        size_t l                           = strlen(dirNoSlash);
        if (l > 0 && (dirNoSlash[l - 1] == '\\' || dirNoSlash[l - 1] == '/')) {
            dirNoSlash[l - 1] = '\0';
        }
        CreateDirectoryA(dirNoSlash, NULL);
    }
#else
    const char *base = getenv("TMPDIR");
    if (base == NULL || base[0] == '\0')
        base = "/tmp";

    char dirPath[1024];
    snprintf(dirPath, sizeof(dirPath), "%s/avd", base);

    struct stat st;
    if (stat(dirPath, &st) != 0) {
        mkdir(dirPath, 0700);
    }

    snprintf(tempDirPath, sizeof(tempDirPath), "%s/", dirPath);
#endif
    return tempDirPath;
}

bool avdPathExists(const char *path)
{
#if defined(_WIN32) || defined(__CYGWIN__)
    DWORD fileAttributes = GetFileAttributesA(path);
    return (fileAttributes != INVALID_FILE_ATTRIBUTES && !(fileAttributes & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat buffer;
    return (stat(path, &buffer) == 0 && S_ISREG(buffer.st_mode));
#endif
}

// print the  shader code formatted with line number (only one line at a time, use a temp buffer to extract)
void avdPrintShaderWithLineNumbers(const char *shaderCode, const char *shaderName)
{
    if (shaderCode == NULL) {
        AVD_LOG_ERROR("Shader code is NULL");
        return;
    }

    AVD_LOG_INFO("Shader: %s", shaderName);
    const char *lineStart = shaderCode;
    int lineNumber        = 1;
    while (*lineStart) {
        const char *lineEnd = strchr(lineStart, '\n');
        if (lineEnd == NULL) {
            lineEnd = lineStart + strlen(lineStart);
        }

        size_t lineLength = lineEnd - lineStart;
        char *lineBuffer  = (char *)malloc(lineLength + 1);
        if (lineBuffer == NULL) {
            AVD_LOG_ERROR("Failed to allocate memory for shader line");
            return;
        }

        strncpy(lineBuffer, lineStart, lineLength);
        lineBuffer[lineLength] = '\0';

        AVD_LOG_INFO("%d: %s", lineNumber++, lineBuffer);

        free(lineBuffer);
        lineStart = (*lineEnd == '\0') ? lineEnd : lineEnd + 1;
    }
    AVD_LOG_INFO("");
}

void avdSleep(uint32_t milliseconds)
{
#if defined(_WIN32) || defined(__CYGWIN__)
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

void avdMessageBox(const char *title, const char *message)
{
#if defined(_WIN32) || defined(__CYGWIN__)
    MessageBoxA(NULL, message, title, MB_OK | MB_ICONINFORMATION);
#else
    fprintf(stderr, "%s: %s\n", title, message);
    if (system("which zenity > /dev/null 2>&1") == 0) {
        char command[1024];
        snprintf(command, sizeof(command), "zenity --info --title=\"%s\" --text=\"%s\"", title, message);
        system(command);
    } else {
        fprintf(stderr, "%s: %s\n", title, message);
        fprintf(stderr, "Zenity is not available. Please install it to show message boxes.\n");
    }
#endif
}

bool avdReadBinaryFile(const char *filename, void **data, size_t *size)
{
    if (filename == NULL || data == NULL || size == NULL) {
        return false;
    }

    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        return false;
    }

    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);

    *data = malloc(*size);
    if (*data == NULL) {
        fclose(file);
        return false;
    }

    fread(*data, 1, *size, file);
    fclose(file);
    return true;
}

const char *avdDumpToTmpFile(const void *data, size_t size, const char *extension, const char *prefix)
{
    static char tmpFilePath[4096];
    const char *tempDir = avdGetTempDirPath();
    uint32_t hash       = avdHashBuffer(data, size);
    snprintf(tmpFilePath, sizeof(tmpFilePath), "%savd_%s.DUMP.%d.%s", tempDir, prefix ? prefix : "Generic", hash, extension ? extension : "bin");

    FILE *file = fopen(tmpFilePath, "wb");
    if (!file) {
        AVD_LOG_ERROR("Failed to open temp file for writing: %s", tmpFilePath);
        return NULL;
    }

    size_t written = fwrite(data, 1, size, file);
    fclose(file);

    AVD_LOG_INFO("Dumped data to temp file: %s", tmpFilePath);

    if (written != size) {
        AVD_LOG_ERROR("Failed to write all data to temp file: %s", tmpFilePath);
        return NULL;
    }

    return tmpFilePath;
}

// NOTE: These implementations are taken from the meshoptimizer library.
// Source: https://github.com/zeux/meshoptimizer/blob/3beccf6f3653992cf1724a5ff954af8455eb9965/src/quantization.cpp

typedef union {
    float f;
    uint32_t ui;
} AVD_FloatBits;

uint16_t avdQuantizeHalf(float v)
{
    AVD_FloatBits u = {v};
    unsigned int ui = u.ui;

    int s  = (ui >> 16) & 0x8000;
    int em = ui & 0x7fffffff;

    // bias exponent and round to nearest; 112 is relative exponent bias (127-15)
    int h = (em - (112 << 23) + (1 << 12)) >> 13;

    // underflow: flush to zero; 113 encodes exponent -14
    h = (em < (113 << 23)) ? 0 : h;

    // overflow: infinity; 143 encodes exponent 16
    h = (em >= (143 << 23)) ? 0x7c00 : h;

    // NaN; note that we convert all types of NaN to qNaN
    h = (em > (255 << 23)) ? 0x7e00 : h;

    return (unsigned short)(s | h);
}

float avdDequantizeHalf(uint16_t h)
{
    unsigned int s = (unsigned)(h & 0x8000) << 16;
    int em         = h & 0x7fff;

    // bias exponent and pad mantissa with 0; 112 is relative exponent bias (127-15)
    int r = (em + (112 << 10)) << 13;

    // denormal: flush to zero
    r = (em < (1 << 10)) ? 0 : r;

    // infinity/NaN; note that we preserve NaN payload as a byproduct of unifying inf/nan cases
    // 112 is an exponent bias fixup; since we already applied it once, applying it twice converts 31 to 255
    r += (em >= (31 << 10)) ? (112 << 23) : 0;

    AVD_FloatBits u;
    u.ui = s | r;
    return u.f;
}

// Source: https://github.com/zeux/meshoptimizer/blob/3beccf6f3653992cf1724a5ff954af8455eb9965/src/meshoptimizer.h#L875
int avdQuantizeSnorm(float v, int N)
{
    const float scale = (float)((1 << (N - 1)) - 1);

    float round = (v >= 0 ? 0.5f : -0.5f);

    v = (v >= -1) ? v : -1;
    v = (v <= +1) ? v : +1;

    return (int)(v * scale + round);
}