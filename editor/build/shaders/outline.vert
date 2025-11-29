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
    float outlineWidth = 0.03;

    vec4 viewPos = ubo.view * ubo.model * vec4(inPosition, 1.0);

    vec3 viewNormal = normalize(mat3(ubo.view * ubo.model) * inNormal);

    viewPos.xyz += viewNormal * outlineWidth;

    gl_Position = ubo.projection * viewPos;
}
