
#ifndef __VOXEL__
#define __VOXEL__

cbuffer VoxelizationData : register(b10)
{
    uint volumeResolution;
    float3 minBounds;
    float3 maxBounds;
    float coneTracingStep;
    float coneTracingAngle;
    float HDRPackingFactor;
    float maxConeTraceDistance;
    float TemporalUpdateAlpha;
}

struct VoxelType
{
    // expects normalized colors and emission
    float3 color;
    bool isSet;
};


struct VoxelGIConfig
{
    float IndirectBoost;
};

static const int DiffuseConeCount = 32;
static const float DiffuseConeAperture = 0.628319f / 2;
static const float3 DiffuseConeDirections[DiffuseConeCount] = {
    float3(0.898904f, 0.435512f, 0.0479745f),
    float3(0.898904f, -0.435512f, -0.0479745f),
    float3(0.898904f, 0.0479745f, -0.435512f),
    float3(0.898904f, -0.0479745f, 0.435512f),
    float3(-0.898904f, 0.435512f, -0.0479745f),
    float3(-0.898904f, -0.435512f, 0.0479745f),
    float3(-0.898904f, 0.0479745f, 0.435512f),
    float3(-0.898904f, -0.0479745f, -0.435512f),
    float3(0.0479745f, 0.898904f, 0.435512f),
    float3(-0.0479745f, 0.898904f, -0.435512f),
    float3(-0.435512f, 0.898904f, 0.0479745f),
    float3(0.435512f, 0.898904f, -0.0479745f),
    float3(-0.0479745f, -0.898904f, 0.435512f),
    float3(0.0479745f, -0.898904f, -0.435512f),
    float3(0.435512f, -0.898904f, 0.0479745f),
    float3(-0.435512f, -0.898904f, -0.0479745f),
    float3(0.435512f, 0.0479745f, 0.898904f),
    float3(-0.435512f, -0.0479745f, 0.898904f),
    float3(0.0479745f, -0.435512f, 0.898904f),
    float3(-0.0479745f, 0.435512f, 0.898904f),
    float3(0.435512f, -0.0479745f, -0.898904f),
    float3(-0.435512f, 0.0479745f, -0.898904f),
    float3(0.0479745f, 0.435512f, -0.898904f),
    float3(-0.0479745f, -0.435512f, -0.898904f),
    float3(0.57735f, 0.57735f, 0.57735f),
    float3(0.57735f, 0.57735f, -0.57735f),
    float3(0.57735f, -0.57735f, 0.57735f),
    float3(0.57735f, -0.57735f, -0.57735f),
    float3(-0.57735f, 0.57735f, 0.57735f),
    float3(-0.57735f, 0.57735f, -0.57735f),
    float3(-0.57735f, -0.57735f, 0.57735f),
    float3(-0.57735f, -0.57735f, -0.57735f)
};

static const uint3 MaxPackingColorValues = uint3(
    2048 - 1, 
    1024 - 1,
    1024 - 1
);

static uint DarkPackingPower = 8;

// Packs color to R11G10B10 with last bit being flag
uint PackVoxelType(VoxelType vType)
{
    float3 rgb = vType.color;
    rgb = saturate(rgb / HDRPackingFactor);
    float maxVal = max(max(1.0 / 255.0, rgb.x), max(rgb.y, rgb.z));
    float divisor = 255.0 / maxVal;
    uint R = (uint)(rgb.x * divisor);
    uint G = (uint)(rgb.y * divisor);
    uint B = (uint)(rgb.z * divisor);
    uint M = (uint)(maxVal * 255.0);
    return R << 24 | G << 16 | B << 8 | (M & 0x8F) << 1 | vType.isSet;
}

