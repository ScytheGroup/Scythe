#include "Samplers.hlsli"
#include "SH.hlsli"

// Skybox Cubemap
TextureCube InputTexture : register(t0);

RWStructuredBuffer<ColorSH9> OutputBuffer : register(u0);

static const float MaxSHColorValue = 1000.0f;

[numthreads(1, 1, 1)]
void main(uint2 threadID : SV_DispatchThreadID)
{
    ColorSH9 result;
    
    float samples = 1024;
    
    for (int i = 0; i < samples; ++i)
    {
        float index = i + 0.5;
        float phi = acos(1 - 2 * i / samples);
        float theta = PI * (1 + sqrt(5)) * index;

        float3 p = float3(cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi));

        float3 direction = normalize(p);
        
        float3 color = InputTexture.SampleLevel(LinearSampler, direction, 0).rgb;
                
        color = clamp(color, 0, MaxSHColorValue);
    
        result = SumSH9(result, ProjectSH9(direction, color));
    }
    
    result = MultiplySH9(result, (4.0f * PI) / samples);

    OutputBuffer[0] = result;
}