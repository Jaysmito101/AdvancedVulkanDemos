#version 450
#pragma shader_stage(fragment)

precision highp float;

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in vec4 inPosition;
layout(location = 5) flat in int inRenderingLight;
// Extremely inefficient thing to do but well, this is just a demo.
layout(location = 6) in vec3 inLightAPos;
layout(location = 7) in vec3 inLightBPos;

layout(location = 0) out vec4 outColor;

struct PushConstantData {
    mat4 modelMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;

    vec4 lightA;
    vec4 lightB;

    int vertexOffset;
    int vertexCount;
    int renderingLight;
    int pad1;
};

layout(push_constant) uniform PushConstants
{
    PushConstantData data;
}
pushConstants;

// light a slight yellow sun color
// light b sky color
const vec3 lightAColor = vec3(1.0, 1.0, 0.8) * 3.0;
const vec3 lightBColor = vec3(0.8, 0.9, 1.0) * 3.0;

void main()
{
    if (inRenderingLight == 1) {
        outColor = vec4(lightAColor, 1.0);
    } else if (inRenderingLight == 2) {
        outColor = vec4(lightBColor, 1.0);
    } else {

        float ambientLight  = 0.1; // Example ambient light value
        vec3 normal         = normalize(inNormal);
        vec3 lightDirection = normalize(vec3(1.0, 5.0, 1.0)); // Example light direction
        float diffuse       = max(dot(normal, lightDirection), 0.0);
        vec3 diffuseColor   = vec3(1.0, 1.0, 1.0) * 3.0; // Example diffuse color
        vec3 ambientColor   = vec3(0.1, 0.1, 0.1) * 2.0; // Example ambient color
        vec3 finalColor     = ambientColor + diffuse * diffuseColor;

        outColor = vec4(finalColor, 1.0); // Set the output color with full opacity
    }
}
