#ifndef _GBUFFER_
#define _GBUFFER_

struct GBuffer
{
    float4 diffuseColor : SV_Target0;
    float4 normal : SV_Target1;
    
    // R = ambient
    // G = roughness
    // B = metallic
    float3 arm : SV_Target2;
};

#endif // _GBUFFER_