#include "DeccerCubeCommon"

[[vk::push_constant]]
cbuffer PushConstants {
    UberPushConstantData data;
};

[[vk::binding(1, 1)]]
SAMPLER2D_TAB(textures, 1);

float4 main(VertexShaderOutput input) : SV_Target {
    const float3 lightPosition = float3(5.0, 5.0, 5.0);

    float3 N = normalize(input.normal);
    float3 L = normalize(lightPosition - input.position.xyz);

    float3 albedo = float3(0.8, 0.3, 0.2);
    if (data.textureIndex > 0) {
        albedo = SAMPLE_TEXTURE_TAB(textures, input.uv, data.textureIndex).rgb;
    }

    float3 ambient = 0.05 * albedo;
    float3 diffuse = max(dot(N, L), 0.0) * albedo;

    float3 color = ambient + diffuse;
    return float4(color, 1.0);
}
