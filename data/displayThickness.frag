#version 430

layout(location=0) in vec2 TexCoords;

uniform sampler2D FustrumVolume;

layout(location=0) out vec4 FragColor;

void main() {
    vec4 range = texture(FustrumVolume, TexCoords);
    float thickness = range.g - range.r;
    FragColor = vec4(.5, 0, 0, thickness * .01);
}
