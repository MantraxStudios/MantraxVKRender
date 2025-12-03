#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragWorldPos;
layout(location = 4) in vec3 fragCameraPos; // NUEVO

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 cameraPosition; // NUEVO
} ubo;

// Texturas PBR
layout(binding = 1) uniform sampler2D albedoMap;
layout(binding = 2) uniform sampler2D normalMap;
layout(binding = 3) uniform sampler2D metallicMap;
layout(binding = 4) uniform sampler2D roughnessMap;
layout(binding = 5) uniform sampler2D aoMap;

// Push constants para control de materiales
layout(push_constant) uniform MaterialProperties {
    vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float normalScale;
    int useAlbedoMap;
    int useNormalMap;
    int useMetallicMap;
    int useRoughnessMap;
    int useAOMap;
} material;

// ============ CONFIGURACIÓN DE ILUMINACIÓN ============

// Luz puntual principal (MÁS INTENSA para efecto mojado)
const vec3 pointLightPos = vec3(5.0, 8.0, 10.0);
const vec3 pointLightColor = vec3(1.0, 0.98, 0.95);
const float pointLightIntensity = 250.0; // AUMENTADO

// Luz direccional 
const vec3 dirLightDirection = normalize(vec3(-0.2, -0.8, -0.3));
const vec3 dirLightColor = vec3(1.0, 0.98, 0.95);
const float dirLightIntensity = 4.0; // AUMENTADO

// Luz de relleno
const vec3 fillLightDirection = normalize(vec3(0.5, 0.2, 0.8));
const vec3 fillLightColor = vec3(0.7, 0.75, 0.8);
const float fillLightIntensity = 1.5;

// Iluminación ambiental (oscura para contraste)
const vec3 ambientSkyColor = vec3(0.20, 0.22, 0.25);
const vec3 ambientGroundColor = vec3(0.10, 0.10, 0.11);
const vec3 ambientSpecularColor = vec3(0.5, 0.55, 0.6); // MÁS BRILLANTE
const float ambientIntensity = 0.2;

const float PI = 3.14159265359;
const float EPSILON = 0.0001;

// ============ FUNCIONES AUXILIARES ============

mat3 CalculateTBN(vec3 N, vec3 P, vec2 uv) {
    vec3 dp1 = dFdx(P);
    vec3 dp2 = dFdy(P);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);
    
    vec3 dp2perp = cross(dp2, N);
    vec3 dp1perp = cross(N, dp1);
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
    
    float invmax = inversesqrt(max(dot(T, T), dot(B, B)));
    return mat3(T * invmax, B * invmax, N);
}

vec3 GetNormalFromMap(mat3 TBN) {
    vec3 tangentNormal = texture(normalMap, fragTexCoord).xyz * 2.0 - 1.0;
    tangentNormal.xy *= material.normalScale;
    return normalize(TBN * tangentNormal);
}

// ============ FUNCIONES PBR ============

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

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 CalculateHemisphericAmbient(vec3 N, vec3 albedo, float metallic) {
    float upness = N.y * 0.5 + 0.5;
    vec3 ambient = mix(ambientGroundColor, ambientSkyColor, upness);
    
    // Los metales puros no tienen componente difusa, pero agregamos un mínimo
    // para evitar negros absolutos
    float diffuseContribution = mix(1.0, 0.15, metallic);
    
    return ambient * albedo * ambientIntensity * diffuseContribution;
}

vec3 CalculateSpecularIBL(vec3 N, vec3 V, vec3 F0, float roughness, float metallic) {
    vec3 R = reflect(-V, N);
    
    // Environment map brillante para efecto mojado
    float upness = R.y * 0.5 + 0.5;
    vec3 envColor = mix(ambientGroundColor, ambientSpecularColor, upness);
    
    // Múltiples highlights brillantes para simular reflexiones nítidas
    float highlight1 = max(dot(R, normalize(vec3(0.4, 0.7, 0.5))), 0.0);
    float highlight2 = max(dot(R, normalize(vec3(-0.3, 0.8, 0.2))), 0.0);
    float highlight3 = max(dot(R, normalize(vec3(0.6, 0.5, -0.4))), 0.0);
    
    // Highlights muy intensos y nítidos (exponentes altos)
    envColor += vec3(0.6, 0.65, 0.7) * pow(highlight1, 3.0);
    envColor += vec3(0.4, 0.45, 0.5) * pow(highlight2, 4.0);
    envColor += vec3(0.3, 0.35, 0.4) * pow(highlight3, 5.0);
    
    float NdotV = max(dot(N, V), 0.0);
    vec3 F = fresnelSchlickRoughness(NdotV, F0, roughness);
    
    // Mayor contribución especular = look más mojado/brillante
    float smoothness = 1.0 - roughness;
    float specularStrength = mix(0.3, 1.2, smoothness * smoothness);
    
    return envColor * F * specularStrength;
}

