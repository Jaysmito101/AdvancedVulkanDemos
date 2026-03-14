#include "DrawListCommon"


[[vk::binding(1, 0)]]
SAMPLER2D(tex, 1);

float4 main(VertexShaderOutput input) : SV_Target {
    return float4(input.texCoord, 0.0, 1.0);
}
