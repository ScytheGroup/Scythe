#include "Voxel.hlsli"

RWStructuredBuffer<uint> Voxels;
RWStructuredBuffer<float2> Normals;

[numthreads(8, 8, 8)]
void main(uint3 threadID : SV_DispatchThreadID)
{
    uint3 voxelID = threadID;
    if (threadID.x >= volumeResolution || threadID.y >= volumeResolution || threadID.z >= volumeResolution)
        return;
    
    uint idx = VoxelCoordToArrayIndex(voxelID);
    Voxels[idx] = 0;
    Normals[idx] = 0;    
}