vec3 CalculatePBRLight(vec3 N, vec3 V, vec3 L, vec3 radiance, vec3 albedo, float metallic, float roughness, vec3 F0) {
    vec3 H = normalize(V + L);
    
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), EPSILON);
    float HdotV = max(dot(H, V), 0.0);
    
    // Para look "mojado", queremos reflejos especulares MUY intensos
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(HdotV, F0);
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL + EPSILON;
    vec3 specular = numerator / denominator;
    
    // Boost adicional al especular para efecto mojado
    specular *= mix(1.0, 1.5, metallic);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

// ============ MAIN ============

void main() {
    // SOLUCIÓN SIMPLE: Usar la posición de cámara del uniform
    vec3 camPos = fragCameraPos;
    
    // Normalizar vectores base
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(camPos - fragWorldPos);
    
    // Double-sided lighting
    if (!gl_FrontFacing) {
        N = -N;
    }
    
    // ============ SAMPLE PBR TEXTURES ============
    
    // Albedo con gamma correction
    vec3 albedo;
    if (material.useAlbedoMap == 1) {
        vec4 albedoSample = texture(albedoMap, fragTexCoord);
        albedo = pow(albedoSample.rgb, vec3(2.2));
    } else {
        albedo = material.baseColorFactor.rgb;
    }
    
    // Normal mapping
    if (material.useNormalMap == 1) {
        mat3 TBN = CalculateTBN(N, fragWorldPos, fragTexCoord);
        N = GetNormalFromMap(TBN);
    }
    
    // Metallic
    float metallic;
    if (material.useMetallicMap == 1) {
        metallic = texture(metallicMap, fragTexCoord).b * material.metallicFactor;
    } else {
        metallic = material.metallicFactor;
    }
    
    // Roughness
    float roughness;
    if (material.useRoughnessMap == 1) {
        roughness = texture(roughnessMap, fragTexCoord).g * material.roughnessFactor;
    } else {
        roughness = material.roughnessFactor;
    }
    roughness = clamp(roughness, 0.02, 1.0);
    
    // Ambient Occlusion
    float ao;
    if (material.useAOMap == 1) {
        ao = texture(aoMap, fragTexCoord).r;
    } else {
        ao = 1.0;
    }
    
    // ============ PBR LIGHTING ============
    
    // F0 (reflectancia en incidencia normal)
    // CLAVE: Para metales, F0 ES el color del albedo
    // Para no-metales, F0 es ~0.04 (4% de reflexión)
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    vec3 Lo = vec3(0.0);
    
    // Luz direccional
    vec3 L_dir = -dirLightDirection;
    vec3 radiance_dir = dirLightColor * dirLightIntensity;
    Lo += CalculatePBRLight(N, V, L_dir, radiance_dir, albedo, metallic, roughness, F0);
    
    // Luz de relleno
    vec3 L_fill = fillLightDirection;
    vec3 radiance_fill = fillLightColor * fillLightIntensity;
    Lo += CalculatePBRLight(N, V, L_fill, radiance_fill, albedo, metallic, roughness, F0);
    
    // Luz puntual
    vec3 L_point = normalize(pointLightPos - fragWorldPos);
    float distance = length(pointLightPos - fragWorldPos);
    float attenuation = pointLightIntensity / (distance * distance);
    vec3 radiance_point = pointLightColor * attenuation;
    Lo += CalculatePBRLight(N, V, L_point, radiance_point, albedo, metallic, roughness, F0);
    
    // Iluminación ambiental
    vec3 ambient = CalculateHemisphericAmbient(N, albedo, metallic);
    vec3 specularIBL = CalculateSpecularIBL(N, V, F0, roughness, metallic);
    
    // Combinar componente difusa y especular ambiental
    ambient += specularIBL;
    ambient *= ao;
    
    // Composición final
    vec3 color = ambient + Lo;
    
    // Tone mapping ACES
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    color = clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    outColor = vec4(color, 1.0);
}