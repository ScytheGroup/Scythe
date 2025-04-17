#ifndef _SHADING_LIT_
#define _SHADING_LIT_

////////////////////////////////////////////////
//             Lit Shading Model
//
// References:
// [1] Google Filament
// [2] https://seblagarde.wordpress.com/wp-content/uploads/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
//
////////////////////////////////////////////////

#include "Common.hlsli"
#include "PBR_BRDF.hlsli"

#if (DIRECTIONAL_SHADOWS + SPOT_SHADOWS + POINT_SHADOWS) > 0
#include "Shadows.hlsli"
#endif

#if DIRECTIONAL_CASCADED_SHADOWS == 1
Texture2DArray DirectionalCascadedShadowMap;
#endif

#if DIRECTIONAL_PROJECTED_SHADOWS == 1
Texture2D DirectionalProjectedShadowMap;
#endif

#if SPOT_SHADOWS == 1
Texture2DArray SpotShadowMaps;
#endif

#if POINT_SHADOWS == 1
TextureCubeArray PointShadowMaps;
struct PointLightData
{
    float3 lightPosition;
    float farPlane;
};
StructuredBuffer<PointLightData> PointLightDataBuffer;
#endif

#ifdef ENABLE_VOXEL_GI
#include "Voxel.hlsli"
Texture3D<float4> VoxelIlluminationVolume;
cbuffer VoxelGIConfig
{
    VoxelGIConfig voxelGIConfig;
}
#endif

#define DIFFUSE_IRRADIANCE_MAP 0
#define DIFFUSE_IRRADIANCE_SH 1

#if DIFFUSE_IRRADIANCE == DIFFUSE_IRRADIANCE_MAP
TextureCube DiffuseIrradianceMap;
#elif DIFFUSE_IRRADIANCE == DIFFUSE_IRRADIANCE_SH

#include "SH.hlsli"

cbuffer DiffuseIrradianceSH9PackedConstantBuffer
{
    float4 diffuseIrradianceSH9Packed[7];
}

// Unpack cbuffer format to usable hlsl structure
ColorSH9 GetDiffuseIrradianceSH9()
{
    ColorSH9 colorSH = ZeroColorSH9();
    
    // Red
    colorSH.r.v[0] = diffuseIrradianceSH9Packed[0].x;
    colorSH.r.v[1] = diffuseIrradianceSH9Packed[0].y;
    colorSH.r.v[2] = diffuseIrradianceSH9Packed[0].z;
    colorSH.r.v[3] = diffuseIrradianceSH9Packed[0].w;
    colorSH.r.v[4] = diffuseIrradianceSH9Packed[1].x;
    colorSH.r.v[5] = diffuseIrradianceSH9Packed[1].y;
    colorSH.r.v[6] = diffuseIrradianceSH9Packed[1].z;
    colorSH.r.v[7] = diffuseIrradianceSH9Packed[1].w;
    colorSH.r.v[8] = diffuseIrradianceSH9Packed[2].x;

    // Green
    colorSH.g.v[0] = diffuseIrradianceSH9Packed[2].y;
    colorSH.g.v[1] = diffuseIrradianceSH9Packed[2].z;
    colorSH.g.v[2] = diffuseIrradianceSH9Packed[2].w;
    colorSH.g.v[3] = diffuseIrradianceSH9Packed[3].x;
    colorSH.g.v[4] = diffuseIrradianceSH9Packed[3].y;
    colorSH.g.v[5] = diffuseIrradianceSH9Packed[3].z;
    colorSH.g.v[6] = diffuseIrradianceSH9Packed[3].w;
    colorSH.g.v[7] = diffuseIrradianceSH9Packed[4].x;
    colorSH.g.v[8] = diffuseIrradianceSH9Packed[4].y;
    
    // Blue
    colorSH.b.v[0] = diffuseIrradianceSH9Packed[4].z;
    colorSH.b.v[1] = diffuseIrradianceSH9Packed[4].w;
    colorSH.b.v[2] = diffuseIrradianceSH9Packed[5].x;
    colorSH.b.v[3] = diffuseIrradianceSH9Packed[5].y;
    colorSH.b.v[4] = diffuseIrradianceSH9Packed[5].z;
    colorSH.b.v[5] = diffuseIrradianceSH9Packed[5].w;
    colorSH.b.v[6] = diffuseIrradianceSH9Packed[6].x;
    colorSH.b.v[7] = diffuseIrradianceSH9Packed[6].y;
    colorSH.b.v[8] = diffuseIrradianceSH9Packed[6].z;

    return colorSH;
}
#endif

