#version 430

layout(location = 1) in vec3 Normal;
layout(location = 0) out vec4 FragColor;

void main() {
    FragColor = vec4(abs(Normal), 1);
}
