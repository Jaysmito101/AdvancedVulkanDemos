#ifndef AVD_BASE_H
#define AVD_BASE_H

// common stdlib includes
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#endif

// common macros
#define AVD_ARRAY_COUNT(arr)       (sizeof(arr) / sizeof((arr)[0]))
#define AVD_MIN(a, b)              ((a) < (b) ? (a) : (b))
#define AVD_MAX(a, b)              ((a) > (b) ? (a) : (b))
#define AVD_CLAMP(value, min, max) (AVD_MIN(AVD_MAX(value, min), max))
#define AVD_SWAP(a, b)         \
    {                          \
        typeof(a) temp = a;    \
        a              = b;    \
        b              = temp; \
    }
#define AVD_OFFSET_OF(type, member) ((size_t) & (((type *)0)->member))
#define AVD_ASSERT(condition)       assert(condition)
#define AVD_LOG(msg, ...)           printf(msg, ##__VA_ARGS__)

#define AVD_CHECK_VK_HANDLE(result, log, ...) \
    if (result == VK_NULL_HANDLE) {           \
        AVD_LOG(log, ##__VA_ARGS__);          \
        return false;                         \
    }

#define AVD_CHECK_VK_RESULT(result, log, ...)                                     \
    {                                                                             \
        VkResult localResult = result;                                            \
        if (localResult != VK_SUCCESS) {                                          \
            AVD_LOG("------------------------------------\n");                    \
            AVD_LOG("Check [%s] [%s] failed in %s:%d\n", #result, string_VkResult(localResult), __FILE__, __LINE__); \
            AVD_LOG(log, ##__VA_ARGS__);                                          \
            AVD_LOG("------------------------------------\n");                    \
            return false;                                                         \
        }                                                                        \
    }

#define AVD_CHECK_MSG(result, log, ...)                                       \
    if (!(result)) {                                                          \
        AVD_LOG("------------------------------------\n");                    \
        AVD_LOG("Check [%s] failed in %s:%d\n", #result, __FILE__, __LINE__); \
        AVD_LOG(log, ##__VA_ARGS__);                                          \
        AVD_LOG("------------------------------------\n");                    \
        return false;                                                         \
    }

#define AVD_CHECK(result)                                                     \
    if (!(result)) {                                                          \
        AVD_LOG("------------------------------------\n");                    \
        AVD_LOG("Check [%s] failed in %s:%d\n", #result, __FILE__, __LINE__); \
        AVD_LOG("------------------------------------\n");                    \
        return false;                                                         \
    }

#define GAME_WIDTH  1920
#define GAME_HEIGHT 1080

#endif // AVD_BASE_H