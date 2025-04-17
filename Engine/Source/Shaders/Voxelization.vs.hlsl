#include "Common.hlsli"
#include "InputLayouts/DefaultInputLayout.hlsli"

struct VSOutput 
{
    float4 outPosition : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

VSOutput main(VSInput input) 
{
    VSOutput vout;
    vout.outPosition = mul(objectData.model, float4(input.position, 1));
    vout.normal = mul(objectData.normalMatrix, float4(input.normal, 1)).xyz;
    vout.uv = input.uv;

    return vout;
}