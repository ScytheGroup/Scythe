#include "Common.hlsli"
#define DECLARE_TRANSMITTANCE_LUT
#define DECLARE_MS_LUT
#include "AtmosphericScattering.hlsli"

struct PSInput
{
    float4 outPosition: SV_POSITION;
    float3 viewDirection : POSITION;
};

float4 main(PSInput input) : SV_TARGET
{
    input.viewDirection.y *= -1;
    float3 rayDir = normalize(input.viewDirection);
    float3 sunDir = normalize(-frameData.dirLight.direction);
    
    // Use the same view position as in cubemap generation
    float3 viewPos = float3(0, 0.001f, 0);
    
    float3 light = RaymarchScattering(viewPos, rayDir, sunDir, 1.0f, 1000000.0f);

    light += frameData.dirLight.intensity * SunWithBloom(rayDir, sunDir) * frameData.dirLight.color;
    
    return float4(light * AtmosphereIntensity, 1);
}