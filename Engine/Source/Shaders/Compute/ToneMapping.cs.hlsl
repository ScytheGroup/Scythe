#include "../ToneMapping/ACES.hlsli"
#include "../ToneMapping/Reinhard.hlsli"

#define ACES 0
#define REINHARD 1
#define REINHARD_JODIE 2

RWTexture2D<float4> sourceImage : register(u0);

[numthreads(8, 8, 1)]
void main(uint2 threadID : SV_DispatchThreadID)
{
#if TONE_MAPPER == ACES
    sourceImage[threadID] = float4(ACES_ToneMapping(sourceImage[threadID].rgb), 1);
#elif TONE_MAPPER == REINHARD
    sourceImage[threadID] = float4(Reinhard_ToneMapping(sourceImage[threadID].rgb), 1);
#elif TONE_MAPPER == REINHARD_JODIE
    sourceImage[threadID] = float4(Reinhard_Jodie_ToneMapping(sourceImage[threadID].rgb), 1);
#endif
}