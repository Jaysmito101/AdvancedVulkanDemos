#ifndef PS_SHADERS_H
#define PS_SHADERS_H

#include "ps_common.h"
#include "ps_shader_compiler.h"
#include "ps_vulkan.h"

extern const char* psShader_PresentationVertex;
extern const char* psShader_PresentationFragment;

extern const char* psShader_SplashSceneVertex;
extern const char* psShader_SplashSceneFragment;

extern const char* psShader_LoadingVertex;
extern const char* psShader_LoadingFragment;

extern const char* psShader_MainMenuVertex;
extern const char* psShader_MainMenuFragment;

extern const char* psShader_PrologueVertex;
extern const char* psShader_PrologueFragment;

VkShaderModule psShaderModuleCreate(PS_GameState *gameState, const char* shaderCode, VkShaderStageFlagBits shaderType, const char* inputFileName);


#define PS_SHADER_ACES "\n" \
"vec3 aces(vec3 x) {\n" \
"    const float a = 2.51;\n" \
"    const float b = 0.03;\n" \
"    const float c = 2.43;\n" \
"    const float d = 0.59;\n" \
"    const float e = 0.14;\n" \
"    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);\n" \
"}\n" \
"\n"

#define PS_SHADER_UTILS \
"\n" \
"float sdCapsule( vec2 p, vec2 a, vec2 b, float r ) {\n" \
"    vec2 pa = p - a, ba = b - a;\n" \
"    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );\n" \
"    return length( pa - ba*h ) - r;\n" \
"}\n" \
"\n" \
"float random(vec2 st) {\n" \
"    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);\n" \
"}\n" 

#endif // PS_SHADERS_H
