#include "../Samplers.hlsli"
#include "../Color.hlsli"

Texture2D InputTexture : register(t0);
RWTexture2D<float> OutputTexture : register(u0);

[numthreads(8, 8, 1)]
void main(uint2 threadID : SV_DispatchThreadID)
{
#ifdef FIRST_PASS
    float4 topLeft = InputTexture.Load(int3(threadID * 2 + int2(0,0), 0));
    float4 topRight = InputTexture.Load(int3(threadID * 2 + int2(1,0), 0));
    float4 bottomLeft = InputTexture.Load(int3(threadID * 2 + int2(0,1), 0));
    float4 bottomRight = InputTexture.Load(int3(threadID * 2 + int2(1,1), 0));

    float lum0 = ComputeLuminance(topLeft);
    float lum1 = ComputeLuminance(topRight);
    float lum2 = ComputeLuminance(bottomLeft);
    float lum3 = ComputeLuminance(bottomRight);
#else
    float lum0 = InputTexture.Load(int3(threadID * 2 + int2(0,0), 0));
    float lum1 = InputTexture.Load(int3(threadID * 2 + int2(1,0), 0));
    float lum2 = InputTexture.Load(int3(threadID * 2 + int2(0,1), 0));
    float lum3 = InputTexture.Load(int3(threadID * 2 + int2(1,1), 0));
#endif
    
    OutputTexture[threadID] = max(max(lum0, lum1), max(lum2, lum3));
}