#version 430

layout(rg16f) coherent writeonly uniform image2DArray CavityVolume;
layout(r32ui) coherent writeonly uniform uimage2D Counter;

layout(rg16f) coherent writeonly uniform image2DArray ValvesVolume;
layout(r32ui) coherent writeonly uniform uimage2D ValvesCounter;

void main() {
    ivec2 loc = ivec2(gl_FragCoord.xy);
    imageStore(Counter, loc, ivec4(0));
    imageStore(CavityVolume, ivec3(loc, 0), vec4(0));

    imageStore(ValvesCounter, loc, ivec4(0));
    imageStore(ValvesVolume, ivec3(loc, 0), vec4(0));
    discard;
}
