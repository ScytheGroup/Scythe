#ifndef __SHADOWS_HLSL__
#define __SHADOWS_HLSL__

#include "Samplers.hlsli"
#include "Common.hlsli"

static const int PCFKernelSize = 4;
static const int PCFHalfKernelSize = PCFKernelSize / 2;

struct ShadowComputeData 
{
    float3 worldPosition;
    float3 normal;
    float depth;
};

// Use "PCF" define to enable soft shadows

// This needs to match CASCADE_COUNT in DirectionalShadows.h
static const uint SHADOWMAP_CASCADE_COUNT = 3;

struct CascadeShadow {
    matrix transformMatrixes;
    float2 cascadeBoundaries;
    float bias;
    float _pad; // Force padding on 16 byte frontier for array packing
};

cbuffer ShadowCascades
{
    CascadeShadow cascades[SHADOWMAP_CASCADE_COUNT];
}

StructuredBuffer<matrix> SpotLightMatrices;

cbuffer DirectionalProjectedData 
{
    matrix directionalProjectedViewProjMatrix;
}

matrix GetShadowMatrix(uint cascadeId)
{
    return cascades[cascadeId].transformMatrixes;
}

float GetCascadeBias(uint cascadeId)
{
    return cascades[cascadeId].bias;
}


float GetCascadeLowerBound(CascadeShadow shadow)
{
    return shadow.cascadeBoundaries.x;
}

float GetCascadeUpperBound(CascadeShadow shadow)
{
    return shadow.cascadeBoundaries.y;
}

int GetCascadeIndex(float distanceFromCamera, int startOffset = 0)
{
    if (startOffset < 0)
        return -1;
    
    for (uint i = startOffset; i < SHADOWMAP_CASCADE_COUNT; ++i) 
    {
        if (distanceFromCamera > cascades[i].cascadeBoundaries.x && 
            distanceFromCamera < cascades[i].cascadeBoundaries.y)
        {
            return i;
        }
    }
    
    return -1;
}

float CalculateShadowFactor(float4 posFromLight, float bias, Texture2D shadowMap)
{
    float3 projCoords = posFromLight.xyz / posFromLight.w;
    const float currentDepth = projCoords.z;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.x < 0 || projCoords.y < 0 || projCoords.x > 1 || projCoords.y > 1)
        return 0;
    
    float2 sampleCoord = float2(projCoords.x, 1 - projCoords.y);

#if PCF == 1
    float total = 0;
    
    [unroll] for (int x = -PCFHalfKernelSize; x < -PCFHalfKernelSize + PCFKernelSize; x++)
    {
        [unroll] for (int y = -PCFHalfKernelSize; y < -PCFHalfKernelSize + PCFKernelSize; y++)
        {
            total += shadowMap.SampleCmpLevelZero(ShadowMapSampler, sampleCoord, currentDepth - bias, int2(x, y));
        }
    }
    
    return total / pow(PCFKernelSize, 2.0f);
#endif
    
    return shadowMap.SampleCmpLevelZero(ShadowMapSampler, sampleCoord, currentDepth - bias);
}

float CalculateShadowFactorArray(int arrayIndex, float4 posFromLight, float bias, Texture2DArray shadowMap)
{
    float3 projCoords = posFromLight.xyz / posFromLight.w;
    const float currentDepth = projCoords.z;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.x < 0 || projCoords.y < 0 || projCoords.x > 1 || projCoords.y > 1)
        return 0;
    
    float2 sampleCoord = float2(projCoords.x, 1 - projCoords.y);

#if PCF == 1
    float total = 0;

    [unroll] for (int x = -PCFHalfKernelSize; x < -PCFHalfKernelSize + PCFKernelSize; x++)
    {
        [unroll] for (int y = -PCFHalfKernelSize; y < -PCFHalfKernelSize + PCFKernelSize; y++)
        {
            total += shadowMap.SampleCmpLevelZero(ShadowMapSampler, float3(sampleCoord, arrayIndex), currentDepth - bias, int2(x, y));
        }
    }
    
    return total / pow(PCFKernelSize, 2.0f);
#endif
    
    return shadowMap.SampleCmpLevelZero(ShadowMapSampler, float3(sampleCoord, arrayIndex), currentDepth - bias);
}

float ComputeShadowForCascade(int lightIndex, ShadowComputeData computeData, Texture2DArray shadowMap, int cascadeId)
{
    const matrix cascadeMatrix = GetShadowMatrix(cascadeId);
    const float4 posFromLight = mul(cascadeMatrix, float4(computeData.worldPosition + computeData.normal * ((cascadeId + 1) * 0.2), 1));
    return CalculateShadowFactorArray(lightIndex * SHADOWMAP_CASCADE_COUNT + cascadeId, posFromLight, GetCascadeBias(cascadeId), shadowMap);
}

