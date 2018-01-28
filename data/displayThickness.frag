#version 430

layout(location=0) in vec2 TexCoords;

layout(binding=0) uniform sampler2D Volume;

layout(location=0) out vec4 FragColor;

void main() {
    vec4 range = texture(Volume, TexCoords);
    float thickness = range.g - range.r;
    FragColor = vec4(.5, 0, 0, thickness * .01);
}
