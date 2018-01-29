#version 430

layout(rg16f) coherent uniform image2DArray CavityVolume;
layout(r32ui) coherent uniform uimage2D Counter;

void main() {
    ivec2 loc = ivec2(gl_FragCoord.xy);
    imageStore(Counter, loc, ivec4(0));
    imageStore(CavityVolume, ivec3(loc, 0), vec4(0));
    discard;
}
