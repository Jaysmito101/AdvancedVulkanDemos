#ifndef PS_BASE_H
#define PS_BASE_H

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

// common macros
#define PS_ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))
#define PS_MIN(a, b) ((a) < (b) ? (a) : (b))
#define PS_MAX(a, b) ((a) > (b) ? (a) : (b))
#define PS_CLAMP(value, min, max) (PS_MIN(PS_MAX(value, min), max))
#define PS_SWAP(a, b) { typeof(a) temp = a; a = b; b = temp; }
#define PS_OFFSET_OF(type, member) ((size_t)&(((type *)0)->member))
#define PS_ASSERT(condition) assert(condition)
#define PS_LOG(msg, ...) printf(msg, ##__VA_ARGS__)

#define PS_CHECK_VK_HANDLE(result, log, ...) \
    if (result == VK_NULL_HANDLE) { \
        PS_LOG(log, ##__VA_ARGS__); \
        return false; \
    }

#define PS_CHECK_VK_RESULT(result, log, ...) \
    if (result != VK_SUCCESS) { \
        PS_LOG("------------------------------------\n"); \
        PS_LOG(log, ##__VA_ARGS__); \
        PS_LOG("------------------------------------\n"); \
        return false; \
    }

#define PS_CHECK_MSG(result, log, ...) \
    if (!(result)) { \
        PS_LOG("------------------------------------\n"); \
        PS_LOG(log, ##__VA_ARGS__); \
        PS_LOG("------------------------------------\n"); \
        return false; \
    }

#define PS_CHECK(result) \
    if (!(result)) { \
        PS_LOG("------------------------------------\n"); \
        PS_LOG("Check [%s] failed in %s:%d\n", #result, __FILE__, __LINE__); \
        PS_LOG("------------------------------------\n"); \
        return false; \
    }

#define GAME_WIDTH 1920
#define GAME_HEIGHT 1080

#endif // PS_BASE_H