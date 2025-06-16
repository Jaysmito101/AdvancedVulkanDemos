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



void main() {
   // vec2 uv = (inUV * 2.0 - 1.0);
   vec4 color = texture(textures[3], vec2(inUV.x, 1.0 - inUV.y));
   outColor = vec4(color.rgb, 1.0); 
}

