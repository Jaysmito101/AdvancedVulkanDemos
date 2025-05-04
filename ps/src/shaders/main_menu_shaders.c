#include "ps_shader.h"

#define PS_SHADER_MAIN_MENU_PUSH_CONSTANTS "" \
"\n" \
"struct PushConstantsData {\n" \
"    float windowWidth;\n" \
"    float windowHeight;\n" \
"    float time;\n" \
"};\n" \
"\n" \
"layout(push_constant) uniform PushConstants {\n" \
"    PushConstantsData data;\n" \
"} pushConstants;\n" \
"\n" \

#define PS_SDF_FUNCTIONS "" \
"\n" \
"float sdBox(vec2 p, vec2 b) {\n" \
"    vec2 d = abs(p) - b;\n" \
"    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);\n" \
"}\n" \
"float opSmoothUnion( float d1, float d2, float k ) {\n" \
"    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );\n" \
"    return mix( d2, d1, h ) - k*h*(1.0-h);\n" \
"}\n" \
"\n"


const char* psShader_MainMenuVertex = ""
"#version 450\n"
"#pragma shader_stage(vertex)\n"
"layout(location = 0) out vec2 outUV;\n"
"const vec2 positions[6] = vec2[](\n"
"    vec2(-1.0, -1.0),\n"
"    vec2(-1.0,  1.0),\n"
"    vec2( 1.0,  1.0),\n"
"    vec2(-1.0, -1.0),\n"
"    vec2( 1.0,  1.0),\n"
"    vec2( 1.0, -1.0)\n"
");\n"
"const vec2 uvs[6] = vec2[](\n"
"    vec2(0.0, 1.0),\n"
"    vec2(0.0, 0.0),\n"
"    vec2(1.0, 0.0),\n"
"    vec2(0.0, 1.0),\n"
"    vec2(1.0, 0.0),\n"
"    vec2(1.0, 1.0)\n"
");\n"
"void main() {\n"
"    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);\n"
"    outUV = uvs[gl_VertexIndex];\n"
"}\n";

const char* psShader_MainMenuFragment = ""
"#version 450\n"
"#pragma shader_stage(fragment)\n"
"layout(location = 0) in vec2 inUV;\n"
"layout(location = 0) out vec4 outColor;\n"
PS_SHADER_MAIN_MENU_PUSH_CONSTANTS 
PS_SDF_FUNCTIONS
"void main() {\n"
"    vec2 fragCoord = inUV * vec2(pushConstants.data.windowWidth, pushConstants.data.windowHeight);\n"
"    vec2 center = vec2(pushConstants.data.windowWidth, pushConstants.data.windowHeight) * 0.5;\n"
"    vec2 p = fragCoord - center;\n"

"    float aspect = pushConstants.data.windowWidth / pushConstants.data.windowHeight;\n"
"    p.x /= aspect; // Adjust for aspect ratio \n"
"    vec2 buttonSize = vec2(pushConstants.data.windowHeight * 0.3, pushConstants.data.windowHeight * 0.06); // Width based on height for consistency \n"
"    float buttonSpacing = pushConstants.data.windowHeight * 0.08;\n"
"    vec2 buttonPositions[4];\n"
"    buttonPositions[0] = vec2(0.0, buttonSpacing * 1.5); // New Game (Top) \n"
"    buttonPositions[1] = vec2(0.0, buttonSpacing * 0.5); // Continue \n"
"    buttonPositions[2] = vec2(0.0, -buttonSpacing * 0.5); // Options \n"
"    buttonPositions[3] = vec2(0.0, -buttonSpacing * 1.5); // Exit (Bottom) \n"
"    float d = 1e10; // Initialize with a large distance\n"
"    for (int i = 0; i < 4; ++i) {\n"
"        float buttonDist = sdBox(p - buttonPositions[i], buttonSize);\n"
"        d = min(d, buttonDist);\n"
"    }\n"

"    vec3 col = vec3(40.0/255.0, 40.0/255.0, 40.0/255.0); // Dark grey background \n"

"    vec3 buttonColor = vec3(0.6, 0.6, 0.6); // Grey buttons \n"
"    float edgeSmoothness = 2.0; // Adjust for desired edge sharpness \n"
"    col = mix(buttonColor, col, smoothstep(0.0, edgeSmoothness, d));\n"

"    outColor = vec4(col, 1.0);\n"
"}\n";
