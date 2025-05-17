
#version 450

#pragma shader_stage(vertex)

layout(location = 0) out vec2 fragTexCoord;

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

const vec2 positions[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2( 1.0, -1.0)
);

void main() {
    vec2 pos = positions[gl_VertexIndex];
    fragTexCoord = pos * 0.5 + 0.5;

    float fbAspect = pushConstants.data.framebufferWidth / pushConstants.data.framebufferHeight;
    float imgAspect = pushConstants.data.imageWidth      / pushConstants.data.imageHeight;
    if (fbAspect > imgAspect) {
        pos.x *= imgAspect / fbAspect;
    } else {
        pos.y *= fbAspect / imgAspect;
    }

    pos *= pushConstants.data.scale;
    gl_Position = vec4(pos, 0.0, 1.0);
}