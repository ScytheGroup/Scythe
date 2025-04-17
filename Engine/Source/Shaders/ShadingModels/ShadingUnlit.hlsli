#ifndef _SHADING_UNLIT_
#define _SHADING_UNLIT_

float3 EvaluateMaterialUnlit(float4 diffuseColor, float3 worldPosition, float3 normal, float2 uv)
{
    return diffuseColor.rgb;
}

#endif // _SHADING_UNLIT_