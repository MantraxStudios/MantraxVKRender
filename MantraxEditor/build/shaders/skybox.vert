#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragWorldPos;
layout(location = 4) out vec3 fragCameraPos;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 cameraPosition;
} ubo;

void main() {
    fragWorldPos = inPosition;
    fragNormal = normalize(inPosition);
    
    // Remover traslación, mantener rotación
    mat4 viewNoTranslation = mat4(mat3(ubo.view));
    
    // Calcular posición normalmente
    vec4 pos = ubo.projection * viewNoTranslation * vec4(inPosition, 1.0);
    
    // SOLUCIÓN: Empujar el skybox hacia atrás ligeramente
    // En lugar de forzar z = w, reducimos z un poco
    gl_Position = vec4(pos.xy, pos.w * 0.9999, pos.w);
    
    // Esto da depth = 0.9999, justo antes del far plane
    // Los objetos tendrán depth < 0.9999, así que siempre estarán adelante
    
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    fragCameraPos = ubo.cameraPosition.xyz;
}