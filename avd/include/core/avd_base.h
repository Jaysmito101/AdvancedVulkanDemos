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

#include "core/avd_types.h"

#include "pico/picoLog.h"
#include "pico/picoPerf.h"

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
#define AVD_OFFSET_OF(type, member) ((size_t)&(((type *)0)->member))
#define AVD_ALIGN(value, alignment) ((((value) + ((alignment) - 1)) / (alignment)) * (alignment))
#define AVD_ASSERT(condition)                                                                        \
    {                                                                                                \
        if (!(condition)) {                                                                          \
            AVD_LOG_ERROR("Assertion failed: %s, file %s, line %d", #condition, __FILE__, __LINE__); \
            assert(condition);                                                                       \
        }                                                                                            \
    }

#if defined(_WIN32) || defined(_WIN64)
#include <malloc.h>
#define AVD_ALIGNED_ALLOC(alignment, size) _aligned_malloc(size, alignment)
#define AVD_ALIGNED_FREE(ptr)              _aligned_free(ptr)
#else
#define AVD_ALIGNED_ALLOC(alignment, size) aligned_alloc(alignment, size)
#define AVD_ALIGNED_FREE(ptr)              free(ptr)
#endif

#define AVD_MALLOC(size) malloc(size)
#define AVD_FREE(ptr)                                                                  \
    if (ptr != NULL) {                                                                 \
        free(ptr);                                                                     \
        ptr = NULL;                                                                    \
    } else {                                                                           \
        AVD_LOG_WARN("Attempted to free a NULL pointer at %s:%d", __FILE__, __LINE__); \
    }

#define AVD_EXECUTE_DEBOUNCED(intervalMs, codeBlock)                                                         \
    do {                                                                                                     \
        static picoPerfTime __avdDebouncedExecutorLastTime = {0};                                            \
        static AVD_Size __avdNumSkippedCalls               = 0;                                              \
        picoPerfTime __avdDebouncedExecutorCurrentTime     = picoPerfNow();                                  \
        AVD_Double __avdDebouncedExecutorElapsed =                                                           \
            picoPerfDurationMilliseconds(__avdDebouncedExecutorLastTime, __avdDebouncedExecutorCurrentTime); \
        if (__avdDebouncedExecutorElapsed >= (AVD_Double)(intervalMs)) {                                     \
            __avdDebouncedExecutorLastTime = __avdDebouncedExecutorCurrentTime;                              \
            codeBlock;                                                                                       \
            __avdNumSkippedCalls = 0;                                                                        \
        } else {                                                                                             \
            __avdNumSkippedCalls++;                                                                          \
        }                                                                                                    \
    } while (0)

#define AVD_LOG_INIT()            PICO_LOG_INIT()
#define AVD_LOG_SHUTDOWN()        PICO_LOG_SHUTDOWN()
#define AVD_LOG_DEBUG(msg, ...)   PICO_DEBUG(msg, ##__VA_ARGS__)
#define AVD_LOG_VERBOSE(msg, ...) PICO_VERBOSE(msg, ##__VA_ARGS__)
#define AVD_LOG_INFO(msg, ...)    PICO_INFO(msg, ##__VA_ARGS__)
#define AVD_LOG_WARN(msg, ...)    PICO_WARN(msg, ##__VA_ARGS__)
#define AVD_LOG_ERROR(msg, ...)   PICO_ERROR(msg, ##__VA_ARGS__)

#define __AVD_LOG_DEBOUNCED(intervalMs, logFunc, msg, ...)                                      \
    AVD_EXECUTE_DEBOUNCED(intervalMs, {                                                         \
        logFunc("[Debounced] " msg " (skipped %d calls)", ##__VA_ARGS__, __avdNumSkippedCalls); \
    })

#define AVD_LOG_DEBUG_DEBOUNCED(intervalMs, msg, ...) \
    __AVD_LOG_DEBOUNCED(intervalMs, AVD_LOG_DEBUG, msg, ##__VA_ARGS__)
#define AVD_LOG_VERBOSE_DEBOUNCED(intervalMs, msg, ...) \
    __AVD_LOG_DEBOUNCED(intervalMs, AVD_LOG_VERBOSE, msg, ##__VA_ARGS__)
#define AVD_LOG_INFO_DEBOUNCED(intervalMs, msg, ...) \
    __AVD_LOG_DEBOUNCED(intervalMs, AVD_LOG_INFO, msg, ##__VA_ARGS__)
#define AVD_LOG_WARN_DEBOUNCED(intervalMs, msg, ...) \
    __AVD_LOG_DEBOUNCED(intervalMs, AVD_LOG_WARN, msg, ##__VA_ARGS__)
#define AVD_LOG_ERROR_DEBOUNCED(intervalMs, msg, ...) \
    __AVD_LOG_DEBOUNCED(intervalMs, AVD_LOG_ERROR, msg, ##__VA_ARGS__)

#define AVD_CHECK_VK_HANDLE(result, log, ...) \
    if (result == VK_NULL_HANDLE) {           \
        AVD_LOG_ERROR(log, ##__VA_ARGS__);    \
        return false;                         \
    }

#define AVD_VK_CALL(result)                                                                                              \
    {                                                                                                                    \
        VkResult localResult = result;                                                                                   \
        if (localResult != VK_SUCCESS) {                                                                                 \
            AVD_LOG_ERROR("Check [%s] [%s] failed in %s:%d", #result, string_VkResult(localResult), __FILE__, __LINE__); \
            return false;                                                                                                \
        }                                                                                                                \
    }

#define AVD_CHECK_VK_RESULT(result, log, ...)                                                                            \
    {                                                                                                                    \
        VkResult localResult = result;                                                                                   \
        if (localResult != VK_SUCCESS) {                                                                                 \
            AVD_LOG_ERROR("Check [%s] [%s] failed in %s:%d", #result, string_VkResult(localResult), __FILE__, __LINE__); \
            AVD_LOG_ERROR(log, ##__VA_ARGS__);                                                                           \
            return false;                                                                                                \
        }                                                                                                                \
    }

#define AVD_CHECK_MSG(result, log, ...)                                           \
    if (!(result)) {                                                              \
        AVD_LOG_ERROR("Check [%s] failed in %s:%d", #result, __FILE__, __LINE__); \
        AVD_LOG_ERROR(log, ##__VA_ARGS__);                                        \
        return false;                                                             \
    }

#define AVD_CHECK(result)                                                           \
    if (!(result)) {                                                                \
        AVD_LOG_ERROR("Check [%s] failed in %s:%d\n", #result, __FILE__, __LINE__); \
        return false;                                                               \
    }

#ifdef AVD_DEBUG
#define AVD_DEBUG_ONLY(code) \
    do {                     \
        code;                \
    } while (0)
#define AVD_RELEASE_ONLY(code) \
    do {                       \
    } while (0)
#else
#define AVD_DEBUG_ONLY(code) \
    do {                     \
    } while (0)
#define AVD_RELEASE_ONLY(code) \
    do {                       \
        code;                  \
    } while (0)
#endif

#define GAME_WIDTH  1920
#define GAME_HEIGHT 1080

#endif // AVD_BASE_H
