#include "Math.hlsli"
#include "Samplers.hlsli"
#include "Voxel.hlsli"

Texture3D IlluminationVolume;
StructuredBuffer<float2> NormalsBuffer;
RWStructuredBuffer<uint> DestinationBuffer;

cbuffer BounceInformation
{
    int bounceNumber;
}

[numthreads(8, 8, 8)]
void main(uint3 threadID : SV_DispatchThreadID)
{
    if (threadID.x >= volumeResolution || threadID.y >= volumeResolution || threadID.z >= volumeResolution)
        return;

    uint voxelArrayIndex = VoxelCoordToArrayIndex(threadID);

    float2 packedNormal = NormalsBuffer[voxelArrayIndex];
    if (packedNormal.x == 0 && packedNormal.y == 0)
    {
        DestinationBuffer[voxelArrayIndex] = 0;
        return;
    }

    float3 worldPosition = VoxelIndexToWorldPos(threadID);
    float3 normal = NormalizedOctahedronDecode(packedNormal);

    float3 color = IlluminationVolume.Load(uint4(threadID, 0)).rgb;
    float4 coneTracedColor = ConeTraceInVoxelIlluminationVolume(IlluminationVolume, PointSampler, worldPosition, normal);
    float3 newColor = color + coneTracedColor.rgb * 1.0f / (bounceNumber + 1);

    VoxelType vox;
    // Might need to tone map here
    vox.color = newColor;
    vox.isSet = true;
    DestinationBuffer[voxelArrayIndex] = PackVoxelType(vox);
}