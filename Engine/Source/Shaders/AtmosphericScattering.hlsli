#ifndef _ATMOSPHERIC_SCATTERING_
#define _ATMOSPHERIC_SCATTERING_

#include "Constants.hlsli"
#include "Samplers.hlsli"

static const float3 ScatteringWavelengths = float3(680, 550, 440);

static const float DistanceScale = 1e-3;

static const float3 RayleighScatteringCoeff = float3(6.605f, 12.344f, 29.412f) * DistanceScale;
static const float3 RayleighExtinctionCoeff = 0;

static const float MieScatteringCoeff = 3.996f * DistanceScale;
static const float MieExtinctionCoeff = 4.4f * DistanceScale;

static const float OzoneScatteringCoeff = 0;
static const float3 OzoneExtinctionCoeff = float3(3.426f, 8.298f, 0.356f) * 0.06f * pow(10, -5);

cbuffer AtmosphereSettings : register(b13)
{
    float PlanetRadius;
    float AtmosphereHeight;
    float DensityScaleHeightRayleigh;
    float DensityScaleHeightMie;
    float3 PlanetCenter;
    float AtmosphereIntensity;
}

#define AtmosphereRadius (PlanetRadius + AtmosphereHeight)

float GetRayleighPhase(float cosTheta) {
    const float k = 3.0 / (16.0 * PI);
    return k * (1.0 + cosTheta * cosTheta);
}

float GetMiePhase(float cosTheta) {
    const float g = 0.8;
    const float scale = 3.0 / (8.0 * PI);
    
    float num = (1.0 - g * g) * (1.0 + cosTheta * cosTheta);
    float denom = (2.0 + g * g) * pow(abs(1.0 + g * g - 2.0 * g * cosTheta), 1.5);
    
    return scale * num / denom;
}

// Supposes the sphere is positioned at (0, 0, 0)
float RaySphereIntersect(float3 rayOrigin, float3 rayDir, float sphereRadius)
{
    float b = dot(rayOrigin, rayDir);
    float c = dot(rayOrigin, rayOrigin) - sphereRadius * sphereRadius;
    if (c > 0.0f && b > 0.0f) return -1.0f;
    float discr = b * b - c;
    if (discr < 0.0f) return -1.0f;
    // Special case: inside sphere, use far discriminant
    if (discr > b * b) return (-b + sqrt(discr));
    return -b - sqrt(discr);
}

// Returns the intersection point of a ray sphere intersection
float2 RaySphereIntersect2D(float3 rayOrigin, float3 rayDir, float3 sphereCenter, float sphereRadius)
{
    rayOrigin -= sphereCenter;
    float b = dot(rayOrigin, rayDir);

    float3 qc = rayOrigin - b * rayDir;
    float d = sphereRadius * sphereRadius - dot(qc, qc);
    if (d < 0.0f) return -1;
    d = sqrt(d);
    float x1 = (-b - d);
    float x2 = (-b + d);
    return float2(x1, x2);
}

float3 GetValueFromLUT(Texture2D<float4> LUT, SamplerState samplerLUT, float3 worldPos, float3 planetCenter, float3 sunDir, float distToPlanet)
{
    float3 up = (worldPos - planetCenter) / distToPlanet;
    float sunCosZenithAngle = dot(sunDir, up);
    float2 uv = float2(
        saturate(0.5f + 0.5f * sunCosZenithAngle),
        saturate((distToPlanet - PlanetRadius) / AtmosphereHeight)
    );
    return LUT.SampleLevel(samplerLUT, uv, 0).rgb;
}

float3 SunWithBloom(float3 rayDir, float3 sunDir) 
{
    const float sunSolidAngle = 0.53 * PI / 180.0;
    const float minSunCosTheta = cos(sunSolidAngle);

    float cosTheta = dot(rayDir, sunDir);
    if (cosTheta > minSunCosTheta) return 1.0f;
    
    float offset = minSunCosTheta - cosTheta;
    float gaussianBloom = exp(-offset * 50000.0) * 0.5f;
    float invBloom = 1.0f / (0.02f + offset * 300.0) * 0.01f;
    return (float3) gaussianBloom + invBloom;
}

// Precondition: surfaceAltitude is in km
float2 GetAtmosphereDensityRatio(float surfaceAltitude)
{
    // Use Rayleigh and Mie density scales
    return float2(
        exp(-surfaceAltitude / DensityScaleHeightRayleigh),
        exp(-surfaceAltitude / DensityScaleHeightMie)
    );
}

