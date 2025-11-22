#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

// Parámetros PBR básicos
const vec3 lightPos = vec3(5.0, 5.0, 5.0);
const vec3 lightColor = vec3(1.0, 1.0, 1.0);
const float lightIntensity = 10.0;

const vec3 camPos = vec3(0.0, 0.0, 3.0);

// Parámetros del material
const float metallic = 0.1;
const float roughness = 0.5;
const float ao = 1.0;

const float PI = 3.14159265359;
const float EPSILON = 0.0001;

// Función de distribución normal (GGX/Trowbridge-Reitz)
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

// Aproximación de Schlick para la geometría
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / max(denom, EPSILON);
}

// Función de geometría de Smith
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), EPSILON);
    float NdotL = max(dot(N, L), EPSILON);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// Aproximación de Fresnel-Schlick
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    // Normalizar vectores (CRÍTICO para evitar artefactos)
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(camPos - fragWorldPos);
    
    // Protección contra normales invertidas
    if (dot(N, V) < 0.0) {
        N = -N;
    }
    
    // Color base del material
    vec3 albedo = fragColor;
    
    // Calcular reflectancia en incidencia normal
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    // Iluminación directa
    vec3 L = normalize(lightPos - fragWorldPos);
    vec3 H = normalize(V + L);
    
    // Atenuación por distancia
    float distance = length(lightPos - fragWorldPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = lightColor * lightIntensity * attenuation;
    
    // Calcular productos punto con protección
    float NdotV = max(dot(N, V), EPSILON);
    float NdotL = max(dot(N, L), 0.0);
    float HdotV = max(dot(H, V), 0.0);
    
    // BRDF de Cook-Torrance
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(HdotV, F0);
    
    // Especular con denominador protegido
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * NdotV * max(NdotL, EPSILON) + EPSILON;
    vec3 specular = numerator / denominator;
    
    // Balance entre difuso y especular
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    // Contribución final de luz directa
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;
    
    // Luz ambiente mejorada
    vec3 ambient = vec3(0.1) * albedo * ao;
    
    vec3 color = ambient + Lo;
    
    // Tone mapping (Reinhard)
    color = color / (color + vec3(1.0));
    
    // Corrección gamma
    color = pow(color, vec3(1.0/2.2));
    
    outColor = vec4(color, 1.0);
}