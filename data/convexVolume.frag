#version 430

layout(location = 0) in vec3 Normal;
layout(location = 1) in float Depth;

layout(location = 0) out vec2 FragColor;

void main() {
    FragColor = vec2(0);
    if (gl_FrontFacing) FragColor.r = Depth;
    else                FragColor.g = Depth;
}
