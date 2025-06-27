#ifndef MESH_UTILS
#define MESH_UTILS

struct ModelVertex {
    float16_t vx, vy, vz;
    uint16_t tp; // packed tangent: 8-8 octahedral
    uint np;     // packed normal: 10-10-10-2 vector + bitangent sign
    float16_t tu, tv;
};

// Source: https://github.com/zeux/niagara/blob/master/src/shaders/math.h
vec3 decodeOct(vec2 e)
{
	// https://x.com/Stubbesaurus/status/937994790553227264
	vec3 v = vec3(e.xy, 1.0 - abs(e.x) - abs(e.y));
	float t = max(-v.z, 0);
	v.xy += vec2(v.x >= 0 ? -t : t, v.y >= 0 ? -t : t);
	return normalize(v);
}


void unpackTBN(uint np, uint tp, out vec3 normal, out vec4 tangent)
{
	normal = ((ivec3(np) >> ivec3(0, 10, 20)) & ivec3(1023)) / 511.0 - 1.0;
	tangent.xyz = decodeOct(((ivec2(tp) >> ivec2(0, 8)) & ivec2(255)) / 127.0 - 1.0);
	tangent.w = (np & (1 << 30)) != 0 ? -1.0 : 1.0;
}

#endif