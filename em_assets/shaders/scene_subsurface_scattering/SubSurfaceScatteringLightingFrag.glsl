#version 450
#pragma shader_stage(fragment)

precision highp float;

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in vec4 inPosition;


layout(location = 0) out vec4 outColor;

struct PushConstantData {
    mat4 modelMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;

    int vertexOffset;
    int vertexCount;
    int pad0;
    int pad1;
};

layout(push_constant) uniform PushConstants {
   PushConstantData data;
} pushConstants;



void main() {
    float ambientLight = 0.1; // Example ambient light value
    vec3 normal = normalize(inNormal);
    vec3 lightDirection = normalize(vec3(1.0, 5.0, 1.0)); // Example light direction
    float diffuse = max(dot(normal, lightDirection), 0.0);
    vec3 diffuseColor = vec3(1.0, 1.0, 1.0) * 3.0; // Example diffuse color
    vec3 ambientColor = vec3(0.1, 0.1, 0.1) * 2.0; // Example ambient color
    vec3 finalColor = ambientColor + diffuse * diffuseColor;

    outColor = vec4(finalColor, 1.0); // Set the output color with full opacity


}

