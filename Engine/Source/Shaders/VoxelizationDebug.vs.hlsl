#include "Voxel.hlsli"

uint3 main(uint vertexId : SV_VertexID) : POSITION 
{
    int x = vertexId % volumeResolution;
    int y = (vertexId / volumeResolution) % volumeResolution;
    int z = vertexId / (volumeResolution * volumeResolution); 
    return uint3(x, y, z);
}