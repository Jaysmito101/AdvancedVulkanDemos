#include "HLSPlayerCommon"
#include "HLSPlayerSDFFunctions"
#include "ColorUtils"

[[vk::push_constant]]
cbuffer PushConstants {
    UberPushConstantData data;
};

[[vk::binding(1, 0)]]
SAMPLER2D_TAB(textures, 1);


float3 calcNormal(float3 p) {
    const float eps = 0.001;
    return normalize(float3(
        mapScene(p + float3(eps, 0.0, 0.0)).x - mapScene(p - float3(eps, 0.0, 0.0)).x,
        mapScene(p + float3(0.0, eps, 0.0)).x - mapScene(p - float3(0.0, eps, 0.0)).x,
        mapScene(p + float3(0.0, 0.0, eps)).x - mapScene(p - float3(0.0, 0.0, eps)).x
    ));
}

float2 rayMarch(float3 ro, float3 rd) {
    float t = 0.0;
    float materialId = 0.0;
    
    for (int i = 0; i < 64; i++) {
        float3 p = ro + rd * t;
        float3 result = mapScene(p);
        float d = result.x;
        materialId = result.y;
        
        if (d < 0.01) break;
        if (t > 100.0) {
            materialId = 0.0;
            break;
        }
        
        t += d;
    }
    
    return float2(t, materialId);
}


// iq's soft shadow function adapted for our scene
float calcSoftShadow( in float3 ro, in float3 rd, in float mint, in float tmax, in float k )
{
    float res = 1.0;
    float t = mint;
    float ph = 1e10;
    
    for( int i=0; i < 64; i++ )
    {
        float h = mapScene( ro + rd*t ).x;
        if( h<0.001 ) return 0.0;
        
        float y = h*h/(2.0*ph);
        float d = sqrt(h*h-y*y);
        res = min( res, k*d/max(0.0,t-y) );
        ph = h;
        
        t += h;
        if( t>tmax ) break;
    }
    return clamp( res, 0.0, 1.0 );
}


float3 getScreenColor(float3 p, float3 normal, float3 rd, int tvIndex) {
    float3 localP = p - screenPositions[tvIndex];
    localP.xz = rotateXZ(localP.xz, screenRotations[tvIndex]);
    
    localP = localP / 10.0;
    
    localP.y -= 0.1;
    
    float2 screenUV;
    screenUV.x = (localP.x / 1.1) + 0.5;
    screenUV.y = (-localP.y / 0.8) + 0.5;
    
    screenUV = saturate(screenUV);
    
    float3 screenColor = float3(0.0, 0.0, 0.0); // Default black screen
    
    int texIndex = (data.textureIndices >> (tvIndex * 8)) & 0xFF;
    
    if (texIndex < 5) {
        float y = SAMPLE_TEXTURE_TAB(textures, screenUV, texIndex * 2 + 0).r;
        float2 cbcr = SAMPLE_TEXTURE_TAB(textures, screenUV, texIndex * 2 + 1).rg;
        screenColor = yCbCrToRgbBt601(float3(y, cbcr));
    } else {
        float noise = frac(sin(dot(screenUV * 100.0, float2(12.9898, 78.233))) * 43758.5453);
        screenColor = float3(noise * 0.05);
    }
    
    float2 centered = screenUV * 2.0 - 1.0;
    float vignette = 1.0 - dot(centered, centered) * 0.15;
    screenColor *= vignette;
    
    float fresnel = pow(saturate(1.0 - dot(normal, -rd)), 4.0);
    screenColor += float3(0.15) * fresnel;
    
    return screenColor;
}

