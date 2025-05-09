#version 450
#pragma shader_stage(vertex)

layout(location = 0) out vec2 outUV;

// Simple fullscreen quad shader for UI
void main() {
    // Generate a fullscreen triangle fan from vertex ID
    vec2 positions[6] = vec2[](
        vec2(0.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 1.0),
        vec2(0.0, 0.0),
        vec2(1.0, 1.0),
        vec2(1.0, 0.0)
    );
    
    vec2 pos = positions[gl_VertexIndex];
    outUV = pos; // Pass UV coordinates to fragment shader
    
    
    pos = pos * 2.0 - 1.0;
    gl_Position = vec4(pos, 0.0, 1.0);
}
