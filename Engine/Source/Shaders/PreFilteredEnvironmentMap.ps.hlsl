#include "Constants.hlsli"
#include "Math.hlsli"
#include "Samplers.hlsli"
#include "ImportanceSampling.hlsli"
#include "ToneMapping/Reinhard.hlsli"
#include "PBR_BRDF.hlsli"

static const uint SampleCount = 64;
static const uint MaxMipLevel = 5;
static const uint CubeSize = 512;

TextureCube EnvironmentMap;

cbuffer FilterData
{
    uint mipLevel;
    uint mipOffset;
}

struct PSInput
{
    float4 outPosition : SV_POSITION;
    float3 viewDirection : POSITION;
};

float PrefilteredImportanceSampling(float ipdf, float solidAngleTexel) {
    // See: "Real-time Shading with Filtered Importance Sampling", Jaroslav Krivanek
    // Prefiltering doesn't work with anisotropy
    const float invNumSamples = 1.0 / float(SampleCount);
    const float K = 4.0;
    float solidAngleSample = invNumSamples * ipdf;
    float mipLevel = log2(K * solidAngleSample / solidAngleTexel) * 0.5;
    return mipLevel;
}

float4 main(PSInput input) : SV_Target
{
    float roughness = mipLevel / float(MaxMipLevel - 1);
    
    // This convolution only works on isotropic materials.
    // Setting N = R = V will remove grazing specular reflections but this is considered an acceptable compromise
    
    // Somehow, flipping this fixes all my life's problems :-) 
    input.viewDirection.y *= -1;

    float3 N = normalize(input.viewDirection);
    float3 R = N;
    float3 V = R;

    if (mipLevel == 0)
    {
        return float4(EnvironmentMap.SampleLevel(LinearSampler, input.viewDirection, mipOffset + mipLevel).rgb, 1);
    }

    float3x3 tangentBasis = GetTangentBasis(N);
    float saTexel = 4.0 * PI / (6.0 * CubeSize * CubeSize);
    
    float3 prefilteredColor = 0;
    float totalWeight = 0;
    for (uint i = 0; i < SampleCount; ++i)
    {
        float2 Xi = Hammersley(i, SampleCount);
        float3 H = ImportanceSampleGGX(Xi, roughness, N);
        float3 L = 2 * dot(V, H) * H - V;

        float NoL = saturate(dot(N, L));
        if (NoL > 0)
        {
            float NoH = saturate(dot(N, H));
            float invPdf = 4 / (D_GGX(NoH, roughness) + 0.0001f);
            float mipLevel = PrefilteredImportanceSampling(invPdf, saTexel);

            float3 color = EnvironmentMap.SampleLevel(LinearSampler, L, mipOffset + mipLevel).rgb;
            prefilteredColor += color * NoL;
            totalWeight += NoL;
        }
    }

    prefilteredColor /= max(totalWeight, SMALL_FLOAT_EPSILON);
    // Removes NaNs
    prefilteredColor = -min(-prefilteredColor, 0);
    return float4(prefilteredColor, 1.0f);
}
