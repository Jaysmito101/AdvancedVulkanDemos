#ifndef AVD_COMMON_H
#define AVD_COMMON_H


// third party includes
#include "glfw/glfw3.h"

#define API_VERSION VK_API_VERSION_1_2
#include <vulkan/vulkan.h>
 
// ps includes
#include "avd_base.h"
#include "avd_font.h"
#include "avd_types.h"

// common functions
uint32_t avdHashBuffer(const void *buffer, size_t size);
uint32_t avdHashString(const char *str);
void avdPrintShaderWithLineNumbers(const char *shaderCode, const char *shaderName);
const char* avdGetTempDirPath(void);

#endif // AVD_COMMON_H