// Precondition: surfaceAltitude is in km
void GetScatteringValues(float surfaceAltitude, out float3 rayleighScattering, out float mieScattering, out float3 extinction)
{
    float2 rayleighMieDensities = GetAtmosphereDensityRatio(surfaceAltitude);

    rayleighScattering = rayleighMieDensities.x * RayleighScatteringCoeff;
    float3 rayleighExtinction = rayleighMieDensities.x * RayleighExtinctionCoeff;

    mieScattering = rayleighMieDensities.y * MieScatteringCoeff;
    float3 mieExtinction = rayleighMieDensities.y * MieExtinctionCoeff;

    float3 ozoneExtinction = OzoneExtinctionCoeff * max(0.0, 1.0 - abs(surfaceAltitude - 25.0) / 15.0);

    extinction = rayleighScattering + rayleighExtinction + mieScattering + mieExtinction + ozoneExtinction;
}

#ifdef DECLARE_TRANSMITTANCE_LUT
Texture2D<float4> TransmittanceLUT;

#ifdef DECLARE_MS_LUT
Texture2D<float4> MultipleScatteringLUT;

static const uint NumScatteringSteps = 16;

float3 RaymarchScattering(float3 viewPos, float3 rayDir, float3 sunDir, float depth, float linearDepth)
{
    // Get the intersection points with the outer atmosphere
    float2 atmosphereIntersections = RaySphereIntersect2D(viewPos, rayDir, PlanetCenter, AtmosphereRadius);

    // Get the intersection with the planet
    float planetIntersection = RaySphereIntersect(viewPos - PlanetCenter, rayDir, PlanetRadius);

    float distToStart = max(0, atmosphereIntersections.x);
    float distToEnd = 0;

    // No intersection with planet
    if (planetIntersection < 0)
    {
        // No intersection with either atmosphere either means there is nothing to do.
        if (atmosphereIntersections.y < 0)
        {
            return 0;
        }
        else
        {
            // Set the end point of raymarching to the outer edge of the atmosphere
            distToEnd = atmosphereIntersections.y;
        }
    }
    else
    {
        // Should be impossible to intersect planet and not intersect atmosphere (the planet is inside the atmosphere)
        if (atmosphereIntersections.y > 0)
        {
            distToEnd = min(atmosphereIntersections.y, planetIntersection);
        }
    }

    if (depth < 1.0f && depth >= 0.0f)
    {
        // Stop early if depth is blocking
        distToEnd = min(linearDepth, distToEnd);
    }

    // If we are supposed to end before the start (depth stops outside of atmosphere), we can return.
    if (distToEnd < distToStart)
    {
        return 0;
    }

    float cosTheta = dot(rayDir, sunDir);
    float rayleighPhaseValue = GetRayleighPhase(-cosTheta);
    float miePhaseValue = GetMiePhase(cosTheta);

    // Constant amount of samples along the ray
    float rayLength = distToEnd - distToStart;
    float stepSize = rayLength / (NumScatteringSteps - 1);

    float3 rayStart = viewPos + rayDir * distToStart;
    float3 samplePos = rayStart;

    float3 luminance = 0;
    float3 transmittance = 1;

    for (uint i = 0; i < NumScatteringSteps; ++i)
    {
        float distToCenter = distance(samplePos, PlanetCenter);
        float surfaceAltitude = distToCenter - PlanetRadius;

        float3 rayleighScattering, extinction;
        float mieScattering;
        GetScatteringValues(surfaceAltitude, rayleighScattering, mieScattering, extinction);

        float3 sampleTransmittance = exp(-stepSize * extinction);

        float3 sunTransmittance = GetValueFromLUT(TransmittanceLUT, LinearSampler, samplePos, PlanetCenter, sunDir, distToCenter);
        float3 psiMS = GetValueFromLUT(MultipleScatteringLUT, LinearSampler, samplePos, PlanetCenter, sunDir, distToCenter);

        float3 rayleighInScattering = rayleighScattering * (rayleighPhaseValue * sunTransmittance + psiMS);
        float3 mieInScattering = mieScattering * (miePhaseValue * sunTransmittance + psiMS);
        float3 inScattering = rayleighInScattering + mieInScattering;

        float3 scatteringIntegral = (inScattering - inScattering * sampleTransmittance) / extinction;
        luminance += scatteringIntegral * transmittance;
        transmittance *= sampleTransmittance;

        samplePos += rayDir * stepSize;
    }

    return luminance;
}

float3 RaymarchScattering(float3 viewPos, float3 rayDir, float3 sunDir)
{
    return RaymarchScattering(viewPos, rayDir, sunDir, -1, -1);
}

#endif // DECLARE_MS_LUT

#endif // DECLARE_TRANSMITTANCE_LUT

#endif // _ATMOSPHERIC_SCATTERING_
