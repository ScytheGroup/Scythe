#include "Samplers.hlsli"

Texture2D SpriteTexture;

struct PSInput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
};

float4 main(PSInput input) : SV_TARGET
{
    input.uv.y = 1 - input.uv.y;
    return SpriteTexture.Sample(AnisotropicSampler, input.uv);
}