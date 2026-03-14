#include "DrawListCommon"


[[vk::binding(0, 1)]]
SAMPLER2D(tex, 0, 1);

float4 main(VertexShaderOutput input) : SV_Target {
    
    float3 color = float3(1.0);
    if (input.hasTexture > 0) {
        color = SAMPLE_TEXTURE(tex, input.texCoord).rgb;
    }
    color *= input.color;

    return float4(color, 1.0);
}
