#version 430

layout(location = 0) in vec4 Position;
layout(location = 1) in vec3 Normal;

layout(location = 0) uniform mat4 ProjectionView;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out float Depth;

void main() {
    gl_Position = ProjectionView * Position;
    outNormal = Normal;

    Depth = gl_Position.z;
}
