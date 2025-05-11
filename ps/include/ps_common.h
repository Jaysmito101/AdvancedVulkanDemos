#ifndef PS_COMMON_H
#define PS_COMMON_H


// third party includes
#include "glfw/glfw3.h"

#define API_VERSION VK_API_VERSION_1_2
#include <vulkan/vulkan.h>
 
// ps includes
#include "ps_base.h"
#include "ps_font.h"
#include "ps_game_state.h"

// common functions
uint32_t psHashBuffer(const void *buffer, size_t size);
uint32_t psHashString(const char *str);
void psPrintShaderWithLineNumbers(const char *shaderCode, const char *shaderName);
const char* psGetTempDirPath(void);

#endif // PS_COMMON_H