TextureCube PrefilteredEnvironmentMap;
Texture2D<float2> BRDFIntegrationLUT;

struct GlobalLighting
{
    Lighting direct;
    Lighting indirect;
};

// From [2], gives us a more accurate reflection. 
// This is based on the fact that the actual fD and fR(/fS) terms are dependant on the view vector while the preintegration is done in a view independant way.
float3 GetSpecularDominantDir(float3 N, float3 R, float roughness)
{
    float smoothness = saturate(1 - roughness);
    float lerpFactor = smoothness * (sqrt(smoothness) + roughness);
    // The result is not normalized as we fetch in a cubemap
    return lerp(N, R, lerpFactor);
}

// IBL related code should be moved into another shader file if we ever get around to implementing local reflection capture probes
// since they would require an accumulation pass which is quite a bit more complex than this code.
Lighting EvaluateGlobalIBL(float3 diffuseColor, float3 specularColor, float3 normal, float3 view, float3 reflection, float NoV, float roughness)
{
    float3 kS = F_SchlickRoughness(NoV, specularColor, roughness);
    float3 kD = float3(1, 1, 1) - kS;
    float3 fd = diffuseColor * fd_Lambert();

    // Diffuse Irradiance
#if DIFFUSE_IRRADIANCE == DIFFUSE_IRRADIANCE_MAP
    float3 irradiance = DiffuseIrradianceMap.Sample(LinearSampler, normal).rgb;
#elif DIFFUSE_IRRADIANCE == DIFFUSE_IRRADIANCE_SH
    ColorSH9 diffuseIrradianceSH9 = GetDiffuseIrradianceSH9();
    float3 irradiance = ReconstructIrradiance(diffuseIrradianceSH9, normal);
#endif
    float3 diffuseIrradiance = fd * irradiance;

    // Specular Reflection
    float2 envBRDF = BRDFIntegrationLUT.Sample(PointSampler, float2(NoV, roughness)).xy;
    static const float MaxReflectionLOD = 4;
    float3 specularDominant = GetSpecularDominantDir(normal, reflection, roughness);
    float3 prefilteredColor = PrefilteredEnvironmentMap.SampleLevel(LinearSampler, specularDominant, roughness * MaxReflectionLOD).rgb;
    float3 specular = prefilteredColor * (kS * envBRDF.x + envBRDF.y);

    Lighting lighting;
    lighting.specular = specular;
    lighting.diffuse = kD * diffuseIrradiance;
    return lighting;
}

#if SUN_TRANSMITTANCE
Texture2D TransmittanceLUT;
// Only exposed for debugging purposes. This can be assumed in shader bcause we don't expose this to game.
float3 PlanetCenter = float3(0, -6371, 0);
float PlanetRadius = 6371;
float AtmosphereHeight = 80;
#endif

