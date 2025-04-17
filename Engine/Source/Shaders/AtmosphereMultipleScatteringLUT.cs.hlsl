#include "Math.hlsli"
#define DECLARE_TRANSMITTANCE_LUT
#include "AtmosphericScattering.hlsli"

static const uint MultipleScatteringSamples = 20;
static const uint SqrtSamples = 8;

float3 GetSphericalDir(float theta, float phi) 
{
    float cosPhi = cos(phi);
    float sinPhi = sin(phi);
    float cosTheta = cos(theta);
    float sinTheta = sin(theta);
    return float3(sinPhi * sinTheta, cosPhi, sinPhi * cosTheta);
}

cbuffer TextureSize : register(b12)
{
    uint2 textureSize;
    float3 groundAlbedo;
}

RWTexture2D<float4> MultipleScatteringLUT;

void GetMultipleScattering(in float3 worldPos, in float3 sunDir, inout float3 lumTotal, inout float3 fms)
{
    static const float invSampleCount = 1.0f / float(SqrtSamples * SqrtSamples);
    for (uint i = 0; i < SqrtSamples; ++i)
    {
        for (uint j = 0; j < SqrtSamples; ++j)
        {
            // The final integral we're approximating here is symetrical so we only need to compute values from 0 to PI instead of 0 to 2*PI.
            float theta = 2 * PI * (float(i) + 0.5f) / float(SqrtSamples);
            float phi = acos(clamp(1.0f - 2.0f * (float(j) + 0.5f) / float(SqrtSamples), -1, 1));
            float3 rayDir = GetSphericalDir(theta, phi);

            float atmosphereDist = RaySphereIntersect(worldPos, rayDir, AtmosphereRadius);

            float planetDist = RaySphereIntersect(worldPos, rayDir, PlanetRadius);

            float rayMarchLength = atmosphereDist;
            if (planetDist > 0)
            {
                rayMarchLength = planetDist;
            }

            float cosTheta = dot(rayDir, sunDir);
            float rayleighPhaseValue = GetRayleighPhase(-cosTheta);
            float miePhaseValue = GetMiePhase(cosTheta);

            float3 lum = 0, lumFactor = 0, transmittance = 1;
            
            float3 samplePos = worldPos;
            float stepSize = rayMarchLength / (MultipleScatteringSamples - 1);

            for (uint k = 0; k < MultipleScatteringSamples; ++k)
            {
                // World unit is in meters, altitude is in m.
                // In this LUT, we suppose worldPos is in m to avoid converting.
                float distToPlanet = length(samplePos);
                float altitude = distToPlanet - PlanetRadius;

                float3 rayleighScattering, extinction;
                float mieScattering;
                GetScatteringValues(altitude, rayleighScattering, mieScattering, extinction);

                float3 sampleTransmittance = exp(-stepSize * extinction);

                float3 scatteringNoPhase = rayleighScattering + mieScattering;
                float3 scatteringF = (scatteringNoPhase - scatteringNoPhase * sampleTransmittance) / extinction;

                float3 sunTransmittance = GetValueFromLUT(TransmittanceLUT, LinearSampler, samplePos, 0, sunDir, distToPlanet);

                float3 rayleighInScattering = rayleighScattering * rayleighPhaseValue;
                float mieInScattering = mieScattering * miePhaseValue;
                float3 inScattering = (rayleighInScattering + mieInScattering) * sunTransmittance;

                // Integrated scattering within path segment.
                float3 scatteringIntegral = (inScattering - inScattering * sampleTransmittance) / extinction;

                lum += scatteringIntegral * transmittance;
                lumFactor += transmittance * scatteringF;
                transmittance *= sampleTransmittance;

                samplePos += rayDir * stepSize;
            }

            if (planetDist > 0)
            {
                float3 hitPos = worldPos + planetDist * rayDir;
                if (dot(worldPos, sunDir) > 0)
                {
                    hitPos = normalize(hitPos) * PlanetRadius;
                    float distToHitPos = length(hitPos);
                    lum += transmittance * groundAlbedo * GetValueFromLUT(TransmittanceLUT, LinearSampler, hitPos, 0, sunDir, distToHitPos);
                }
            }

            fms += lumFactor * invSampleCount;
            lumTotal += lum * invSampleCount;
        }
    }
}

[numthreads(8, 8, 1)]
void main(uint2 dispatchThreadID : SV_DispatchThreadID)
{
    float2 uv = (float2(dispatchThreadID) + 0.5f.xx) / textureSize;

    float sunCosTheta = 2 * uv.x - 1;
    float sunTheta = acos(clamp(sunCosTheta, -1.0, 1.0));
    float height = lerp(PlanetRadius, AtmosphereRadius, uv.y);

    float3 worldPos = float3(0, height, 0);
    float3 sunDir = normalize(float3(0, sunCosTheta, -sin(sunTheta)));

    float3 lum = 0, fms = 0;
    GetMultipleScattering(worldPos, sunDir, lum, fms);

    float3 psi = lum / (1.0f - fms);
    MultipleScatteringLUT[dispatchThreadID] = float4(psi, 1);
}