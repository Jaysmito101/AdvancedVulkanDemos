struct VertexShaderInput {
    float2 position : POSITION;
    float2 texCoord : TEXCOORD;
};

struct VertexShaderOutput {
    float2 texCoord : TEXCOORD;
    float4 position : SV_Position;
};

struct PushConstantData {
    float fragmentBufferWidth;
    float fragmentBufferHeight;
    float scale;
    float opacity;
    float offsetX;
    float offsetY;
    float pxRange;
    float texSize;
    float4 color;
};

[[vk::push_constant]]
cbuffer PushConstants {
    PushConstantData data;
};

VertexShaderOutput main(VertexShaderInput input) {
    float2 offset = float2(data.offsetX, data.offsetY);
    float2 pos = input.position * data.scale + offset;
    pos /= float2(data.fragmentBufferWidth, data.fragmentBufferHeight);
    pos = pos * 2.0 - 1.0;

    VertexShaderOutput output;
    output.texCoord = input.texCoord;
    output.position = float4(pos, 0.0, 1.0);
    return output;
}