#version 450

#pragma shader_stage(fragment)

layout(binding = 0) uniform sampler2D texSampler;
layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;


struct PushConstantData {
    float framebufferWidth;
    float framebufferHeight;
    float imageWidth;
    float imageHeight;
    float scale;
    float opacity;
};

layout(push_constant) uniform PushConstants {
    PushConstantData data;
} pushConstants;


void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);
    vec3 color = vec3(215.0f / 255.0f, 204.0f / 255.0f, 246.0f / 255.0f);
    texColor.rgb = mix(color, texColor.rgb, pushConstants.data.opacity);
    outColor = vec4(texColor.rgb, texColor.a);
}