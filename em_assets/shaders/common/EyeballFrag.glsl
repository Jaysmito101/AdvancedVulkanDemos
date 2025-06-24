#version 450

#pragma shader_stage(fragment)

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

void main() 
{
    outColor = vec4(0.0, 1.0, 0.0, 1.0);
}