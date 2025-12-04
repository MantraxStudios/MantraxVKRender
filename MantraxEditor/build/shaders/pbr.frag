#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragWorldPos;
layout(location = 4) in vec3 fragCameraPos;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 cameraPosition;
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

// ============ CONFIGURACIÓN DE ILUMINACIÓN REALISTA ============

// Luz direccional principal (simula sol/luz de estudio)
const vec3 mainLightDir = normalize(vec3(-0.3, -0.7, -0.5));
const vec3 mainLightColor = vec3(1.0, 0.98, 0.95);
const float mainLightIntensity = 3.5;

// Luz de relleno suave (simula rebote de luz)
const vec3 fillLightDir = normalize(vec3(0.5, 0.3, 0.8));
const vec3 fillLightColor = vec3(0.6, 0.65, 0.7);
const float fillLightIntensity = 1.2;

// Luz trasera/rim (para definir siluetas)
const vec3 rimLightDir = normalize(vec3(0.0, 0.2, 1.0));
const vec3 rimLightColor = vec3(0.9, 0.95, 1.0);
const float rimLightIntensity = 1.5;

// Iluminación ambiental (IBL simulado)
const vec3 ambientColorTop = vec3(0.15, 0.18, 0.22);      // Cielo
const vec3 ambientColorBottom = vec3(0.08, 0.08, 0.09);   // Suelo
const vec3 ambientColorHorizon = vec3(0.12, 0.14, 0.16);  // Horizonte
const float ambientIntensity = 0.4;

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

// Distribution (Specular D) - GGX/Trowbridge-Reitz
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

// Geometry (Specular G) - Smith's Schlick-GGX
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

// Fresnel (Specular F) - Schlick approximation
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ============ ILUMINACIÓN AMBIENTAL AVANZADA ============

vec3 CalculateRealisticAmbient(vec3 N, vec3 V, vec3 albedo, vec3 F0, float roughness, float metallic, float ao) {
    // Componente difusa ambiental con gradiente hemisférico
    float upFactor = N.y * 0.5 + 0.5;
    float horizonFactor = (1.0 - abs(N.y)) * 0.5;
    
    vec3 ambientDiffuse = mix(
        mix(ambientColorBottom, ambientColorHorizon, horizonFactor),
        ambientColorTop,
        upFactor
    );
    
    // Los metales no tienen componente difusa
    ambientDiffuse *= (1.0 - metallic);
    
    // Componente especular ambiental (simulación de IBL)
    vec3 R = reflect(-V, N);
    float NdotV = max(dot(N, V), 0.0);
    
    // Simular diferentes LODs del environment map basado en roughness
    float lod = roughness * 5.0;
    
    // Color del environment basado en la dirección de reflexión
    float reflUpness = R.y * 0.5 + 0.5;
    vec3 envColor = mix(ambientColorBottom, ambientColorTop, reflUpness);
    
    // Agregar highlights suaves para simular fuentes de luz en el environment
    float highlight1 = max(dot(R, normalize(vec3(0.5, 0.8, 0.3))), 0.0);
    float highlight2 = max(dot(R, normalize(vec3(-0.4, 0.6, -0.5))), 0.0);
    
    // Los highlights se vuelven más difusos con mayor roughness
    float sharpness = mix(8.0, 2.0, roughness);
    envColor += vec3(0.3, 0.32, 0.35) * pow(highlight1, sharpness);
    envColor += vec3(0.2, 0.22, 0.25) * pow(highlight2, sharpness);
    
    // Fresnel para el specular ambiental
    vec3 F = fresnelSchlickRoughness(NdotV, F0, roughness);
    
    // Aproximación de la integral especular
    vec3 specularAmbient = envColor * F;
    
    // Energy conservation
    vec3 kD = (1.0 - F) * (1.0 - metallic);
    
    // Combinar difuso y especular
    vec3 ambient = (kD * albedo * ambientDiffuse + specularAmbient) * ambientIntensity;
    
    // Aplicar AO
    ambient *= ao;
    
    return ambient;
}

// ============ CÁLCULO DE LUZ DIRECTA PBR ============

vec3 CalculatePBRLight(
    vec3 N, vec3 V, vec3 L, 
    vec3 radiance, 
    vec3 albedo, 
    float metallic, 
    float roughness, 
    vec3 F0
) {
    vec3 H = normalize(V + L);
    
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), EPSILON);
    float HdotV = max(dot(H, V), 0.0);
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(HdotV, F0);
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL + EPSILON;
    vec3 specular = numerator / denominator;
    
    // Energy conservation
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    
    // Lambertian diffuse + Cook-Torrance specular
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

// ============ MAIN ============

void main() {
    // Vectores base
    vec3 camPos = fragCameraPos;
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
    metallic = clamp(metallic, 0.0, 1.0);
    
    // Roughness
    float roughness;
    if (material.useRoughnessMap == 1) {
        roughness = texture(roughnessMap, fragTexCoord).g * material.roughnessFactor;
    } else {
        roughness = material.roughnessFactor;
    }
    roughness = clamp(roughness, 0.04, 1.0);
    
    // Ambient Occlusion
    float ao;
    if (material.useAOMap == 1) {
        ao = texture(aoMap, fragTexCoord).r;
    } else {
        ao = 1.0;
    }
    
    // ============ PBR LIGHTING ============
    
    // F0 (reflectancia base en incidencia normal)
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    // Acumulador de luz directa
    vec3 Lo = vec3(0.0);
    
    // Luz principal
    vec3 L_main = -mainLightDir;
    vec3 radiance_main = mainLightColor * mainLightIntensity;
    Lo += CalculatePBRLight(N, V, L_main, radiance_main, albedo, metallic, roughness, F0);
    
    // Luz de relleno
    vec3 L_fill = fillLightDir;
    vec3 radiance_fill = fillLightColor * fillLightIntensity;
    Lo += CalculatePBRLight(N, V, L_fill, radiance_fill, albedo, metallic, roughness, F0);
    
    // Luz trasera/rim (sutil)
    vec3 L_rim = rimLightDir;
    vec3 radiance_rim = rimLightColor * rimLightIntensity;
    float rimFactor = 1.0 - max(dot(N, V), 0.0);
    rimFactor = pow(rimFactor, 3.0);
    Lo += CalculatePBRLight(N, V, L_rim, radiance_rim * rimFactor, albedo, metallic, roughness, F0);
    
    // Iluminación ambiental realista (IBL simulado)
    vec3 ambient = CalculateRealisticAmbient(N, V, albedo, F0, roughness, metallic, ao);
    
    // Composición final
    vec3 color = ambient + Lo;
    
    // Tone mapping mejorado (ACES filmic)
    color = color / (color + vec3(1.0));
    
    // Pequeño ajuste de exposición
    color *= 1.0;
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    // Clamp final
    color = clamp(color, 0.0, 1.0);
    
    outColor = vec4(color, 1.0);
}