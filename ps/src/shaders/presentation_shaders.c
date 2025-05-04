#include "ps_shader.h"

#define PS_SHADER_PRESENTATION_PUSH_CONSTANTS \
"struct PushConstantData {\n"   \
"    float windowWidth;\n" \
"    float windowHeight;\n"    \
"    float framebufferWidth;\n"   \
"    float framebufferHeight;\n"  \
"    float circleRadius;\n"    \
"    float iconWidth;\n" \
"    float iconHeight;\n" \
"    float time;\n" \
"};\n"  \
"\n"    \
"layout(push_constant) uniform PushConstants {\n"   \
"    PushConstantData data;\n"  \
"} pushConstants;\n"

const char* psShader_PresentationVertex = ""
"\n"
"#version 450\n"
"\n"
"#pragma shader_stage(vertex)\n"
"\n"
"layout (location = 0) out vec2 outUV;\n"
"\n"
PS_SHADER_PRESENTATION_PUSH_CONSTANTS
"\n"
"const vec2 positions[6] = vec2[](\n"
"    vec2(0.0, 0.0),\n"
"    vec2(0.0, 1.0),\n"
"    vec2(1.0, 1.0),\n"
"    vec2(0.0, 0.0),\n"
"    vec2(1.0, 1.0),\n"
"    vec2(1.0, 0.0)\n"
");\n"
"\n"
"void main() {\n"
"    vec2 pos = positions[gl_VertexIndex];\n"
"\n"
"    outUV = pos;\n"
"\n"
"    pos = pos * 2.0 - 1.0;\n"
"\n"
"    float windowAspect = pushConstants.data.windowWidth / pushConstants.data.windowHeight;\n"
"    float framebufferAspect = pushConstants.data.framebufferWidth / pushConstants.data.framebufferHeight;\n"
"    if (windowAspect > framebufferAspect) {\n"
"        pos.x *= framebufferAspect / windowAspect;\n"
"    } else {\n"
"        pos.y *= windowAspect / framebufferAspect;\n"
"    }\n"
"    gl_Position = vec4(pos, 0.0, 1.0);\n"
"\n"
"}\n";

const char* psShader_PresentationFragment = ""
"#version 450\n"
"\n"
"#pragma shader_stage(fragment)\n"
"\n"
"layout (set=0, binding = 0) uniform sampler2D sceneFramebuffer;\n"
"layout (set=1, binding = 0) uniform sampler2D iconTexture;\n"
"\n"
"layout (location = 0) in vec2 inUV;\n"
"\n"
"layout (location = 0) out vec4 outColor;\n"
"\n"
PS_SHADER_PRESENTATION_PUSH_CONSTANTS
"\n"
"void main() \n"
"{\n" 
"    vec2 ndc = inUV * 2.0 - 1.0;\n" // Convert UV to NDC [-1, 1]\n"
"\n"
"    float framebufferAspect = pushConstants.data.framebufferWidth / pushConstants.data.framebufferHeight;\n"
"    ndc.x *= framebufferAspect;"
"\n"
"    float dist = length(ndc);\n"
"\n"
"    vec3 overlayColor = vec3(215.0 / 255.0, 204.0 / 255.0, 246.0 / 255.0);\n"
"\n"
"    outColor = texture(sceneFramebuffer, inUV);\n"
"    float overlayAlpha = smoothstep(pushConstants.data.circleRadius, pushConstants.data.circleRadius + 0.01, dist);\n"
"    outColor.rgb = mix(outColor.rgb, overlayColor, overlayAlpha);\n"
"    outColor.a = 1.0;\n"
"    if (dist <= 0.155 && pushConstants.data.circleRadius <= 0.1) {\n"
"        float timefactor = smoothstep(-1.0, 1.0, sin(pushConstants.data.time * 20.0));\n"
"        float angle = sin(timefactor) * 0.2;\n"
"        float cosAngle = cos(angle);\n"
"        float sinAngle = sin(angle);\n"
"        mat2 rotationMatrix = mat2(cosAngle, -sinAngle, sinAngle, cosAngle);\n"
"        vec2 rotatedNdc = rotationMatrix * ndc;\n"
"        vec2 iconUV = rotatedNdc * 6.5 * 0.5 + 0.5;\n"
"        vec4 iconColor = texture(iconTexture, iconUV);\n"
"        float opacity = 1.0 - smoothstep(0.1, 0.15, pushConstants.data.circleRadius);\n"
"        outColor = mix(outColor, iconColor, iconColor.a * opacity);\n"
"    }\n"
"}\n";

