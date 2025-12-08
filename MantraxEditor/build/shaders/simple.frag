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
    // ===== DIAGNÓSTICO AVANZADO DE TEXTURA =====
    vec4 texColor = texture(albedoMap, fragTexCoord);
    
    // Opción A: Multiplicar por un factor para ver si hay datos oscuros
    outColor = vec4(texColor.rgb * 10.0, 1.0); // Amplificar x10
    
    // Opción B: Ver cada canal por separado (descomentar para probar)
    // outColor = vec4(texColor.r, 0.0, 0.0, 1.0); // Solo canal ROJO
    // outColor = vec4(0.0, texColor.g, 0.0, 1.0); // Solo canal VERDE
    // outColor = vec4(0.0, 0.0, texColor.b, 1.0); // Solo canal AZUL
    // outColor = vec4(texColor.aaa, 1.0); // Ver canal ALPHA
    
    // Opción C: Ver si hay ALGÚN dato (descomentar para probar)
    // float brightness = (texColor.r + texColor.g + texColor.b) / 3.0;
    // outColor = vec4(vec3(brightness * 50.0), 1.0); // Amplificar brillo
    
    /* ILUMINACIÓN FINAL
    vec4 texColor = texture(albedoMap, fragTexCoord);
    
    // Fallback: si la textura es negra, usar gris
    vec3 albedo = texColor.rgb;
    if (length(albedo) < 0.01) {
        albedo = vec3(0.5); // Gris por defecto
    }
    
    vec3 N = normalize(fragNormal);
    vec3 lightDir = normalize(vec3(1.0, 1.0, 0.5));
    vec3 viewDir = normalize(fragCameraPos - fragWorldPos);
    
    vec3 ambient = vec3(0.3);
    float diff = max(dot(N, lightDir), 0.0);
    vec3 diffuse = vec3(0.8) * diff;
    
    vec3 reflectDir = reflect(-lightDir, N);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = vec3(0.5) * spec;
    
    vec3 result = (ambient + diffuse + specular) * albedo;
    outColor = vec4(result, 1.0);
    */
}