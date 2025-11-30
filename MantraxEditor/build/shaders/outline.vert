#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

void main()
{
    // Ancho del outline reducido para mejor visualización
    float outlineWidth = 0.015;

    // Transformar posición al espacio mundial
    vec4 worldPos = ubo.model * vec4(inPosition, 1.0);
    
    // Transformar normal al espacio mundial y normalizar
    vec3 worldNormal = normalize(mat3(ubo.model) * inNormal);
    
    // Extruir en el espacio mundial (más estable que en view space)
    worldPos.xyz += worldNormal * outlineWidth;
    
    // Transformar al clip space
    gl_Position = ubo.projection * ubo.view * worldPos;
}