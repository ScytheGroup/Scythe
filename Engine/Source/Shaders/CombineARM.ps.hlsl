#include "Samplers.hlsli"

Texture2D ambientTex : register(t0);
Texture2D roughnessTex : register(t1);
Texture2D metallicTex : register(t2);

struct PSInput 
{
    float4 outPosition: SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 main(PSInput input) : SV_Target
{
    float4 ao = ambientTex.Sample(WrapLinearSampler, input.uv);
    float4 roughness = roughnessTex.Sample(WrapLinearSampler, input.uv);
    float4 metallic = metallicTex.Sample(WrapLinearSampler, input.uv);
    return float4(ao.r, roughness.g, metallic.b, 1);
}