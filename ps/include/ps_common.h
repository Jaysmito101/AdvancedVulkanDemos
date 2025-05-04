#ifndef PS_COMMON_H
#define PS_COMMON_H

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
 
// ps includes
#include "ps_game_state.h"

// common macros
#define PS_ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))
#define PS_MIN(a, b) ((a) < (b) ? (a) : (b))
#define PS_MAX(a, b) ((a) > (b) ? (a) : (b))
#define PS_CLAMP(value, min, max) (PS_MIN(PS_MAX(value, min), max))
#define PS_SWAP(a, b) { typeof(a) temp = a; a = b; b = temp; }
#define PS_OFFSET_OF(type, member) ((size_t)&(((type *)0)->member))
#define PS_ASSERT(condition) assert(condition)
#define PS_LOG(msg, ...) printf(msg, ##__VA_ARGS__)

#define GAME_WIDTH 1920
#define GAME_HEIGHT 1080

// common functions
uint32_t psHashBuffer(const void *buffer, size_t size);
uint32_t psHashString(const char *str);

const char* psGetTempDirPath(void);

#endif // PS_COMMON_H