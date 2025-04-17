#include "Math.hlsli"
#include "AtmosphericScattering.hlsli"

const static uint SunTransmittanceSamples = 40;

// Precondition: the worldPos must be inside the atmosphere!
float3 GetSunTransmittance(float3 worldPos, float3 sunDir)
{
    // If the sun is on the other side of the planet or we are in the ground, transmittance is 0.
    if (RaySphereIntersect(worldPos, sunDir, PlanetRadius) > 0)
    {
        return 0;
    }

    float distAtmosphere = RaySphereIntersect(worldPos, sunDir, AtmosphereRadius);
    float rayMarchLength = distAtmosphere;

    // Start raymarch at first intersection point
    float3 samplePos = worldPos;
    float stepSize = rayMarchLength / (SunTransmittanceSamples - 1);

    float3 transmittance = 1;
    for (uint i = 0; i < SunTransmittanceSamples; ++i)
    {
        // World unit is in meters, altitude is in m.
        // In this LUT, we suppose worldPos is in km to avoid converting.
        float altitude = length(samplePos) - PlanetRadius;

        // _ and __ will be optimized out, we only need extinction here to compute transmittance
        float3 _, extinction;
        float __;
        GetScatteringValues(altitude, _, __, extinction);

        transmittance *= exp(-stepSize * extinction);
        samplePos += sunDir * stepSize;
    }
    return transmittance;
}

cbuffer TextureSize : register(b12)
{
    uint2 textureSize;
}

RWTexture2D<float4> TransmittanceLUT;

[numthreads(8, 8, 1)]
void main(uint2 dispatchThreadID : SV_DispatchThreadID)
{
    float2 uv = (float2(dispatchThreadID) + 0.5f.xx) / textureSize;

    float sunCosTheta = 2 * uv.x - 1;
    float sunTheta = acos(clamp(sunCosTheta, -1.0, 1.0));
    float height = lerp(PlanetRadius, AtmosphereRadius, uv.y);

    float3 worldPos = float3(0, height, 0);
    float3 sunDir = normalize(float3(0, sunCosTheta, -sin(sunTheta)));

    TransmittanceLUT[dispatchThreadID] = float4(GetSunTransmittance(worldPos, sunDir), 1);
}