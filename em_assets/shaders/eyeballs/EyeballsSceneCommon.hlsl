#ifndef EYEBALLS_SCENE_COMMON
#define EYEBALLS_SCENE_COMMON

#include "EyeballUtils"

struct VertexTransfer {
    float4 position : SV_Position;
    [[vk::location(0)]] float2 uv : TEXCOORD0;
    [[vk::location(1)]] float3 normal : NORMAL;
    [[vk::location(2)]] float3 viewPos : VIEW_POSITION;
};

struct EyeballScenePushconstants {
    float4x4 viewModelMatrix;
    float4x4 projectionMatrix;

    int indexOffset;
    int indexCount;
    int pad0;
    int pad1;
};

[[vk::push_constant]]
cbuffer Pushconstants {
    EyeballScenePushconstants data;
}

#endif