#include "Voxel.hlsli"
#include "Math.hlsli"
#include "Common.hlsli"

struct GSOutput
{
    float4 position : SV_Position;
    float3 color : COLOR;
};

StructuredBuffer<uint> Voxels;
//StructuredBuffer<float2> Normals;

static float3 Vertices[8] = {
    float3(0.500000, -0.500000, -0.500000),
    float3(0.500000, -0.500000, 0.500000),
    float3(-0.500000, -0.500000, 0.500000),
    float3(-0.500000, -0.500000, -0.500000),
    float3(0.500000, 0.500000, -0.500000),
    float3(0.500000, 0.500000, 0.500000),
    float3(-0.500000, 0.500000, 0.500000),
    float3(-0.500000, 0.500000, -0.500000),
};

static uint Indices[36] = {
    3, 2, 7, // Z FRONT
    2, 6, 7, // Z FRONT

    5, 1, 4, // Z BACK
    8, 5, 4, // Z BACK
    
    2, 3, 4, // Y DOWN
    1, 2, 4, // Y DOWN

    5, 8, 7, // Y UP
    6, 5, 7, // Y UP
    
    3, 7, 4, // X LEFT
    7, 8, 4, // X LEFT
    
    5, 6, 2, // X RIGHT
    1, 5, 2, // X RIGHT
};

[maxvertexcount(36)]
void main(point uint3 input[1] : POSITION, inout TriangleStream<GSOutput> OutputStream)
{
    uint3 index = input[0].xyz;

    GSOutput output;

    uint arrayIndex = VoxelCoordToArrayIndex(index);
    uint voxelData = Voxels[arrayIndex];
    //float3 normals =  NormalizedOctahedronDecode(Normals[arrayIndex]);
    VoxelType vox = UnpackVoxelType(voxelData);
    
    if (!vox.isSet)
         return;

    float3 pos = VoxelIndexToWorldPos(index);

    static int3 offsets[] = {
        int3(0, 0, 1),
        int3(0, 0, -1),

        int3(0, -1, 0),
        int3(0, 1, 0),
        
        int3(-1, 0, 0),
        int3(1, 0, 0),
    };

    uint i = 0;
    for (i = 0; i < 6; ++i)
    {
        int3 offset = index + offsets[i];
        uint neighbor = Voxels[VoxelCoordToArrayIndex(offset)];

        if (neighbor & 1)
            continue;

        for (int j = 0; j < 6; ++j)
        {
            output.position = mul(frameData.viewProjection, float4(pos + Vertices[Indices[i * 6 + j] - 1] * GetVoxelSize(), 1));
            output.color = vox.color.rgb;
            OutputStream.Append(output);

            if (j == 2)
                OutputStream.RestartStrip(); 
        }
        OutputStream.RestartStrip(); 
    }
}