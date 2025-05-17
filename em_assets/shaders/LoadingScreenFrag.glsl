#version 450
#pragma shader_stage(fragment)

precision highp float;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

struct PushConstantData {
   float windowWidth;
   float windowHeight;
   float progress;
   float padding;
};

layout(push_constant) uniform PushConstants {
   PushConstantData data;
} pushConstants;


float sdCapsule( vec2 p, vec2 a, vec2 b, float r ) {
    vec2 pa = p - a, ba = b - a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    return length( pa - ba*h ) - r;
}

float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

void main() {
    vec2 uv = (inUV * 2.0 - 1.0);
 
    vec4 backgroundColor = vec4(0.0);
    vec4 barBackgroundColor = vec4(180.0/255.0, 141.0/255.0, 209.0/255.0, 1.0);
    vec4 barForegroundColor = vec4(79.0/255.0, 45.0/255.0, 95.0/255.0, 1.0);
    vec4 trackBackgroundColor = vec4(130.0/255.0, 88.0/255.0, 150.0/255.0, 1.0);
    vec4 borderColor = vec4(60.0/255.0, 40.0/255.0, 70.0/255.0, 1.0);
 
    float barWidth = 1.0;
    float barHeight = 0.1;
    float borderThickness = 0.015;
 
    vec2 capsuleStart = vec2(-barWidth / 2.0, 0.0);
    vec2 capsuleEnd = vec2(barWidth / 2.0, 0.0);
    vec2 capsuleProgressEnd = vec2(-barWidth / 2.0 + barWidth * pushConstants.data.progress, 0.0);
 
    float capsuleOuter = sdCapsule(uv, capsuleStart, capsuleEnd, barHeight);
    float capsuleInner = sdCapsule(uv, capsuleStart, capsuleEnd, barHeight - borderThickness);
    float capsuleTrack = sdCapsule(uv, capsuleStart, capsuleEnd, barHeight - 2.0 * borderThickness);
    float capsuleProgress = sdCapsule(uv, capsuleStart, capsuleProgressEnd, barHeight - 2.0 * borderThickness);
    float capsuleProgressInner = sdCapsule(uv, capsuleStart, capsuleProgressEnd, barHeight - 3.0 * borderThickness);
 
    vec4 finalColor = backgroundColor;
 
    float aa = 1.5 / pushConstants.data.windowHeight;
    float borderShape = max(capsuleOuter, -capsuleInner);
    float progressBorderShape = max(capsuleProgress, -capsuleProgressInner);
 
    finalColor = mix(finalColor, barBackgroundColor, smoothstep(-aa, aa, capsuleOuter));
    finalColor = mix(finalColor, borderColor, smoothstep(aa, -aa, borderShape));
    finalColor = mix(finalColor, trackBackgroundColor, smoothstep(-aa, aa, -capsuleTrack));
    finalColor = mix(finalColor, barForegroundColor, smoothstep(-aa, aa, -capsuleProgress));
    finalColor = mix(finalColor, borderColor, smoothstep(aa, -aa, progressBorderShape));
 
    if (capsuleOuter >= 0.0) {
        finalColor.a = 0.0;
    }

    float contrast = 1.1;
    finalColor.rgb = (finalColor.rgb - 0.5) * contrast + 0.5;
    float noise = (random(gl_FragCoord.xy / 2.0) - 0.5) * 0.05;
    finalColor.rgb += noise;
    finalColor.rgb = clamp(finalColor.rgb, 0.0, 1.0);
    outColor = finalColor;
}

