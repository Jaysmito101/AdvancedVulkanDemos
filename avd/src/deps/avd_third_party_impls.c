// #define CUTE_SOUND_IMPLEMENTATION
// #include "cute_sound.h"

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
#define PICO_M3U8_LOG(...) do { \
    printf(__VA_ARGS__); \
    printf("\n"); \
 } while(0)

#endif

#define PICO_LOG_THREAD_SAFE
#define PICO_IMPLEMENTATION
#include "pico/picoLog.h"
#include "pico/picoM3U8.h"
#include "pico/picoThreads.h"