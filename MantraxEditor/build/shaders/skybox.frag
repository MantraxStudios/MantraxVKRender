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

// Textura albedo (skybox equirectangular)
layout(binding = 1) uniform sampler2D albedoMap;

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

// Convertir coordenadas cartesianas a UV para textura equirectangular
vec2 directionToEquirectangularUV(vec3 dir) {
    // Normalizar la dirección
    vec3 normDir = normalize(dir);
    
    // Calcular UV usando coordenadas esféricas
    // atan(z, x) da el ángulo horizontal (longitud)
    // asin(y) da el ángulo vertical (latitud)
    float u = atan(normDir.z, normDir.x) / (2.0 * 3.14159265359) + 0.5;
    float v = asin(normDir.y) / 3.14159265359 + 0.5;
    
    return vec2(u, v);
}

void main() {
    vec3 color;
    
    // Usar albedo map si está disponible
    if (material.useAlbedoMap == 1) {
        // CRÍTICO: Usar la dirección del skybox, no las UVs del mesh
        // fragWorldPos contiene la dirección desde el vertex shader
        vec2 skyboxUV = directionToEquirectangularUV(fragWorldPos);
        vec4 albedoSample = texture(albedoMap, skyboxUV);
        color = albedoSample.rgb;
    } else {
        color = material.baseColorFactor.rgb;
    }
    
    // Aplicar el brightness desde baseColorFactor (multiplicativo)
    color *= material.baseColorFactor.rgb;
    
    outColor = vec4(color, 1.0);
}