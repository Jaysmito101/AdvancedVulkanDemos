#ifndef AVD_BASE_H
#define AVD_BASE_H
 
// common stdlib includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <float.h>
#include <time.h>
#include <ctype.h>
#include <stdarg.h>
#include <signal.h>

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

// common macros
#define AVD_ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))
#define AVD_MIN(a, b) ((a) < (b) ? (a) : (b))
#define AVD_MAX(a, b) ((a) > (b) ? (a) : (b))
#define AVD_CLAMP(value, min, max) (AVD_MIN(AVD_MAX(value, min), max))
#define AVD_SWAP(a, b) { typeof(a) temp = a; a = b; b = temp; }
#define AVD_OFFSET_OF(type, member) ((size_t)&(((type *)0)->member))
#define AVD_ASSERT(condition) assert(condition)
#define AVD_LOG(msg, ...) printf(msg, ##__VA_ARGS__)

#define AVD_CHECK_VK_HANDLE(result, log, ...) \
    if (result == VK_NULL_HANDLE) { \
        AVD_LOG(log, ##__VA_ARGS__); \
        return false; \
    }

#define AVD_CHECK_VK_RESULT(result, log, ...) \
    if (result != VK_SUCCESS) { \
        AVD_LOG("------------------------------------\n"); \
        AVD_LOG("Check [%s] failed in %s:%d\n", #result, __FILE__, __LINE__); \
        AVD_LOG(log, ##__VA_ARGS__); \
        AVD_LOG("------------------------------------\n"); \
        return false; \
    }

#define AVD_CHECK_MSG(result, log, ...) \
    if (!(result)) { \
        AVD_LOG("------------------------------------\n"); \
        AVD_LOG("Check [%s] failed in %s:%d\n", #result, __FILE__, __LINE__); \
        AVD_LOG(log, ##__VA_ARGS__); \
        AVD_LOG("------------------------------------\n"); \
        return false; \
    }

#define AVD_CHECK(result) \
    if (!(result)) { \
        AVD_LOG("------------------------------------\n"); \
        AVD_LOG("Check [%s] failed in %s:%d\n", #result, __FILE__, __LINE__); \
        AVD_LOG("------------------------------------\n"); \
        return false; \
    }

#define GAME_WIDTH 1920
#define GAME_HEIGHT 1080

#endif // AVD_BASE_H