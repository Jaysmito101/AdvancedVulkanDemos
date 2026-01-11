#include "RealisticHeadCommon"

float4 main(VertexShaderOutput input) : SV_Target {
      float3 N = normalize(input.normal);
      float3 L = normalize(cSunlightDir);

      // a skin color
      float3 albedo = float3(1.0, 0.8, 0.6);

      float3 ambient = 0.05 * albedo;
      float3 diffuse = max(dot(N, L), 0.0) * albedo;

      float3 color = ambient + diffuse;
      return float4(color, 1.0);
}
