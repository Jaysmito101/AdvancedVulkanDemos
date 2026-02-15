struct GuiVertexData {
	min16float posX, posY;
	min16float sizeX, sizeY;
	min16float uv0X, uv0Y;
	min16float uv1X, uv1Y;
	min16float cornerRadius;
	min16uint useTexture;
	uint color;
};

[[vk::binding(0, 0)]]
StructuredBuffer<GuiVertexData> guiData;

struct VertexShaderOutput {
	float4 position : SV_Position;
	float2 uv : TEXCOORD0;
	float4 color : COLOR0;
	float2 rectSize : TEXCOORD1;
	float cornerRadius : TEXCOORD2;
	float2 localPos : TEXCOORD3;
	float useTexture : TEXCOORD4;
};

static uint2 decodeU16x2(uint packed)
{
	return uint2(packed & 0xFFFFu, (packed >> 16) & 0xFFFFu);
}

VertexShaderOutput main(uint vertexIndex : SV_VertexID, uint instanceIndex : SV_InstanceID)
{
	GuiVertexData data = guiData[instanceIndex];

	float2 rectPos = float2(data.posX, data.posY);
	float2 rectSize = float2(data.sizeX, data.sizeY);
	float2 uvMin = float2(data.uv0X, data.uv0Y);
	float2 uvMax = float2(data.uv1X, data.uv1Y);
	float cornerRadius = data.cornerRadius;
	float useTexture = (uint)data.useTexture != 0u ? 1.0 : 0.0;
	uint packedColor = data.color;

	uint triangleVertex = vertexIndex % 6u;
	float2 corner;
	if (triangleVertex == 0u) {
		corner = float2(0.0, 0.0);
	} else if (triangleVertex == 1u) {
		corner = float2(1.0, 0.0);
	} else if (triangleVertex == 2u) {
		corner = float2(0.0, 1.0);
	} else if (triangleVertex == 3u) {
		corner = float2(0.0, 1.0);
	} else if (triangleVertex == 4u) {
		corner = float2(1.0, 0.0);
	} else {
		corner = float2(1.0, 1.0);
	}

	float2 normalizedPos = rectPos + corner * rectSize;
	float2 ndc;
	ndc.x = normalizedPos.x * 2.0 - 1.0;
	ndc.y = normalizedPos.y * 2.0 - 1.0;

	float2 uv = lerp(uvMin, uvMax, corner);

	float4 color;
	color.r = ((packedColor >> 0u) & 0xFFu) / 255.0;
	color.g = ((packedColor >> 8u) & 0xFFu) / 255.0;
	color.b = ((packedColor >> 16u) & 0xFFu) / 255.0;
	color.a = ((packedColor >> 24u) & 0xFFu) / 255.0;

	VertexShaderOutput output;
	output.position = float4(ndc, 0.0, 1.0);
	output.uv = uv;
	output.color = color;
	output.rectSize = rectSize;
	output.cornerRadius = cornerRadius;
	output.localPos = corner * rectSize;
	output.useTexture = useTexture;
	return output;
}
