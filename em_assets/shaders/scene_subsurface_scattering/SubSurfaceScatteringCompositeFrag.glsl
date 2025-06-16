#version 450
#pragma shader_stage(fragment)

precision highp float;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout (set = 0, binding = 1) uniform sampler2D textures[];

struct PushConstantData {
   mat4 modelMatrix;
   mat4 viewMatrix;
   mat4 projectionMatrix;
};

layout(push_constant) uniform PushConstants {
   PushConstantData data;
} pushConstants;

float linearizeDepth(float depth) {
   float zNear = 0.1; // Near plane distance
   float zFar = 100.0; // Far plane distance
   return zNear * zFar / (zFar - depth * (zFar - zNear));
}

void main() {
   vec2 uv = (inUV * 2.0 - 1.0);
   vec4 color = texture(textures[3], vec2(inUV.x, 1.0 - inUV.y));
   outColor = vec4(color.rgb, 1.0); 

   // float color = texture(textures[11], vec2(inUV.x, 1.0 - inUV.y)).r;
   // outColor = vec4(vec3(linearizeDepth(color)), 1.0); // Set the output color with full opacity
}

