#version 450
#pragma shader_stage(fragment)

precision highp float;

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inPosition;
layout(location = 3) flat in int inRenderingLight;
// Extremely inefficient thing to do but well, this is just a demo.
layout(location = 4) in vec3 inLightAPos;
layout(location = 5) in vec3 inLightBPos;

void main()
{
}
