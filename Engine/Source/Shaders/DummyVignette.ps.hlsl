#include "Math.hlsli"

struct PSInput
{
    float4 position: SV_POSITION;
    float2 uv : TEXCOORD;
};

struct PSOutput
{
    float4 color: SV_Target;
};

PSOutput main(PSInput input)
{
    PSOutput output;
    float dist = distance(input.uv, float2(0.5, 0.5));
    output.color = float4(0, 0, 0, pow(smoothstep(0, 1, dist), 2));
    return output;
}