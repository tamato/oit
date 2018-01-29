#version 430
#extension GL_ARB_shader_image_load_store : enable

layout(location = 0) in float Depth;

layout(rg16f, location=1) coherent uniform image2DArray CavityVolume;
layout(r32ui, location=2) coherent uniform uimage2D Counter;

void main() {
    ivec2 loc = ivec2(gl_FragCoord.xy);
    uint layer = imageAtomicAdd(Counter, loc, uint(1));
    imageStore(CavityVolume, ivec3(loc, layer), vec4(Depth));
    discard;
}
