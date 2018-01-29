#version 430

layout(rg16f) coherent uniform image2DArray CavityVolume;
layout(r32ui) coherent uniform uimage2D Counter;

#define ABUFFER_SIZE 16
float fragmentList[ABUFFER_SIZE];
void fillFragmentArray(ivec2 coords, uint abNumFrag);
void saveSortedValues(ivec2 coords, uint max_layers);

//Bitonic sort test, http://www.tools-of-computing.com/tc/CS/Sorts/bitonic_sort.htm
void bitonicSort( int n );
void bubbleSort(uint array_size);

void main() {
    ivec2 loc = ivec2(gl_FragCoord.xy);
    uint max_layers = imageLoad(Counter, loc).r;

    // collect depth values
    fillFragmentArray(loc, max_layers);

    // sort them
    bubbleSort(max_layers);

    // fill the texture right back out.
    saveSortedValues(loc, max_layers);
    memoryBarrier();
    discard;
}

void fillFragmentArray(ivec2 coords, uint abNumFrag){
    //Load fragments into a local memory array for sorting
    for(uint i=0; i<abNumFrag; i++){
        fragmentList[i]=imageLoad(CavityVolume, ivec3(coords, i)).r;
    }
}

void saveSortedValues(ivec2 coords, uint max_layers) {
    //store depth values back into the global array
    for(uint i=0; i<max_layers; i++){
        imageStore(CavityVolume, ivec3(coords, i), vec4(fragmentList[i]));
    }
}

//Bubble sort used to sort fragments
void bubbleSort(uint array_size) {
  for (uint i = (array_size - 2); i >= 0; --i) {
    for (uint j = 0; j <= i; ++j) {
      if (fragmentList[j] > fragmentList[j+1]) {
        float temp = fragmentList[j+1];
        fragmentList[j+1] = fragmentList[j];
        fragmentList[j] = temp;
      }
    }
  }
}

//Swap function
void swapFragArray(int n0, int n1){
    float temp = fragmentList[n1];
    fragmentList[n1] = fragmentList[n0];
    fragmentList[n0] = temp;
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
                  if ((i&k)==0 && fragmentList[i]>fragmentList[ixj]){
                    swapFragArray(i, ixj);
                  }
                  if ((i&k)!=0 && fragmentList[i]<fragmentList[ixj]) {
                    swapFragArray(i, ixj);
                  }
              }
            }
        }
    }
}
