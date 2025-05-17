#version 450

#pragma shader_stage(fragment)

layout (set=0, binding = 0) uniform sampler2D sceneFramebuffer;
layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outColor;

struct PushConstantData {
    float windowWidth;
    float windowHeight; 
    float framebufferWidth; 
    float framebufferHeight; 

    float sceneLoadingProgress;
    float time;
    float pad0;
    float pad1;
};


layout(push_constant) uniform PushConstants {
    PushConstantData data;
} pushConstants;

const float PI = 3.14159265358979323846;

void main() 
{
    if (pushConstants.data.sceneLoadingProgress >= 1.0) {
        outColor = texture(sceneFramebuffer, inUV);
        return;
    }

    vec2 p = (inUV * 2.0 - 1.0); 
    float aspectRatio = pushConstants.data.windowWidth / pushConstants.data.windowHeight;
    p.x *= aspectRatio; 

    float aa = 2.0 / pushConstants.data.windowHeight;

    vec3 finalColor = vec3(5.0/255.0, 5.0/255.0, 5.0/255.0); 

    vec2 loaderCenter = vec2(0.0, 0.0); 
    float loaderRadius = 0.15;
    float loaderTrackThickness = 0.045; 
    float borderThicknessAA = aa * 1.0; // Border width in terms of anti-aliasing width

    vec3 borderColor = vec3(0.03, 0.03, 0.035); 
    vec3 trackColor = vec3(0.05, 0.05, 0.06); 
    vec3 fillColor = vec3(0.08, 0.12, 0.2);  

    float distToCenter = length(p - loaderCenter);

    // Render border (slightly thicker ring)
    float borderRingSDF = abs(distToCenter - loaderRadius) - (loaderTrackThickness * 0.5 + borderThicknessAA);
    finalColor = mix(finalColor, borderColor, smoothstep(aa, -aa, borderRingSDF));

    // Render track on top of the border area
    float mainRingSDF = abs(distToCenter - loaderRadius) - loaderTrackThickness * 0.5;
    finalColor = mix(finalColor, trackColor, smoothstep(aa, -aa, mainRingSDF));


    if (pushConstants.data.sceneLoadingProgress > 0.00001) { 
        float angle_norm = (atan(p.x, -p.y) + PI) / (2.0 * PI); 
        angle_norm = fract(angle_norm + 0.5); 

        float progress = pushConstants.data.sceneLoadingProgress;

        // Alpha for being inside the main track ring
        float ringAlpha = smoothstep(aa, -aa, mainRingSDF);

        float angle_aa_width = 1.0 / (PI * pushConstants.data.windowHeight * loaderRadius);
        angle_aa_width = max(angle_aa_width, 0.0001); 

        float angularFill = smoothstep(progress + angle_aa_width, progress - angle_aa_width, angle_norm);
        
        if (progress >= 1.0 - angle_aa_width * 0.5) { 
            angularFill = 1.0;
        }

        float fillCompositeAlpha = ringAlpha * angularFill;
        
        float pulseIntensity = (sin(pushConstants.data.time * 4.0) * 0.5 + 0.5) * 0.2; 
        vec3 pulsedFillColor = clamp(fillColor + fillColor * pulseIntensity, 0.0, 1.0);

        finalColor = mix(finalColor, pulsedFillColor, fillCompositeAlpha);
    }
    
    outColor = vec4(finalColor, 1.0);
}