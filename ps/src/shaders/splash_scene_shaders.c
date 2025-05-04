#include "ps_shader.h"

#define PS_SHADER_SPLASH_SCENE_PUSH_CONSTANTS \
"struct PushConstantData {\n"   \
"    float framebufferWidth;\n" \
"    float framebufferHeight;\n"    \
"    float imageWidth;\n"   \
"    float imageHeight;\n"  \
"    float scale;\n"    \
"    float opacity;\n"  \
"};\n"  \
"\n"    \
"layout(push_constant) uniform PushConstants {\n"   \
"    PushConstantData data;\n"  \
"} pushConstants;\n"    \

const char* psShader_SplashSceneVertex = ""
"#version 450\n"
"\n"
"#pragma shader_stage(vertex)\n"
"\n"
"layout(location = 0) out vec2 fragTexCoord;\n"
"\n"
PS_SHADER_SPLASH_SCENE_PUSH_CONSTANTS
"\n"
"const vec2 positions[6] = vec2[](\n"
"    vec2(-1.0, -1.0),\n"
"    vec2(-1.0,  1.0),\n"
"    vec2( 1.0,  1.0),\n"
"    vec2(-1.0, -1.0),\n"
"    vec2( 1.0,  1.0),\n"
"    vec2( 1.0, -1.0)\n"
");\n"
"\n"
"const vec2 texCoords[6] = vec2[](\n"
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
"    fragTexCoord = texCoords[gl_VertexIndex];\n"
"\n"
"    float fbAspect = pushConstants.data.framebufferWidth / pushConstants.data.framebufferHeight;\n"
"    float imgAspect = pushConstants.data.imageWidth      / pushConstants.data.imageHeight;\n"
"    if (fbAspect > imgAspect) {\n"
"        pos.x *= imgAspect / fbAspect;\n"
"    } else {\n"
"        pos.y *= fbAspect / imgAspect;\n"
"    }\n"
"\n"
"    pos *= pushConstants.data.scale;\n"
"    gl_Position = vec4(pos, 0.0, 1.0);\n"
"}\n";

const char* psShader_SplashSceneFragment = ""
"#version 450\n"
"\n"
"#pragma shader_stage(fragment)\n"
"\n"
"layout(binding = 0) uniform sampler2D texSampler;\n"
"layout(location = 0) in vec2 fragTexCoord;\n"
"\n"
PS_SHADER_SPLASH_SCENE_PUSH_CONSTANTS
"\n"
"layout(location = 0) out vec4 outColor;\n"
"\n"
"void main() {\n"
"    vec4 texColor = texture(texSampler, fragTexCoord);\n"
"    outColor = vec4(texColor.rgb, texColor.a * pushConstants.data.opacity);\n"
"}\n";
