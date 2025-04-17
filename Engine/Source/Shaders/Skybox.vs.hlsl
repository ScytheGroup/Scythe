#include "Common.hlsli"
#include "InputLayouts/DefaultInputLayout.hlsli"

struct VSOutput 
{
    float4 outPosition : SV_POSITION;
    float3 viewDirection : POSITION;
};

VSOutput main(VSInput input) 
{
    VSOutput vOut;
    vOut.outPosition = mul(frameData.projection, mul(objectData.model, float4(input.position, 1)));
    vOut.viewDirection = mul(frameData.invView, float4(input.position, 0)).xyz;
    return vOut;
}