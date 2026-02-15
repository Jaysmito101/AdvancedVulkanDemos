Texture2D guiAtlas : register(t1, space0);
SamplerState guiAtlasSampler : register(s1, space0);

struct FragmentInput {
	float2 uv : TEXCOORD0;
	float4 color : COLOR0;
	float2 rectSize : TEXCOORD1;
	float cornerRadius : TEXCOORD2;
	float2 localPos : TEXCOORD3;
	float useTexture : TEXCOORD4;
};

static float roundedBoxSDF(float2 p, float2 halfSize, float radius)
{
	float2 d = abs(p) - halfSize + float2(radius, radius);
	return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - radius;
}

float4 main(FragmentInput input) : SV_Target
{
	float4 atlasColor = guiAtlas.Sample(guiAtlasSampler, input.uv);

	float2 halfSize = input.rectSize * 0.5;
	float2 centered = input.localPos - halfSize;
	float dist = roundedBoxSDF(centered, halfSize, input.cornerRadius);

	float edgeWidth = fwidth(dist);
	float alpha = 1.0 - smoothstep(-edgeWidth, edgeWidth, dist);

	float4 result = input.color;
	if (input.useTexture > 0.5) {
		result.rgb *= atlasColor.rgb;
		result.a *= atlasColor.a;
	}
	result.a *= alpha;
	return result;
}