float ComputeSpotShadow(int lightIndex, ShadowComputeData computeData, Texture2DArray shadowMap)
{
    const matrix cascadeMatrix = SpotLightMatrices[lightIndex];
    const float4 posFromLight = mul(cascadeMatrix, float4(computeData.worldPosition + computeData.normal * 0.1, 1));
    return CalculateShadowFactorArray(lightIndex, posFromLight, 0.0001, shadowMap);
}

float ComputeDirectionalProjectedShadow(ShadowComputeData computeData, Texture2D shadowMap)
{
    const float4 posFromLight = mul(directionalProjectedViewProjMatrix, float4(computeData.worldPosition + computeData.normal * 0.1, 1));
    return CalculateShadowFactor(posFromLight, 0.0001, shadowMap);
}

float ComputePointShadow(int lightIndex, float3 lightPos, float farPlane, ShadowComputeData computeData, TextureCubeArray shadowMap)
{
    float3 worldPos = computeData.worldPosition + computeData.normal * 0.1;
    float3 fragToLight = worldPos - lightPos;
    fragToLight.y *= -1; 
    float worldDepth = length(fragToLight);

#if PCF
    float shadow  = 0.0;
    float bias    = 0.05; 
    float samples = 4.0;
    float offset  = 0.1;
    for (float x = -offset; x < offset; x += offset / (samples * 0.5))
    {
        for (float y = -offset; y < offset; y += offset / (samples * 0.5))
        {
            for (float z = -offset; z < offset; z += offset / (samples * 0.5))
            {
                float closestDepth = shadowMap.Sample(PointSampler, float4(fragToLight + float3(x, y, z), lightIndex)).r;
                closestDepth *= farPlane;
                if (worldDepth - bias > closestDepth)
                    shadow += 1.0;
            }
        }
    }
    return 1.0f - (shadow /= (samples * samples * samples));
#else
    float closestDepth = shadowMap.Sample(PointSampler, float4(fragToLight, lightIndex)).r;
    
    // inverse falloff equation
    closestDepth *= farPlane;
    
    float bias = 0.05; 
    return worldDepth - bias > closestDepth ? 0.0 : 1.0;
#endif
}

float3 CalculateDirectionalCascadedShadows(uint lightIndex, ShadowComputeData computeData, Texture2DArray shadowMap)
{
    const float distanceFromCam = computeData.depth;
            
    const int cascadeId = GetCascadeIndex(distanceFromCam);
    if (cascadeId == -1)
        return 1;
    
    float shadow = ComputeShadowForCascade(lightIndex, computeData, shadowMap, cascadeId);
            
    float3 _shadow = shadow;
#ifdef DEBUG_SHADOWS
    // Shows different colors for cascades
    if (cascadeId == 0)
        _shadow *= float3(1, 0, 0);
    if (cascadeId == 1)
        _shadow *= float3(1, 1, 0);
    if (cascadeId == 2)
        _shadow *= float3(0, 1, 0);
    if (cascadeId == 3)
        _shadow *= float3(0, 1, 1);
    if (cascadeId == 4)
        _shadow *= float3(0, 0, 1);
    if (cascadeId == 5)
        _shadow *= float3(1, 0, 1);
#endif

    const int cascadeId2 = GetCascadeIndex(distanceFromCam, cascadeId + 1);
            
    // If the position is also in the next cascade, calculate that shadow value and blend
    if (cascadeId2 != -1)
    {
        // Fetch the lower bound of the next cascade and the upper bound of the previous
        float upperBound = GetCascadeUpperBound(cascades[cascadeId]);
        float lowerBound = GetCascadeLowerBound(cascades[cascadeId2]);
        
        float blendShadow = ComputeShadowForCascade(lightIndex, computeData, shadowMap, cascadeId2);
        
        float3 _blendShadow = blendShadow;
#ifdef DEBUG_SHADOWS
        // Shows different colors for cascades
        if (cascadeId2 == 0)
            _blendShadow *= float3(1, 0, 0);
        if (cascadeId2 == 1)
            _blendShadow *= float3(1, 1, 0);
        if (cascadeId2 == 2)
            _blendShadow *= float3(0, 1, 0);
        if (cascadeId2 == 3)
            _blendShadow *= float3(0, 1, 1);
        if (cascadeId2 == 4)
            _blendShadow *= float3(0, 0, 1);
        if (cascadeId == 5)
            _blendShadow *= float3(1, 0, 1);
#endif
        // This calculates the blend ratio
        _shadow = lerp(_shadow, _blendShadow, (distanceFromCam - lowerBound) / (upperBound - lowerBound));
    }
    
    return _shadow;
}

#endif