Lighting EvaluateDirectionalLight(DirectionalLight directionalLight, float3 diffuseColor, float3 specularColor, float3 worldPosition, float3 view, float3 normal, float NoV, float roughness)
{
    float3 light = normalize(-directionalLight.direction);
    float3 H = normalize(light + view);

    float NoH = max(dot(normal, H), 0);
    float NoL = max(dot(normal, light), 0);
    float VoH = max(dot(view, H), 0);
    float LoH = max(dot(light, H), 0);

    Lighting lightColor = BRDF(diffuseColor, specularColor, directionalLight.color * directionalLight.intensity, NoH, NoV, NoL, VoH, LoH, roughness);

#if (DIRECTIONAL_CASCADED_SHADOWS + DIRECTIONAL_PROJECTED_SHADOWS) > 0
    ShadowComputeData shadowData;
    shadowData.worldPosition = worldPosition;
    shadowData.normal = normal;
    // Inverse because of right handed view
    shadowData.depth = GetViewDepth(frameData.view, worldPosition);

#if DIRECTIONAL_CASCADED_SHADOWS == 1
    {
        float3 shadowBlend = CalculateDirectionalCascadedShadows(0, shadowData, DirectionalCascadedShadowMap);
        lightColor.diffuse *= shadowBlend;
        lightColor.specular *= shadowBlend;    
    }
#endif

#if DIRECTIONAL_PROJECTED_SHADOWS == 1
    {
        float3 shadowBlend = ComputeDirectionalProjectedShadow(shadowData, DirectionalProjectedShadowMap);
        lightColor.diffuse *= shadowBlend;
        lightColor.specular *= shadowBlend;
    }
#endif
    
#endif

#if SUN_TRANSMITTANCE
    float3 planetSpaceWorldPosition = float3(0, worldPosition.y / 1000, 0);
    float distToPlanet = distance(planetSpaceWorldPosition, PlanetCenter);
    float3 up = (planetSpaceWorldPosition - PlanetCenter) / distToPlanet;
    float sunCosZenithAngle = dot(light, up);
    float2 uv = float2(
        saturate(0.5f + 0.5f * sunCosZenithAngle),
        saturate((distToPlanet - PlanetRadius) / AtmosphereHeight)
    );
    lightColor.specular *= TransmittanceLUT.SampleLevel(PointSampler, uv, 0).rgb;
#endif

    return lightColor;
}

float GetDistanceAttenuation(float3 light, float falloffSq)
{
    float distanceSq = dot(light, light);

    if (falloffSq <= 0)
    {
        float distance = length(light);
        // Inverse-squared-falloff using:
        // http://www.cemyuksel.com/research/pointlightattenuation/
        return 2.0 / max(distanceSq + falloffSq + distance * sqrt(distanceSq + falloffSq), 0.01f * 0.01f);
    }
    else
    {
        // Normal quadratic range-based attenuation (Khronos punctual lights extension)
        // https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_lights_punctual/README.md
        return max(min(1.0f - distanceSq * distanceSq * falloffSq * falloffSq, 1), 0) / distanceSq;
    }
}

float GetAngleAttentuation(float3 lightVector, float3 lightDir, float2 angleScaleOffset)
{
    float lightAngleScale = angleScaleOffset.x;
    float lightAngleOffset = angleScaleOffset.y;
    float cd = dot(lightDir, lightVector);
    float attenuation = saturate(cd * lightAngleScale + lightAngleOffset);
    // smooth the transition
    attenuation *= attenuation;
    return attenuation;
}

Lighting EvaluatePunctualLights(float3 diffuseColor, float3 specularColor, float3 view, float3 normal, float3 worldPosition, float NoV, float roughness)
{
    Lighting pointLightsContribution = (Lighting)0;
#if (SPOT_SHADOWS + POINT_SHADOWS) > 0
    ShadowComputeData shadowCompute;
    shadowCompute.worldPosition = worldPosition;
    shadowCompute.normal = normal;
    // Inverse because of right handed view
    shadowCompute.depth = GetViewDepth(frameData.view, worldPosition);
#endif
    uint spotLightIndex = 0;
    uint pointLightIndex = 0;

    for (uint l = 0; l < frameData.punctualLightsCount; ++l)
    {
        PunctualLightData light = PunctualLights[l];
        
        float3 lightPixelDistance = light.position - worldPosition;
        float3 lightVector = normalize(lightPixelDistance);
        float3 H = normalize(lightVector + view);

        float NoH = max(dot(normal, H), 0);
        float NoL = max(dot(normal, lightVector), 0);
        float VoH = max(dot(view, H), 0);
        float LoH = max(dot(lightVector, H), 0);

        uint lightType = GetLightType(light);

        float attenuation = GetDistanceAttenuation(lightPixelDistance, light.falloff);

        if (lightType == SPOT_LIGHT_TYPE)
        {
            attenuation *= GetAngleAttentuation(lightVector, normalize(light.direction), light.angleScaleOffset);
            spotLightIndex++;
        }
        else
        {
            pointLightIndex++;
        }

        if (NoL <= 0.0 || attenuation <= 0.0) 
        {
            continue;
        }

        Lighting color = BRDF(diffuseColor, specularColor, light.color * light.intensity * attenuation, NoH, NoV, NoL, VoH, LoH, roughness);

#if SPOT_SHADOWS == 1
        if (lightType == SPOT_LIGHT_TYPE)
        {
            float3 shadowFactor = ComputeSpotShadow(spotLightIndex - 1, shadowCompute, SpotShadowMaps);
            color.diffuse *= shadowFactor;
            color.specular *= shadowFactor;
        }
#endif

#if POINT_SHADOWS == 1
        if (lightType == POINT_LIGHT_TYPE)
        {
            PointLightData pld = PointLightDataBuffer[pointLightIndex - 1];
            float3 shadowFactor = ComputePointShadow(pointLightIndex - 1, pld.lightPosition, pld.farPlane, shadowCompute, PointShadowMaps);
            color.diffuse *= shadowFactor;
            color.specular *= shadowFactor;
        }
#endif

        pointLightsContribution.specular += color.specular;
        pointLightsContribution.diffuse += color.diffuse;
    }

    return pointLightsContribution;
}

