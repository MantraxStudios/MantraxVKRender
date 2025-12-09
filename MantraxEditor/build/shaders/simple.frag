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

layout(binding = 1) uniform sampler2D albedoMap;

void main() {
    // Samplear textura
    vec4 texColor = texture(albedoMap, fragTexCoord);
    
    // Alpha test para texturas con cortes
    if (texColor.a < 0.1) {
        discard;
    }
    
    // Iluminación mejorada con más brillo
    vec3 N = normalize(fragNormal);
    vec3 lightDir = normalize(vec3(1.0, 1.0, 0.5));
    vec3 viewDir = normalize(fragCameraPos - fragWorldPos);
    
    // ✅ Ambient MÁS ALTO (más luz base)
    vec3 ambient = vec3(0.6); // Era 0.3, ahora 0.6 (doble)
    
    // ✅ Diffuse MÁS INTENSO
    float diff = max(dot(N, lightDir), 0.0);
    vec3 diffuse = vec3(1.2) * diff; // Era 0.8, ahora 1.2
    
    // ✅ Specular más brillante
    vec3 reflectDir = reflect(-lightDir, N);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = vec3(0.8) * spec; // Era 0.5, ahora 0.8
    
    // Combinar iluminación
    vec3 result = (ambient + diffuse + specular) * texColor.rgb;
    
    // ✅ OPCIONAL: Aplicar un multiplicador general de brillo
    result *= 1.2; // Aumentar brillo general un 20%
    
    // Salida final
    outColor = vec4(result, texColor.a);
}