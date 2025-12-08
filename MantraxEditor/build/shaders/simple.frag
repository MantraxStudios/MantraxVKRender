#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 cameraPosition;
} ubo;

layout(binding = 1) uniform sampler2D albedoMap;

void main() {
    // Muestrear la textura
    vec4 texColor = texture(albedoMap, fragTexCoord);
    
    // Aplicar corrección gamma 2.2 (convertir de sRGB a linear)
    vec3 linearColor = pow(texColor.rgb, vec3(2.2));
    
    // Multiplicar por el color del vértice
    linearColor *= fragColor;
    
    // Convertir de vuelta a sRGB para display (gamma correction)
    vec3 finalColor = pow(linearColor, vec3(1.0 / 2.2));
    
    outColor = vec4(finalColor, texColor.a);
}