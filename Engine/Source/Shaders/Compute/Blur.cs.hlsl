Texture2D Source : register(t0);
RWTexture2D<float4> UAV : register(u0);

cbuffer BlurParams
{
    int kernelSize;
};

[numthreads(8, 8, 1)]
void main(uint2 threadID : SV_DispatchThreadID)
{
    const uint halfBlur = uint(kernelSize) / 2;
    float4 result = 0.0;
    for (int x = -int(halfBlur); x < -int(halfBlur) + kernelSize; ++x)
    {
        for (int y = -int(halfBlur); y < -int(halfBlur) + kernelSize; ++y)
        {
            float2 offset = float2(float(x), float(y));
            result += float4(Source.Load(int3(threadID + offset, 0)).rgb, 1);
        }
    }
    
    UAV[threadID] = result / pow(kernelSize, 2);
}