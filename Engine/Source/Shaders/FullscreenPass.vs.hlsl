#include "InputLayouts/DefaultInputLayout.hlsli"
#include "PixelShaderHelpers.hlsli"

struct VSOutput 
{
    float4 outPosition: SV_POSITION;
    float2 uv : TEXCOORD;
};

VSOutput main(uint vId : SV_VertexId)
{
    float4 position;
    float2 texcoord;
    ComputeTriangleVertex(2 - vId, position, texcoord);

    VSOutput vout;
    vout.outPosition = position;
    vout.uv = texcoord;
    return vout;
}