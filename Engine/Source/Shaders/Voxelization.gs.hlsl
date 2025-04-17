#include "Voxel.hlsli"

struct VSOutput 
{
    float4 outPosition : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct GSOutput
{
    float4 outPosition : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 P : POSITION3D;
};

[maxvertexcount(3)]
void main(triangle VSOutput input[3], inout TriangleStream<GSOutput> OutputStream)
{   
    GSOutput output[3];
    
    float3 facenormal = abs(input[0].normal + input[1].normal + input[2].normal);
    uint maxi = facenormal[1] > facenormal[0] ? 1 : 0;
    maxi = facenormal[2] > facenormal[maxi] ? 2 : maxi;

    uint i;
    for (i = 0; i < 3; ++i)
    {
        // voxel space pos:
        output[i].outPosition = float4((input[i].outPosition.xyz - GetVoxelBoundsCenter()) / GetHalfVoxelSize(), 1);

        // Project onto dominant axis:
        if (maxi == 0)
        {
            output[i].outPosition.xyz = output[i].outPosition.zyx;
        }
        else if (maxi == 1)
        {
            output[i].outPosition.xyz = output[i].outPosition.xzy;
        }
    }

    // Expand triangle to get fake Conservative Rasterization:
    float2 side0N = normalize(output[1].outPosition.xy - output[0].outPosition.xy);
    float2 side1N = normalize(output[2].outPosition.xy - output[1].outPosition.xy);
    float2 side2N = normalize(output[0].outPosition.xy - output[2].outPosition.xy);
    output[0].outPosition.xy += normalize(side2N - side0N);
    output[1].outPosition.xy += normalize(side0N - side1N);
    output[2].outPosition.xy += normalize(side1N - side2N);

    for (i = 0; i < 3; ++i)
    {
        // projected pos:
        output[i].outPosition.xy /= volumeResolution;
        output[i].outPosition.zw = 1;

        output[i].normal = input[i].normal;
        output[i].uv = input[i].uv;
        output[i].P = input[i].outPosition.xyz; 
        
        OutputStream.Append(output[i]);
    }
    
    OutputStream.RestartStrip();
}