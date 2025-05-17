#version 450
#pragma shader_stage(fragment)

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inRayDir;
layout(location = 0) out vec4 outColor;

struct PushConstantData {
    float windowWidth;
    float windowHeight;
    float sphereRadius;
    float time;
    float spherePosition[3];
    float cameraPosition[3];
    float rotation;
};

layout(push_constant) uniform PushConstants {
    PushConstantData data;
} pushConstants;

// Ray-Sphere intersection
// Returns distance to intersection (negative if no hit)
float intersectSphere(vec3 rayOrigin, vec3 rayDir, vec3 sphereCenter, float sphereRadius) {
    vec3 oc = rayOrigin - sphereCenter;
    float a = dot(rayDir, rayDir);
    float b = 2.0 * dot(oc, rayDir);
    float c = dot(oc, oc) - sphereRadius * sphereRadius;
    float discriminant = b * b - 4.0 * a * c;
    
    if (discriminant < 0.0) {
        return -1.0;
    } else {
        return (-b - sqrt(discriminant)) / (2.0 * a);
    }
}

// Calculate normal at the intersection point
vec3 getSphereNormal(vec3 point, vec3 sphereCenter) {
    return normalize(point - sphereCenter);
}

// Simple lighting calculation
vec3 calculateLighting(vec3 point, vec3 normal, vec3 viewDir) {
    // Ambient light
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);
    
    // Directional light from upper right
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
    
    // Specular highlight
    float specularStrength = 0.5;
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);
    
    return ambient + diffuse + specular;
}

void main() {
    // Setup camera and sphere
    vec3 rayOrigin = vec3(pushConstants.data.cameraPosition[0], 
                         pushConstants.data.cameraPosition[1], 
                         pushConstants.data.cameraPosition[2]);
    
    vec3 sphereCenter = vec3(pushConstants.data.spherePosition[0], 
                           pushConstants.data.spherePosition[1], 
                           pushConstants.data.spherePosition[2]);
    
    // Test ray-sphere intersection
    float t = intersectSphere(rayOrigin, inRayDir, sphereCenter, pushConstants.data.sphereRadius);
    
    // Background gradient
    vec3 bgColor = mix(
        vec3(0.1, 0.1, 0.2),  // Dark blue at bottom
        vec3(0.3, 0.2, 0.5),  // Purple at top
        inUV.y
    );
    
    // Add some animated "stars" to background
    vec2 starPos = inUV * 30.0 + vec2(pushConstants.data.time * 0.1);
    float starPattern = fract(sin(dot(floor(starPos), vec2(12.9898, 78.233))) * 43758.5453);
    starPattern = smoothstep(0.97, 1.0, starPattern) * 0.5;
    bgColor += starPattern;
    
    if (t > 0.0) {
        // Calculate hit point and normal
        vec3 hitPoint = rayOrigin + t * inRayDir;
        vec3 normal = getSphereNormal(hitPoint, sphereCenter);
        
        // Calculate view direction
        vec3 viewDir = normalize(-inRayDir);
        
        // Calculate lighting
        vec3 lighting = calculateLighting(hitPoint, normal, viewDir);
        
        // Generate a pattern on the sphere based on normal
        vec3 pattern;
        float rings = sin((normal.y * 5.0 + pushConstants.data.time) * 2.0) * 0.5 + 0.5;
        pattern = mix(
            vec3(0.8, 0.2, 0.7),  // Magenta
            vec3(0.2, 0.5, 0.9),  // Blue
            rings
        );
        
        // Combine pattern with lighting
        vec3 finalColor = pattern * lighting;
        
        // Add glow near edges (fresnel effect)
        float fresnel = pow(1.0 - max(0.0, dot(normal, viewDir)), 3.0);
        finalColor += vec3(0.8, 0.4, 1.0) * fresnel;
        
        outColor = vec4(finalColor, 1.0);
    }
    else {
        outColor = vec4(bgColor, 1.0);
    }
}
