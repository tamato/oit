#version 430

layout(location = 0) in vec3 Normal;
layout(location = 1) in float Depth;

uniform vec4 Color = vec4(0,1,1,1);

layout(rg16f) coherent uniform image2DArray CavityVolume;
layout(r32ui) coherent uniform uimage2D Counter;

#define ABUFFER_SIZE 16
vec4 depthsList[ABUFFER_SIZE];
void fillDepthsArray(ivec2 coords, uint max_layers);

layout(location = 0) out vec4 FragColor;

void main() {
    ivec2 loc = ivec2(gl_FragCoord.xy);
    uint max_layers = imageLoad(Counter, loc).r;
    fillDepthsArray(loc, max_layers);

    for (int layer=0; layer < max_layers; layer+=2) {
        vec4 cavity_depth0 = depthsList[layer+0];
        vec4 cavity_depth1 = depthsList[layer+1];
        if (Depth > cavity_depth0.x && Depth < cavity_depth1.x)
            discard;
    }

    float falloff = dot(vec3(0,0,1), abs(Normal));
    FragColor = Color * falloff;
    FragColor.a = 1;
}

void fillDepthsArray(ivec2 coords, uint max_layers){
    //Load fragments into a local memory array for sorting
    for(uint i=0; i<max_layers; i++){
        depthsList[i]=imageLoad(CavityVolume, ivec3(coords, i));
    }
}
