#include "Common.hlsli"
#include "Samplers.hlsli"
#include "Color.hlsli"
#include "Math.hlsli"

#ifdef EQUIRECTANGULAR_CUBEMAP
Texture2D SkyboxTexture;
static const float2 invAtan = float2(0.1591, 0.3183);
#else
TextureCube SkyboxTexture;
#endif

float4 SampleCubemap(float3 v)
{
#ifdef EQUIRECTANGULAR_CUBEMAP
    float2 uv = float2(atan2(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return SkyboxTexture.Sample(LinearSampler, uv);
#else
    return SkyboxTexture.SampleLevel(LinearSampler, v, 0);
#endif
}

struct PSInput
{
    float4 outPosition: SV_POSITION;
    float3 viewDirection : POSITION;
};

struct PSOutput
{
    float4 outColor : SV_Target0;
#if !defined(NO_NORMAL_WRITE)
    float4 normal : SV_Target1;
#endif
    float depth : SV_Depth;
};

PSOutput main(PSInput input)
{
    PSOutput output;
    output.outColor = SampleCubemap(normalize(input.viewDirection));
    output.depth = 1.0f;
#if !defined(NO_NORMAL_WRITE)
    output.normal.z = SHADING_MODEL_UNLIT;
    // to see in the editor
    output.normal.w = 1.0f;
#endif
    return output;
}