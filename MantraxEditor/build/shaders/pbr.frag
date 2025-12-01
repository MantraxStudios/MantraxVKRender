#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragWorldPos;
layout(location = 4) in vec3 fragBarycentric;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

// Parámetros de wireframe
const float wireframeThickness = 1.0;
const vec3 wireframeColor = vec3(0.0, 0.0, 0.0);
const bool showSolidWithWireframe = true;

// Parámetros de iluminación
const vec3 lightPos = vec3(10.0, 10.0, 10.0);
const vec3 lightColor = vec3(1.0, 1.0, 1.0);
const float lightIntensity = 150.0;

const vec3 ambientSkyColor = vec3(0.6, 0.7, 0.9);
const vec3 ambientGroundColor = vec3(0.4, 0.35, 0.3);
const float ambientIntensity = 0.8;

const float metallic = 0.0;
const float roughness = 0.5;
const float ao = 1.0;

const float PI = 3.14159265359;
const float EPSILON = 0.0001;

// Función para calcular el factor de wireframe
float edgeFactor() {
    vec3 d = fwidth(fragBarycentric);
    vec3 a3 = smoothstep(vec3(0.0), d * wireframeThickness, fragBarycentric);
    return min(min(a3.x, a3.y), a3.z);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return nom / max(denom, EPSILON);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / max(denom, EPSILON);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), EPSILON);
    float NdotL = max(dot(N, L), EPSILON);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 CalculateHemisphericAmbient(vec3 N, vec3 albedo) {
    float hemisphereBlend = N.y * 0.5 + 0.5;
    vec3 ambient = mix(ambientGroundColor, ambientSkyColor, hemisphereBlend);
    return ambient * albedo * ambientIntensity;
}

void main() {
    // Calcular posición de la cámara desde la matriz de vista
    vec3 camPos = -vec3(ubo.view[3][0], ubo.view[3][1], ubo.view[3][2]);
    
    // Calcular factor de wireframe
    float edge = edgeFactor();
    
    if (showSolidWithWireframe) {
        // Normalizar la normal
        vec3 N = normalize(fragNormal);
        vec3 V = normalize(camPos - fragWorldPos);
        
        // Double-sided lighting
        if (!gl_FrontFacing) {
            N = -N;
        }
        
        // Color base
        vec3 albedo = pow(fragColor, vec3(2.2));
        
        // Reflectancia base
        vec3 F0 = vec3(0.04);
        F0 = mix(F0, albedo, metallic);
        
        // Vectores de iluminación
        vec3 L = normalize(lightPos - fragWorldPos);
        vec3 H = normalize(V + L);
        
        // Atenuación por distancia
        float distance = length(lightPos - fragWorldPos);
        float attenuation = lightIntensity / (distance * distance);
        vec3 radiance = lightColor * attenuation;
        
        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), EPSILON);
        float HdotV = max(dot(H, V), 0.0);
        
        // BRDF Cook-Torrance
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(HdotV, F0);
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * NdotV * NdotL + EPSILON;
        vec3 specular = numerator / denominator;
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;
        
        vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;
        
        vec3 ambient = CalculateHemisphericAmbient(N, albedo) * ao;
        
        vec3 color = ambient + Lo;
        color = max(color, vec3(0.0));
        
        float exposure = 1.5;
        color = vec3(1.0) - exp(-color * exposure);
        
        color = pow(color, vec3(1.0/2.2));
        
        vec3 finalColor = mix(wireframeColor, color, edge);
        outColor = vec4(finalColor, 1.0);
        
    } else {
        if (edge < 0.5) {
            outColor = vec4(wireframeColor, 1.0);
        } else {
            discard;
        }
    }
}