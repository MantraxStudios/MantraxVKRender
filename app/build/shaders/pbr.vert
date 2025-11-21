#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragWorldPos;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

void main() {
    // Calcular posición en espacio mundial
    vec4 worldPos = ubo.model * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;
    
    // Transformar normal al espacio mundial (sin traslación)
    fragNormal = mat3(ubo.model) * inNormal;
    
    // Posición final en clip space
    gl_Position = ubo.projection * ubo.view * worldPos;
    
    // Pasar datos al fragment shader
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}