// Unpacks from R11G10B10 with last bit being flag
VoxelType UnpackVoxelType(uint packedData)
{
    uint R = packedData >> 24;
    uint G = (packedData >> 16) & 0xFF;
    uint B = (packedData >> 8) & 0xFF;
    uint M = packedData & 0x8F;
    float3 color = float3(R, G, B) * (M * HDRPackingFactor) / (255.0f * 255.0f);
    
    VoxelType voxel;
    voxel.color = color;
    voxel.isSet = packedData & 1;
    return voxel;
}

uint VoxelCoordToArrayIndex(uint3 coord)
{
    return coord.x + coord.y * volumeResolution + coord.z * volumeResolution * volumeResolution;
}

float3 GetVoxelBoundsCenter()
{
    return (maxBounds + minBounds) / 2.0f;
}

float3 GetVoxelBoundsSize()
{
    return (maxBounds - minBounds);
}

float3 GetVoxelSize()
{
    return GetVoxelBoundsSize() / volumeResolution;
}

float3 GetHalfVoxelSize()
{
    return GetVoxelSize() / 2.0f;
}

uint3 WorldPosToVoxelIndex(float3 worldPos)
{
    return (worldPos - GetVoxelBoundsCenter() + GetVoxelBoundsSize() / 2) / GetVoxelBoundsSize() * volumeResolution;
}

float3 VoxelIndexToWorldPos(uint3 voxelPos)
{
    return ((voxelPos / (float)volumeResolution) - 0.5f) * GetVoxelBoundsSize() + GetVoxelBoundsCenter();
}

bool IsInVoxelBounds(float3 worldPos)
{
    return worldPos.x > minBounds.x
        && worldPos.y > minBounds.y
        && worldPos.z > minBounds.z
        && worldPos.x < maxBounds.x
        && worldPos.y < maxBounds.y
        && worldPos.z < maxBounds.z;
}

float4 ConeTraceSingle(in Texture3D illuminationVolume, SamplerState pointSampler, float3 origin, float3 normal, float3 direction)
{
    const float voxelSize = GetVoxelSize().x;
    const float coneCoefficient = 2 * tan(coneTracingAngle * 0.5);
    float dist = voxelSize;
    float stepDist = dist;

    float3 startPos = origin + normal * voxelSize;

    float3 color = 0.0f;
    float alpha = 0;

    uint maxIterations = 100;

    uint iteration = 0;
    while (alpha < 1 && dist < maxConeTraceDistance && iteration < maxIterations)
    {
        float3 p0 = startPos + direction * dist;

        float diameter = max(voxelSize, coneCoefficient * dist);
        
        float mip = log2(diameter / voxelSize);
        
        float3 voxelIndex = (float3(WorldPosToVoxelIndex(p0)) + 0.5f) / volumeResolution;
        float4 sampleColor = illuminationVolume.SampleLevel(pointSampler, saturate(voxelIndex), mip);
        sampleColor *= stepDist / voxelSize / 2.0f;

        float a = 1.0f - alpha;
        color += a * sampleColor.rgb;
        alpha += a * sampleColor.a;

        stepDist = diameter * coneTracingStep;
        dist += stepDist;

        iteration++;
    }

    return float4(color, 1);
}

float4 ConeTraceInVoxelIlluminationVolume(in Texture3D illuminationVolume, SamplerState pointSampler, float3 worldPosition, float3 surfaceNormal)
{
    if (!IsInVoxelBounds(worldPosition))
        return 0;
    
    float4 color = 0;
    
    float sum = 0;
    for (int i = 0; i < DiffuseConeCount; ++i)
    {
        float3 coneDirection = normalize(DiffuseConeDirections[i]);
        float3 sampleDirection = coneDirection;
        
        float cosTheta = dot(surfaceNormal, sampleDirection);
        if (cosTheta <= 0)
            continue;
        
        color += ConeTraceSingle(illuminationVolume, pointSampler, worldPosition, surfaceNormal, sampleDirection) * cosTheta;
        sum += cosTheta;
    }
    
    color /= sum;
    
    color.rgb = max(0, color.rgb);
    color.a = saturate(color.a);

    return color;
}


#endif