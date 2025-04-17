#ifndef _COMMON_
#define _COMMON_

#include "SH.hlsli"

struct DirectionalLight
{
    float3 color;
    float intensity;
    float3 direction;
};

struct FrameData
{
    matrix view;
    matrix projection;
    matrix viewProjection;
    matrix invView;
    matrix invProjection;
    matrix invViewProjection;
    float3 cameraPosition;
    float2 screenSize;
    float frameTime;
    float elapsedTime;
    uint punctualLightsCount; // Number of punctual lights
    DirectionalLight dirLight;
};

cbuffer FrameCBuffer : register(b0)
{
    FrameData frameData;
}

struct ObjectData
{
    matrix model;
    matrix normalMatrix;
};

cbuffer ObjectCBuffer : register(b1)
{
    ObjectData objectData;
}

#define SHADING_MODEL_LIT 0
#define SHADING_MODEL_UNLIT 1

#ifndef SHADING_MODEL
#define SHADING_MODEL SHADING_MODEL_UNLIT
#endif // SHADING_MODEL

struct MaterialData
{
    float4 baseColor;
#if SHADING_MODEL == SHADING_MODEL_LIT
    float roughness;
    float metallic;
    float emissive;
// This is 0.04 for dielectric surfaces so we can avoid passing it in
//    float baseReflectance;
#endif
};

cbuffer MaterialCBuffer : register(b2)
{
    MaterialData materialData;
}

static const uint POINT_LIGHT_TYPE = 0;
static const uint SPOT_LIGHT_TYPE = 1;

struct PunctualLightData
{
    uint type; // 0: point, 1: spot, only occupies 1 byte
    // byte 2 3 and 4 are unused
    float3 position;
    float3 color;
    float intensity;
    float falloff; // = 1/(attenuation_radius²)
    float3 direction;
    float2 angleScaleOffset;
};

// StructuredBuffer allows for variable amount of lights
StructuredBuffer<PunctualLightData> PunctualLights : register(t98);

// Extract uint8 from first byte of light type field
uint GetLightType(PunctualLightData light) 
{
    return light.type & 0x000000FF;
}

// Could add helper functions here for the following :
//      - to reconstruct ViewSpacePos from uv and depth
//      - to get NDC from uv and depth
//      - to convert from device Z to linear Z

#endif