#include "ToneMapping/ACES.hlsli"
#include "Voxel.hlsli"

StructuredBuffer<uint> Voxels;
RWTexture3D<float4> IlluminationVolume;

[numthreads(8, 8, 8)]
void main(uint3 threadID : SV_DispatchThreadID)
{
    if (threadID.x >= volumeResolution || threadID.y >= volumeResolution || threadID.z >= volumeResolution)
        return;
    
    uint packedData = Voxels[VoxelCoordToArrayIndex(threadID)];
    VoxelType voxel = UnpackVoxelType(packedData);

    float4 prevColor = IlluminationVolume[threadID];

    IlluminationVolume[threadID] = voxel.isSet * lerp(float4(voxel.color, 1), prevColor, TemporalUpdateAlpha);
}