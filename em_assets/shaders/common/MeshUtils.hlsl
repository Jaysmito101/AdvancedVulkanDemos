#ifndef MESH_UTILS
#define MESH_UTILS

#include "ShaderAdapter"

struct ModelVertex {
    float16_t vx, vy, vz;
    uint16_t tp; // packed tangent: 8-8 octahedral
    uint np;     // packed normal: 10-10-10-2 vector + bitangent sign
    float16_t tu, tv;
};

// Source: https://github.com/zeux/niagara/blob/master/src/shaders/math.h
float3 decodeOct(float2 e)
{
	// https://x.com/Stubbesaurus/status/937994790553227264
	float3 v = float3(e.xy, 1.0 - abs(e.x) - abs(e.y));
	float t = max(-v.z, 0);
	v.xy += float2(v.x >= 0 ? -t : t, v.y >= 0 ? -t : t);
	return normalize(v);
}


void unpackTBN(uint np, uint tp, out float3 normal, out float4 tangent)
{
	normal = ((int3(np) >> int3(0, 10, 20)) & int3(1023)) / 511.0 - 1.0;
	tangent.xyz = decodeOct(((int2(tp) >> int2(0, 8)) & int2(255)) / 127.0 - 1.0);
	tangent.w = (np & (1 << 30)) != 0 ? -1.0 : 1.0;
}

#endif