Lighting EvaluateLights(float4 diffuseColor, float3 view, float3 normal, float3 worldPosition, float roughness, float metallic) 
{
    // NoV doesn't change per light so we can reuse!
    // We clamp to avoid artifacts when it approaches 0 or 1
    float NoV = clamp(dot(normal, view), SMALL_FLOAT_EPSILON, 1 - SMALL_FLOAT_EPSILON);
    float3 reflectionVector = reflect(-view, normal);

    // Take into account the metallic directly here to avoid recomputing in other functions
    float3 metallicAdjustedDiffuse = diffuseColor.rgb * (1.f - metallic);
    // f0 is always 0.04 for dielectric surfaces
    static const float3 f0 = 0.04;
    // In the case of metallics (conductors) we lerp f0 towards the diffuse using the metallic property:
    // Source: https://learnopengl.com/PBR/Lighting#:~:text=The%20Fresnel%2DSchlick%20approximation%20expects,find%20in%20large%20material%20databases.
    float3 metallicAdjustedSpecular = lerp(f0, diffuseColor.rgb, metallic);

    Lighting dirLighting = EvaluateDirectionalLight(frameData.dirLight, metallicAdjustedDiffuse, metallicAdjustedSpecular, worldPosition, view, normal, NoV, roughness);
    Lighting punctualLighting = EvaluatePunctualLights(metallicAdjustedDiffuse, metallicAdjustedSpecular, view, normal, worldPosition, NoV, roughness);

    Lighting totalLighting = (Lighting)0;
    totalLighting.diffuse = dirLighting.diffuse + punctualLighting.diffuse;
    totalLighting.specular = dirLighting.specular + punctualLighting.specular;
#if ENABLE_IBL == 1
    Lighting iblLighting = EvaluateGlobalIBL(metallicAdjustedDiffuse, metallicAdjustedSpecular, normal, view, reflectionVector, NoV, roughness);
    totalLighting.diffuse += iblLighting.diffuse;
    totalLighting.specular += iblLighting.specular;
#endif
    return totalLighting;
}

float3 EvaluateMaterialLit(float4 diffuseColor, float3 arm, float3 worldPosition, float3 normal)
{
    float3 view = normalize(frameData.cameraPosition - worldPosition);

    float ao = arm.r;
    // Our BRDF makes spec highlight disappear at values lower than 0.05, this is a hack to keep it.
    float roughness = max(arm.g, 0.05f);
    // TODO: add metallic version of brdf to avoid the cost of lerping Kd by metallic in BRDF when the material is metallic 0 (separate define to avoid the cost)
    float metallic = arm.b;
    
    Lighting lightContribution = EvaluateLights(diffuseColor, view, normal, worldPosition, roughness, metallic);
    float3 finalPixelColor = lightContribution.diffuse + lightContribution.specular;

#ifdef ENABLE_VOXEL_GI
    float3 diffuseGlobalContribution = ConeTraceInVoxelIlluminationVolume(VoxelIlluminationVolume, LinearSampler, worldPosition, normal).rgb;
    finalPixelColor = diffuseColor.rgb * diffuseGlobalContribution * voxelGIConfig.IndirectBoost + finalPixelColor;
#endif
    finalPixelColor *= ao;

    static const float3 emissivityColor = 0;
    finalPixelColor += emissivityColor;

    return finalPixelColor;
}

#endif // _SHADING_LIT_