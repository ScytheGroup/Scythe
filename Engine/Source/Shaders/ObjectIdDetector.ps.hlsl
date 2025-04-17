#ifdef SPRITE
#include "Samplers.hlsli"
#endif

cbuffer ObjectId
{
    uint objectId;
}

#ifdef SPRITE
Texture2D<float4> SpriteTexture;
#endif

#if defined(FULLSCREEN_PASS) || defined(SPRITE)
struct PSInput 
{
    float4 outPosition: SV_POSITION;
    float2 uv : TEXCOORD;
};
#else
struct PSInput 
{
    float4 outPosition: SV_POSITION;

    float3 worldPosition : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};
#endif

void main(PSInput input, out uint outObjectId : SV_Target)
{
#ifdef SPRITE
    if (SpriteTexture.Sample(PointSampler, input.uv).a < 0.1f)
    {
        outObjectId = 0;
        discard;
        return;
    }
#endif
    outObjectId = objectId;
}