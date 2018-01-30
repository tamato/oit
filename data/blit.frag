#version 430

layout(location=0) in vec2 TexCoords;

uniform sampler2D Results;

layout(location=0) out vec4 FragColor;

void main() {
    FragColor = texture(Results, TexCoords);
}
