#version 430

layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoords;

layout(location = 0) out vec2 OutTexCoords;

void main() {
    gl_Position = Position;
    OutTexCoords = TexCoords;
}
