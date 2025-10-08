#include "HLSPlayerCommon"

[[vk::push_constant]]
cbuffer PushConstants {
    UberPushConstantData data;
};

[[vk::binding(1, 1)]]
SAMPLER2D_TAB(textures, 1);

float4 main(float4 position: SV_Position) : SV_Target {
    return float4(1.0, 0.0, 1.0, 1.0);
}
