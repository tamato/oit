#version 430

layout(location = 0) in vec3 Normal;
layout(location = 1) in float Depth;

uniform vec4 ColorGradient0 = vec4(1,0,1,1);
uniform vec4 ColorGradient1 = vec4(1,0,1,1);
uniform vec4 ColorMinimum = vec4(1,0,1,1);
uniform vec2 ColorDepthRange = vec2(100,1000);
uniform vec2 Resolution = vec2(1024);

uniform sampler2D FustrumVolume;

layout(location = 0) out vec4 FragColor;

void main() {
    vec2 loc = gl_FragCoord.xy / Resolution;
    vec4 depthRange = texture(FustrumVolume, loc);

    if (Depth < depthRange.x || Depth > depthRange.y)
        discard;

    float depth_start = max(0, Depth - ColorDepthRange.r);
    float range = ColorDepthRange.g - ColorDepthRange.r;
    float percent = min(1, depth_start / range);
    vec4 color = percent * (ColorGradient1 - ColorGradient0) + ColorGradient0;

    float falloff = dot(vec3(0,0,1), abs(Normal));
    FragColor = falloff * (color - ColorMinimum) + ColorMinimum;
}

