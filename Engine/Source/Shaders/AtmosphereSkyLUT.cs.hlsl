#include "Math.hlsli"
#define DECLARE_TRANSMITTANCE_LUT
#define DECLARE_MS_LUT
#include "AtmosphericScattering.hlsli"

cbuffer SkyLUTParams : register(b6)
{
    float3 cameraPosition;
    uint textureSizeX;
    float3 sunDir;
    uint textureSizeY;
}

RWTexture2D<float4> SkyLUT;

[numthreads(8, 8, 1)]
void main(uint2 dispatchThreadID : SV_DispatchThreadID)
{
    float2 textureSize = float2(textureSizeX, textureSizeY);
    float2 uv = (float2(dispatchThreadID) + 0.5f.xx) / textureSize;

    float azimuthAngle = uv.x * 2.0f * PI;

    float v = uv.y;
    float altitudeAngle;
    if (v < 0.5f)
    {
        // For negative altitude angles
        float scaledV = (0.5f - v) * 2.0f;
        altitudeAngle = -0.5f * PI * scaledV * scaledV;
    }
    else
    {
        // For positive altitude angles
        float scaledV = (v - 0.5f) * 2.0f;
        altitudeAngle = 0.5f * PI * scaledV * scaledV;
    }
    
    float3 viewPos = float3(0, max(cameraPosition.y / 1000, 0.001f), 0);

    float height = distance(viewPos, PlanetCenter);
    float3 up = (viewPos - PlanetCenter) / height;

    float horizonAngle = acos(clamp(sqrt(height * height - PlanetRadius * PlanetRadius) / height, -1, 1));

    float finalAltitudeAngle = altitudeAngle;
    
    float3 forward = normalize(cross(up, sunDir));
    float3 right = normalize(cross(forward, up));
    
    float cosAltitude = cos(finalAltitudeAngle);
    float sinAltitude = sin(finalAltitudeAngle);
    float cosAzimuth = cos(azimuthAngle);
    float sinAzimuth = sin(azimuthAngle);
    
    float3 rayDir = up * sinAltitude + right * (cosAltitude * sinAzimuth) + forward * (cosAltitude * cosAzimuth);
    
    float3 sDir = normalize(-sunDir);
    float3 lum = RaymarchScattering(viewPos, rayDir, sDir);

    SkyLUT[dispatchThreadID] = float4(lum, 1.0f);
}