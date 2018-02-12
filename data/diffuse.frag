#version 430

layout(location = 0) in vec3 Normal;

uniform vec4 Color = vec4(0,1,1,1);

layout(location = 0) out vec4 FragColor;

void main() {
    float falloff = dot(vec3(0,0,1), abs(Normal));
    FragColor = Color;
    FragColor.a = 1.0 - falloff;
}
