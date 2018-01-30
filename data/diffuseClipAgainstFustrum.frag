#version 430

layout(location = 0) in vec3 Normal;
layout(location = 1) in float Depth;

uniform vec4 Color = vec4(0,1,1,1);
uniform vec2 WindowSize = vec2(1024);

uniform sampler2D FustrumVolume;

layout(location = 0) out vec4 FragColor;

void main() {
    vec2 loc = gl_FragCoord.xy / WindowSize;
    vec4 depthRange = texture(FustrumVolume, loc);

    if (Depth < depthRange.x || Depth > depthRange.y)
        discard;

    float falloff = dot(vec3(0,0,1), abs(Normal));
    FragColor = Color * falloff;
    FragColor.a = 1;
}

