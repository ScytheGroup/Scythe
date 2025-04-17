#include "Common.hlsli"
#include "Math.hlsli"
#define DECLARE_TRANSMITTANCE_LUT
#define DECLARE_MS_LUT
#include "AtmosphericScattering.hlsli"

Texture2D<float4> SkyLUT;
Texture2D<float> Depth;
RWTexture2D<float4> SceneColor;

float3 SampleFromSkyLUT(float3 rayDir, float3 sunDir, float3 viewPos)
{
    float height = distance(viewPos, PlanetCenter);
    float3 up = (viewPos - PlanetCenter) / height;

    float horizonAngle = acos(clamp(sqrt(height * height - PlanetRadius * PlanetRadius) / height, -1, 1));
    float altitudeAngle = horizonAngle - acos(dot(rayDir, up)); // Between -PI/2 and PI/2

    float3 forward = normalize(cross(up, sunDir));
    float3 right = normalize(cross(forward, up));

    float3 projectedDir = normalize(rayDir - up * dot(rayDir, up));
    float sinTheta = dot(projectedDir, right);
    float cosTheta = dot(projectedDir, forward);
    float azimuthAngle = atan2(sinTheta, cosTheta) + PI; // Between 0 and 2*PI

    // Non-linear mapping of altitude angle. See Section 5.3 of the paper.
    float v = 0.5 + 0.5 * sign(altitudeAngle) * sqrt(abs(altitudeAngle) * 2.0 / PI);
    float2 uv = float2(azimuthAngle / (2.0f * PI), v);

    return SkyLUT.SampleLevel(AnisotropicSampler, uv, 0).rgb;
}

[numthreads(8, 8, 1)]
void main(uint2 dispatchThreadID : SV_DispatchThreadID)
{
    float2 uv = float2(dispatchThreadID) / frameData.screenSize;
    float depth = Depth[dispatchThreadID];
    float3 worldPos = WorldPosFromDepth(depth, uv, frameData.invViewProjection);
    float3 rayDir = normalize(worldPos - frameData.cameraPosition);
    float3 sunDir = normalize(-frameData.dirLight.direction);
    // Force the player to be positioned at x=0 and z=0 since we don't allow for the camera to rotate around the x and z axis
    // We keep the height for the behavior of scattering to be consistent and to depend on the player's height.
    // This means outer-planet travel is no longer possible :-(
    float3 viewPos = float3(0, max(frameData.cameraPosition.y / 1000, 0.001f), 0);

    float3 light = 0;
    
    bool isInsideAtmosphere = distance(viewPos, PlanetCenter) < AtmosphereRadius + 2;

    // Only apply sun if nothing occludes it
    if (depth == 1)
    {
        // Bloom should be added at the end, but this is subtle and works well.
        // Make it more intense to have a bigger impact during tonemapping.
        float3 sunLum = frameData.dirLight.intensity * SunWithBloom(rayDir, sunDir) * frameData.dirLight.color;
        // Use smoothstep to limit the effect, so it drops off to actual zero.
        sunLum = smoothstep(0.002, 1.0, sunLum);
        if (any(sunLum > 0.0))
        {
            // Hide sun if obscured by planet (not in depth buffer)
            if (RaySphereIntersect(viewPos - PlanetCenter, rayDir, PlanetRadius) >= 0.0f)
            {
                sunLum *= 0.0;
            }
            else if (isInsideAtmosphere)
            {
                // If the sun value is applied to this pixel, we need to calculate the transmittance to obscure it.
                float distToPlanet = distance(viewPos, PlanetCenter);
                sunLum *= GetValueFromLUT(TransmittanceLUT, LinearSampler, viewPos, PlanetCenter, sunDir, distToPlanet);
            }
        }

        light += sunLum;
    }
    
    // If inside planet or the sky is hidden, don't add atmosphere
    if (distance(viewPos, PlanetCenter) < PlanetRadius - 0.1 || (depth < 1.0f && depth >= 0.0f))
    {
        return;
    }

    float linearDepth = distance(worldPos, viewPos / 1000);

#if USE_SKY_VIEW_LUT == 1
    // Sample SkyLUT, this mode is better in terms of perf because we generate the LUT at a lower resolution.
    light += SampleFromSkyLUT(rayDir, sunDir, viewPos);
#else

    // Raymarch directly, works better outside of the atmosphere but is WAY more costly.
    light += RaymarchScattering(viewPos, rayDir, sunDir, depth, linearDepth);
#endif

    // Make the output brighter arbitrarily to make the sky appear brighter and get clamped less by tonemapping
    SceneColor[dispatchThreadID] += float4(light * AtmosphereIntensity, 1);
}