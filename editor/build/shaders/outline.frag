#version 450

layout(location = 0) out vec4 outColor;

void main() {
    // Color del outline (negro puro)
    outColor = vec4(0.0, 0.0, 0.0, 1.0);
    
    // Alternativamente, podrías usar:
    // outColor = vec4(0.1, 0.05, 0.2, 1.0); // Outline morado oscuro
    // outColor = vec4(1.0, 1.0, 1.0, 1.0); // Outline blanco
}