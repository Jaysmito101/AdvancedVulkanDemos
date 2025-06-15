# Advanced Vulkan Demos (AVD) - Style Guide

## Table of Contents

1. [General Philosophy](#general-philosophy)
2. [File Organization](#file-organization)
3. [Naming Conventions](#naming-conventions)
4. [Code Style](#code-style)
5. [Documentation](#documentation)
6. [Architecture Patterns](#architecture-patterns)
7. [Build System](#build-system)
8. [Assets and Resources](#assets-and-resources)
9. [Testing](#testing)
10. [Platform Support](#platform-support)

## General Philosophy

The Advanced Vulkan Demos project follows these core principles:

- **Minimalist Dependencies**: Prefer standard libraries and avoid unnecessary external dependencies
- **Clear Separation of Concerns**: Each module has a single, well-defined responsibility  
- **Cross-Platform Compatibility**: Write code that works on Windows, Linux, and potentially other platforms
- **Performance-Conscious**: Optimize for runtime performance while maintaining code clarity
- **Educational Value**: Code should be readable and serve as learning material for Vulkan development

## File Organization

### Directory Structure

```
avd/
├── include/           # Public header files
│   ├── core/         # Core system headers
│   ├── vulkan/       # Vulkan abstraction headers
│   ├── math/         # Mathematics library headers
│   ├── font/         # Font rendering headers
│   ├── scenes/       # Scene management headers
│   ├── ui/           # User interface headers
│   └── shader/       # Shader compilation headers
├── src/              # Implementation files
│   ├── core/         # Core system implementations
│   ├── vulkan/       # Vulkan abstraction implementations
│   ├── math/         # Mathematics library implementations
│   ├── font/         # Font rendering implementations
│   ├── scenes/       # Scene management implementations
│   ├── ui/           # User interface implementations
│   ├── shader/       # Shader compilation implementations
│   └── deps/         # Third-party implementations
em_assets/            # Embedded assets (fonts, shaders, images)
assets/              # External assets
tools/               # Build and asset generation tools
```

### Header Guards

All header files must use include guards with the pattern:

```c
#ifndef MODULE_HEADER_H
#define MODULE_HEADER_H

// Content here

#endif // MODULE_HEADER_H
```

### Include Order

1. System headers (standard C library)
2. Platform-specific headers (Windows, GLFW, Vulkan)
3. Third-party library headers
4. Project headers (own modules)

```c
// System headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Platform headers
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

// Third-party headers
#include "stb_image.h"

// Project headers
#include "core/avd_core.h"
#include "vulkan/avd_vulkan.h"
```

## Naming Conventions

### Prefix System

All public symbols use the `AVD_` or `avd` prefix to avoid namespace pollution:

- **Types**: `AVD_PascalCase` (e.g., `AVD_Vector3`, `AVD_VulkanImage`)
- **Functions**: `avdCamelCase` (e.g., `avdVulkanInit`, `avdListPushBack`)
- **Constants/Macros**: `AVD_UPPER_SNAKE_CASE` (e.g., `AVD_ARRAY_COUNT`, `AVD_MAX_FONTS`)
- **Enums**: `AVD_ENUM_NAME_VALUE` (e.g., `AVD_SCENE_TYPE_MAIN_MENU`)

### Internal/Private Symbols

Private symbols use double underscore prefix:

```c
// Private functions
static bool __avdSetupDescriptors(VkDescriptorSetLayout *layout, AVD_Vulkan *vulkan);
static void __avdApplicationUpdateFramerateCalculation(AVD_Frametime *framerateInfo);

// Private callbacks
void __avdGLFWErrorCallback(int error, const char *description);
```

### File Naming

- Headers: `avd_module_name.h`
- Sources: `avd_module_name.c`
- Tests: `avd_module_name_tests.c`

### Variable Naming

- **Local variables**: `camelCase`
- **Structure members**: `camelCase`
- **Global variables**: Avoid; use static when necessary
- **Constants**: `UPPER_SNAKE_CASE`

```c
// Good examples
AVD_Vector3 cameraPosition;
float deltaTime;
size_t vertexCount;
const float AVD_PI = 3.14159265358979323846f;
```

## Code Style

### Formatting

- **Indentation**: 4 spaces (no tabs)
- **Line length**: 120 characters maximum
- **Braces**: Allman style (opening brace on new line)

```c
bool avdApplicationInit(AVD_AppState *appState)
{
    AVD_ASSERT(appState != NULL);

    appState->running = true;

    if (!avdWindowInit(&appState->window, appState))
    {
        AVD_LOG("Failed to initialize window\n");
        return false;
    }

    return true;
}
```

### Function Declarations

- Parameter alignment for long function signatures:

```c
void avdUiDrawRect(
    VkCommandBuffer commandBuffer,
    AVD_Ui *ui,
    struct AVD_AppState *appState,
    float x,
    float y,
    float width,
    float height,
    float r,
    float g,
    float b,
    float a,
    VkDescriptorSet descriptorSet,
    uint32_t imageWidth,
    uint32_t imageHeight);
```

### Structure Definitions

Use descriptive member names and consistent alignment:

```c
typedef struct AVD_Vector3 {
    AVD_Float x;
    AVD_Float y;
    AVD_Float z;
} AVD_Vector3;

typedef struct AVD_FontRendererPushConstants {
    float frameBufferWidth;
    float frameBufferHeight;
    float scale;
    float opacity;
    float offsetX;
    float offsetY;
    float pxRange;
    float texSize;
    float colorR;
    float colorG;
    float colorB;
    float colorA;
} AVD_FontRendererPushConstants;
```

### Macros and Inline Functions

Prefer functions over macros when possible. Use macros for simple operations and constants:

```c
// Simple macros
#define AVD_ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))
#define AVD_MIN(a, b) ((a) < (b) ? (a) : (b))
#define AVD_MAX(a, b) ((a) > (b) ? (a) : (b))

// Complex operations as inline functions
static inline AVD_Vector3 avdVec3Normalize(const AVD_Vector3 v) {
    AVD_Float length = avdVec3Length(v);
    return length > 0.0f ? avdVec3Scale(v, 1.0f / length) : avdVec3Zero();
}
```

### Error Handling

Use consistent error handling patterns:

```c
// Check macros for validation
#define AVD_CHECK(result) \
    if (!(result)) { \
        AVD_LOG("------------------------------------\n"); \
        AVD_LOG("Check [%s] failed in %s:%d\n", #result, __FILE__, __LINE__); \
        AVD_LOG("------------------------------------\n"); \
        return false; \
    }

// Usage
AVD_CHECK(avdWindowInit(&appState->window, appState));
AVD_CHECK(avdVulkanInit(&appState->vulkan));
```

### Memory Management

- Always check allocation results
- Pair every allocation with deallocation
- Use `avdListDestroy` pattern for cleanup functions

```c
void avdListCreate(AVD_List *list, size_t itemSize)
{
    assert(list != NULL);
    assert(itemSize > 0);

    list->itemSize = itemSize;
    list->capacity = AVD_LIST_INITIAL_CAPACITY;
    list->count = 0;
    list->items = malloc(list->capacity * itemSize);
    assert(list->items != NULL);
}

void avdListDestroy(AVD_List *list)
{
    if (list && list->items) {
        free(list->items);
        list->items = NULL;
        list->count = 0;
        list->capacity = 0;
    }
}
```

## Documentation

### Header Comments

All public headers should have descriptive file headers:

```c
/**
 * avd_math.h
 * 
 * Mathematics library for 3D graphics operations including vectors,
 * matrices, and common mathematical functions. Provides both SIMD
 * and non-SIMD implementations.
 */
```

### Function Documentation

Document complex functions with clear parameter descriptions:

```c
/**
 * Compiles GLSL shader code to SPIR-V bytecode
 * 
 * @param shaderCode The GLSL source code as null-terminated string
 * @param inputFileName The filename for error reporting
 * @param outSize Pointer to receive the size of compiled bytecode
 * @return Pointer to compiled SPIR-V bytecode, or NULL on failure
 */
uint32_t *avdCompileShader(const char *shaderCode, const char *inputFileName, size_t *outSize);
```

### Inline Comments

Use comments to explain complex algorithms or non-obvious code:

```c
// Convert from NDC to screen coordinates
pos = pos * 2.0 - 1.0;

// Apply aspect ratio correction for proper scaling
if (windowAspect > framebufferAspect) {
    pos.x *= framebufferAspect / windowAspect;
} else {
    pos.y *= windowAspect / framebufferAspect;
}
```

## Architecture Patterns

### Module System

Each module follows a consistent pattern:

1. **Public Interface**: Clean header with minimal dependencies
2. **Private Implementation**: Internal functions and state
3. **Initialization/Cleanup**: Paired init/destroy functions
4. **Error Handling**: Consistent return codes and logging

### Object-Oriented Style in C

Use struct-based "objects" with associated functions:

```c
// "Class" definition
typedef struct AVD_List {
    size_t itemSize;
    size_t capacity;
    size_t count;
    void *items;
} AVD_List;

// "Methods"
void avdListCreate(AVD_List *list, size_t itemSize);
void avdListDestroy(AVD_List *list);
void *avdListPushBack(AVD_List *list, const void *item);
```

### Scene Management

Scenes use a plugin-like architecture with function pointers:

```c
typedef struct AVD_SceneAPI {
    const char* displayName;
    const char* id;
    
    AVD_SceneCheckIntegrityFn checkIntegrity;
    AVD_SceneInitFn init;
    AVD_SceneRenderFn render;
    AVD_SceneUpdateFn update;
    AVD_SceneDestroyFn destroy;
    AVD_SceneLoaderFn load;
    AVD_SceneInputEventFn inputEvent;
} AVD_SceneAPI;
```

### Resource Management

Resources follow RAII-like patterns:

```c
// Consistent initialization pattern
bool avdResourceInit(AVD_Resource *resource, const AVD_ResourceDesc *desc);

// Consistent cleanup pattern  
void avdResourceDestroy(AVD_Resource *resource);

// Usage pattern
AVD_Resource resource = {0};
if (avdResourceInit(&resource, &desc)) {
    // Use resource
    avdResourceDestroy(&resource);
}
```

## Build System

### CMake Configuration

- Use modern CMake (3.12+)
- Separate target for each major component
- Clear dependency management
- Cross-platform support

```cmake
# Feature-based configuration
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

# Platform-specific definitions
if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
elseif(LINUX)
    # Linux-specific settings
endif()
```

### Asset Pipeline

Assets are processed at build time:

1. Raw assets in `assets/` and `em_assets/`
2. Python tool generates embedded C code
3. Generated code in `avd_assets/generated/`
4. Automatic regeneration on asset changes

## Assets and Resources

### Shader Organization

- Vertex shaders: `*Vert.glsl`
- Fragment shaders: `*Frag.glsl`
- Compute shaders: `*Comp.glsl`
- Use `#pragma shader_stage()` for clarity

### Asset Naming

- Use descriptive names: `MainMenuFrag.glsl`, `FontRendererVert.glsl`
- Group related assets in folders
- Generate C identifiers automatically from filenames

### Resource Loading

Resources are embedded at compile time for simplicity:

```c
// Generated asset access
extern const uint8_t avd_asset_shader_MainMenuFrag_glsl[];
extern const size_t avd_asset_shader_MainMenuFrag_glsl_size;
```

## Testing

### Unit Tests

Each module should have associated tests:

```c
// Test function naming
bool avdMathTestsRun(void);
bool avdListTestsRun(void);

// Test implementation pattern
static bool testVectorOperations(void) {
    AVD_Vector3 v1 = avdVec3(1.0f, 2.0f, 3.0f);
    AVD_Vector3 v2 = avdVec3(4.0f, 5.0f, 6.0f);
    
    // Test specific operations
    if (!avdIsFEqual(avdVec3Dot(v1, v2), expectedValue)) {
        AVD_LOG("FAILED: Vector dot product test\n");
        return false;
    }
    
    return true;
}
```

### Debug Mode

Tests run automatically in debug builds:

```c
#ifdef AVD_DEBUG
    AVD_CHECK(avdMathTestsRun());
    AVD_CHECK(avdListTestsRun());
#endif
```

## Platform Support

### Compiler Support

- **Primary**: MSVC (Visual Studio 2019+)
- **Secondary**: GCC, Clang
- **Standard**: C11 for C code, C++20 for any C++ code

### Platform Abstractions

Use platform-specific code sparingly and isolate it:

```c
// Platform-specific includes
#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// Platform-specific implementations
const char *avdGetTempDirPath(void)
{
    static char tempDirPath[1024];
#ifdef _WIN32
    GetTempPathA(1024, tempDirPath);
#else
    strcpy(tempDirPath, "/tmp/");
#endif
    return tempDirPath;
}
```

### Vulkan Version

- Target Vulkan 1.2 minimum
- Use Volk for dynamic loading
- Platform-specific surface creation

---

## Conclusion

This style guide ensures consistency, maintainability, and readability across the Advanced Vulkan Demos project. When in doubt, follow existing patterns in the codebase and prioritize clarity over cleverness.

For questions or clarifications about this style guide, refer to existing code examples or reach out to the project maintainers.