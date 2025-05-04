#include "ps_shader.h"

#define PS_SHADER_LOADING_PUSH_CONSTANTS \
"struct PushConstantData {\n" \
"   float windowWidth;\n" \
"   float windowHeight;\n" \
"   float progress;\n" \
"   float padding;\n" \
"};\n" \
"layout(push_constant) uniform PushConstants {\n" \
"   PushConstantData data;\n" \
"} pushConstants;\n"

const char* psShader_LoadingVertex =
    "#version 450\n"
    "#pragma shader_stage(vertex)\n"
    "layout(location = 0) out vec2 outUV;\n"
    PS_SHADER_LOADING_PUSH_CONSTANTS
    "const vec2 positions[6] = vec2[](\n"
    "    vec2(0.0, 0.0),\n"
    "    vec2(0.0, 1.0),\n"
    "    vec2(1.0, 1.0),\n"
    "    vec2(0.0, 0.0),\n"
    "    vec2(1.0, 1.0),\n"
    "    vec2(1.0, 0.0)\n"
    ");\n"
    "void main() {\n"
    "    vec2 pos = positions[gl_VertexIndex];\n"
    "    // adjust the rect to be a square quad in the center of the screen\n"
    "    outUV = pos;\n"
    "    pos = pos * 2.0 - 1.0;\n"
    "    float aspectRatio = pushConstants.data.windowWidth / pushConstants.data.windowHeight;\n"
    "    if (aspectRatio > 1.0) {\n"
    "        pos.x /= aspectRatio;\n"
    "    } else {\n"
    "        pos.y *= aspectRatio;\n"
    "    }\n"   
    "    pos *= 2.0;\n"
    "    gl_Position = vec4(pos, 0.0, 1.0);\n"
    "}\n";

const char* psShader_LoadingFragment =
    "#version 450\n"
    "#pragma shader_stage(fragment)\n"
    "precision highp float;\n" // Use high precision
    "\n"
    "layout(location = 0) in vec2 inUV;\n" // Input UV coordinates (0 to 1)
    "\n"
    PS_SHADER_LOADING_PUSH_CONSTANTS // Include push constant struct and uniform
    "\n"
    "layout(location = 0) out vec4 outColor;\n" // Output fragment color
    "\n"
    "// --- SDF Definitions ---\n"
    "float sdCapsule( vec2 p, vec2 a, vec2 b, float r ) {\n"
    "    vec2 pa = p - a, ba = b - a;\n"
    "    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );\n"
    "    return length( pa - ba*h ) - r;\n"
    "}\n"
    "\n"
    "\n"
    "// --- Main Shader Logic ---\n"
    "void main() {\n"
    "    // Center UVs and adjust aspect ratio\n"
    "    vec2 uv = (inUV * 2.0 - 1.0);\n" // Range -1 to 1, centered at 0,0
    "\n"
    "    vec4 backgroundColor = vec4(0.0); // rgb(208, 166, 228)\n"
    "    vec4 barBackgroundColor = vec4(180.0/255.0, 141.0/255.0, 209.0/255.0, 1.0); // rgb(180, 141, 209) - Track\n"
    "    vec4 barForegroundColor = vec4(79.0/255.0, 45.0/255.0, 95.0/255.0, 1.0);  // rgb(79, 45, 95) - Fill\n"
    "    vec4 trackBackgroundColor = vec4(130.0/255.0, 88.0/255.0, 150.0/255.0, 1.0);  // rgb(130, 88, 150) - Fill\n"
    "    vec4 borderColor = vec4(86.0/255.0, 16.0/255.0, 113.0/255.0, 1.0);       // rgb(86, 16, 113) - Border\n"
    "\n"
    "    // --- Define the capsule shape parameters ---\n"
    "    float barWidth = 1.0;  // Width relative to normalized height (adjust as needed)\n"
    "    float barHeight = 0.1; // Height/radius relative to normalized height (adjust as needed)\n"
    "    float borderThickness = 0.015; // Thickness of the border (adjust as needed)\n"
    "\n"
    "    // --- Define the capsule's centerline endpoints ---\n"
    "    vec2 capsuleStart = vec2(-barWidth / 2.0, 0.0);\n"
    "    vec2 capsuleEnd = vec2(barWidth / 2.0, 0.0);\n"
    "    vec2 capsuleProgressEnd = vec2(-barWidth / 2.0 + barWidth * pushConstants.data.progress, 0.0);\n"
    "\n"
    "    // --- Calculate SDF distances ---\n"
    "    float capsuleOuter = sdCapsule(uv, capsuleStart, capsuleEnd, barHeight);\n"
    "    float capsuleInner = sdCapsule(uv, capsuleStart, capsuleEnd, barHeight - borderThickness);\n"
    "    float capsuleTrack = sdCapsule(uv, capsuleStart, capsuleEnd, barHeight - 2.0 * borderThickness);\n"
    "    float capsuleProgress = sdCapsule(uv, capsuleStart, capsuleProgressEnd, barHeight - 2.0 * borderThickness);\n"
    "    float capsuleProgressInner = sdCapsule(uv, capsuleStart, capsuleProgressEnd, barHeight - 3.0 * borderThickness);\n"
    "\n"
    "    vec4 finalColor = backgroundColor;\n"
    "\n"
    "    float aa = 1.5 / pushConstants.data.windowHeight;\n"
    "\n"
    "    float borderShape = max(capsuleOuter, -capsuleInner);\n"
    "    float progressBorderShape = max(capsuleProgress, -capsuleProgressInner);\n"
    "    finalColor = mix(finalColor, barBackgroundColor, smoothstep(-aa, aa, capsuleOuter));\n"
    "    finalColor = mix(finalColor, borderColor, smoothstep(aa, -aa, borderShape));\n"
    "    finalColor = mix(finalColor, trackBackgroundColor, smoothstep(-aa, aa, -capsuleTrack));\n"
    "    finalColor = mix(finalColor, barForegroundColor, smoothstep(-aa, aa, -capsuleProgress));\n"
    "    finalColor = mix(finalColor, borderColor, smoothstep(aa, -aa, progressBorderShape));\n"
    "\n"
    "\n"
    "    outColor = finalColor;\n"
    "}\n";
