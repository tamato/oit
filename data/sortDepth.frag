#version 430

layout(rg16f) coherent uniform image2DArray CavityVolume;
layout(r32ui) coherent uniform uimage2D Counter;

#define ABUFFER_SIZE 16
vec4 depthsList[ABUFFER_SIZE];
void fillFragmentArray(ivec2 coords, uint max_layers);
void saveSortedValues(ivec2 coords, uint max_layers);

//Bitonic sort test, http://www.tools-of-computing.com/tc/CS/Sorts/bitonic_sort.htm
void bitonicSort( int n );
void bubbleSort(int array_size);

void main() {
    ivec2 loc = ivec2(gl_FragCoord.xy);
    uint max_layers = imageLoad(Counter, loc).r;

    // collect depth values
    fillFragmentArray(loc, max_layers);

    // sort them
    bubbleSort( int(max_layers) );

    // fill the texture right back out.
    saveSortedValues(loc, max_layers);
    discard;
}

void fillFragmentArray(ivec2 coords, uint max_layers){
    //Load fragments into a local memory array for sorting
    for(uint i=0; i<max_layers; i++){
        depthsList[i]=imageLoad(CavityVolume, ivec3(coords, i));
    }
}

void saveSortedValues(ivec2 coords, uint max_layers) {
    //store depth values back into the global array
    for(uint i=0; i<max_layers; i++){
        imageStore(CavityVolume, ivec3(coords, i), depthsList[i]);
    }
}

//Bubble sort used to sort fragments
void bubbleSort(int array_size) {
    for (int i = (array_size - 2); i >= 0; --i) {
        for (int j = 0; j <= i; ++j) {
            if (depthsList[j].x > depthsList[j+1].x) {
                vec4 temp = depthsList[j+1];
                depthsList[j+1] = depthsList[j];
                depthsList[j] = temp;
            }
        }
    }
}

//Swap function
void swapFragArray(int n0, int n1){
    vec4 temp = depthsList[n1];
    depthsList[n1] = depthsList[n0];
    depthsList[n0] = temp;
}

//->Same artefact than in L.Bavoil
//Bitonic sort: http://www.tools-of-computing.com/tc/CS/Sorts/bitonic_sort.htm
void bitonicSort( int n ) {
    int i,j,k;
    for (k=2;k<=n;k=2*k) {
        for (j=k>>1;j>0;j=j>>1) {
            for (i=0;i<n;i++) {
              int ixj=i^j;
              if ((ixj)>i) {
                  if ((i&k)==0 && depthsList[i].x>depthsList[ixj].x){
                    swapFragArray(i, ixj);
                  }
                  if ((i&k)!=0 && depthsList[i].x<depthsList[ixj].x) {
                    swapFragArray(i, ixj);
                  }
              }
            }
        }
    }
}
