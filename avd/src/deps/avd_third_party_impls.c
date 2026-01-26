// #define CUTE_SOUND_IMPLEMENTATION
// #include "cute_sound.h"

#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "tinyobj_loader_c.h"

#ifdef AVD_DEBUG
// This one is purely for the purpose of debugging (using DebugPrint functions of picoM3U8)
#define PICO_M3U8_LOG(...)   \
    do {                     \
        printf(__VA_ARGS__); \
        printf("\n");        \
    } while (0)

#endif

#define PICO_ASSERT(expr)                                                               \
    {                                                                                   \
        int __assertionResult##__LINE__ = expr;                                         \
        if (!__assertionResult##__LINE__) {                                             \
            printf("PICO ASSERTION FAILED: %s, in %s:%d\n", #expr, __FILE__, __LINE__); \
            assert(__assertionResult##__LINE__);                                        \
        }                                                                               \
    }

#define PICO_LOG_THREAD_SAFE
#define PICO_IMPLEMENTATION
#include "pico/picoH264.h"
#include "pico/picoLog.h"
#include "pico/picoM3U8.h"
#include "pico/picoMpegTS.h"
#include "pico/picoPerf.h"
#include "pico/picoThreads.h"

