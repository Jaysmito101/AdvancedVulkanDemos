struct VertexShaderOutput {
    float2 texCoord : TEXCOORD;
    float4 position : SV_Position;
};

float4 main(VertexShaderOutput input) : SV_Target {
    return float4(input.texCoord, 0.0, 1.0);
}
