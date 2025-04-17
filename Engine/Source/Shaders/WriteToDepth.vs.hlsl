#include "InputLayouts/DefaultInputLayout.hlsli"
#include "Common.hlsli"

cbuffer TransformMatrixes
{
    matrix matViewProj;
}

struct VSOutput
{
    float4 outPos : SV_Position;
#if STORE_LINEAR_DEPTH == 1    
    float3 worldPos : POSITION;
#endif
};

VSOutput main(VSInput input)
{
    float4 worldPos = mul(objectData.model, float4(input.position, 1));
    
    VSOutput vsOut;
    vsOut.outPos = mul(matViewProj, worldPos);
#if STORE_LINEAR_DEPTH == 1
    vsOut.worldPos = worldPos.xyz;
#endif
    return vsOut;
}