float3 shade(float3 p, float3 normal, float3 rd, float materialId) {
    float3 sunDir = normalize(float3(0.6, 0.8, -0.4));
    float3 sunCol = float3(1.0, 0.95, 0.9);
    float3 skyDir = float3(0.0, 1.0, 0.0);
    float3 skyCol = float3(0.05, 0.05, 0.08); 
    float3 bounceDir = float3(0.0, -1.0, 0.0);
    float3 bounceCol = float3(0.02, 0.02, 0.02); 

    float sunDiff = clamp(dot(normal, sunDir), 0.0, 1.0);
    float skyDiff = clamp(0.5 + 0.5 * dot(normal, skyDir), 0.0, 1.0);
    float bounceDiff = clamp(0.5 + 0.5 * dot(normal, bounceDir), 0.0, 1.0);
    
    float shadow = 1.0;
    if (sunDiff > 0.001) {
        shadow = calcSoftShadow(p + normal * 0.01, sunDir, 0.02, 20.0, 8.0);
    }
    
    float3 baseColor;
    float3 specColor = float3(0.0, 0.0, 0.0);
    float specPower = 16.0;
    float specIntensity = 0.0;
    
    if (materialId < 1.5) {
        baseColor = float3(0.05, 0.05, 0.05);
        float2 grid = abs(frac(p.xz * 0.5) - 0.5) / fwidth(p.xz * 0.5);
        float lineVal = min(grid.x, grid.y);
        float gridIntensity = 1.0 - smoothstep(0.0, 1.0, lineVal);
        baseColor += float3(0.05, 0.05, 0.05) * gridIntensity * 0.5;
        specIntensity = 0.2;
    } else if (materialId < 3.5) {
        baseColor = float3(0.45, 0.28, 0.15);
        float grain = sin(p.x * 50.0 + p.y * 20.0) * 0.08 + sin(p.y * 80.0) * 0.04;
        baseColor *= 1.0 + grain;
        specIntensity = 0.1;
    } else if (materialId < 4.5) {
        baseColor = float3(0.32, 0.20, 0.10);
        float grain = sin(p.y * 60.0) * 0.06;
        baseColor *= 1.0 + grain;
        specIntensity = 0.05;
    } else if (materialId < 5.5) {
        baseColor = float3(0.12, 0.08, 0.05);
        specIntensity = 0.2;
        specPower = 32.0;
    } else if (materialId < 6.5) {
        baseColor = float3(0.03, 0.03, 0.035);
        specIntensity = 0.05;
    } else if (materialId < 7.5) {
        float h = saturate((p.y + 10.0) / 40.0);
        baseColor = lerp(float3(0.0, 0.0, 0.0), float3(0.05, 0.05, 0.06), h);
        specIntensity = 0.0;
    } else if (materialId < 8.5) {
        baseColor = float3(0.06, 0.05, 0.045);
        specIntensity = 0.05;
    } else if (materialId < 9.5) {
        baseColor = float3(0.85, 0.65, 0.35);
        specIntensity = 1.2;
        specPower = 64.0;
    } else if (materialId >= 10.0) {
        int tvIndex = (int)(materialId - 10.0);
        float3 screenColor = getScreenColor(p, normal, rd, tvIndex);
        
        float3 emission = screenColor * 0.8;
        
        float3 r = reflect(-sunDir, normal);
        float sunSpec = pow(max(dot(r, -rd), 0.0), 32.0);
        
        return emission + sunSpec * 0.2;
    } else {
        baseColor = float3(0.5, 0.5, 0.5);
    }
    
    float3 lin = float3(0.0, 0.0, 0.0);
    lin += sunCol * sunDiff * shadow;
    lin += skyCol * skyDiff;
    lin += bounceCol * bounceDiff;
    
    if (specIntensity > 0.0) {
        float3 r = reflect(-sunDir, normal);
        float spe = pow(max(dot(r, -rd), 0.0), specPower);
        specColor = sunCol * spe * specIntensity * shadow;
    }
    
    return baseColor * lin + specColor;
}


float3 getSkyColor(float3 rd) {
    float t = rd.y * 0.5 + 0.5;
    float3 topColor = float3(0.05, 0.05, 0.05); 
    float3 bottomColor = float3(0.0, 0.0, 0.0);
    return lerp(bottomColor, topColor, t);
}

float4 main(VertexShaderOutput input) : SV_Target {
    float2 uv = input.uv * 2.0 - 1.0;
    float aspect = 16.0 / 9.0;
    uv.x *= aspect;
    uv.y *= -1.0;
    
    float3 ro = data.cameraPosition.xyz;
    float3 forward = normalize(data.cameraDirection.xyz);
    float3 up = float3(0.0, 1.0, 0.0);
    float3 right = normalize(cross(forward, up));
    up = cross(right, forward);
    
    float fov = 1.2;
    float3 rd = normalize(forward * fov + right * uv.x + up * uv.y);
    
    float2 result = rayMarch(ro, rd);
    float dist = result.x;
    float materialId = result.y;
    
    float3 color;
    
    if (materialId > 0.0 && dist < 100.0) {
        float3 p = ro + rd * dist;
        float3 normal = calcNormal(p);
        color = shade(p, normal, rd, materialId);
    } else {
        color = getSkyColor(rd);
    }

    float fogIdx = 1.0 - exp(-dist * 0.02);
    float3 fogCol = float3(0.01, 0.01, 0.02);
    color = lerp(color, fogCol, fogIdx);
    
    return float4(color, 1.0);
}
