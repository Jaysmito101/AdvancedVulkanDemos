#version 450
#pragma shader_stage(vertex)

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outRayDir;

struct PushConstantData {
    float windowWidth;
    float windowHeight;
    float sphereRadius;
    float time;
    float spherePosition[3];
    float cameraPosition[3];
    float rotation;
};

layout(push_constant) uniform PushConstants {
    PushConstantData data;
} pushConstants;

void main() {
    // Generate a fullscreen quad
    vec2 positions[6] = vec2[](
        vec2(0.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 1.0),
        vec2(0.0, 0.0),
        vec2(1.0, 1.0),
        vec2(1.0, 0.0)
    );
    
    vec2 pos = positions[gl_VertexIndex];
    outUV = pos;
    
    // Transform to clip space
    pos = pos * 2.0 - 1.0;
    gl_Position = vec4(pos, 0.0, 1.0);
    
    // Calculate ray direction for the fragment shader
    float aspectRatio = pushConstants.data.windowWidth / pushConstants.data.windowHeight;
    vec3 rayDir;
    rayDir.x = pos.x * aspectRatio;
    rayDir.y = pos.y;
    rayDir.z = -1.0; // Looking down negative z-axis
    
    // Apply camera rotation
    float c = cos(pushConstants.data.rotation);
    float s = sin(pushConstants.data.rotation);
    vec3 rotatedRay;
    rotatedRay.x = rayDir.x * c - rayDir.z * s;
    rotatedRay.y = rayDir.y;
    rotatedRay.z = rayDir.x * s + rayDir.z * c;
    
    outRayDir = normalize(rotatedRay);
}
