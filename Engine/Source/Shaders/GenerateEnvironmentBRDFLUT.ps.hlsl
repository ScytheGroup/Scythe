#include "Common.hlsli"
#include "Constants.hlsli"
#include "PBR_BRDF.hlsli"
#include "ImportanceSampling.hlsli"

static const uint SampleCount = 1024;

struct PSInput
{
    float4 position: SV_POSITION;
    float2 uv : TEXCOORD;
};

float2 IntegrateBRDF(float NoV, float roughness)
{
    float3 V = float3(sqrt(1.0f - NoV * NoV), 0, NoV);

    float A = 0;
    float B = 0;

    static const float3 N = float3(0, 0, 1);

    for (uint i = 0; i < SampleCount; ++i)
    {
        float2 xi = Hammersley(i, SampleCount);
        float3 H = ImportanceSampleGGX(xi, roughness, N);
        float3 L = 2.0f * dot(V, H) * H - V;

        float NoL = saturate(L.z);
        float NoH = saturate(H.z);
        float VoH = saturate(dot(V, H));
        if (NoL > 0)
        {
            float G = G_SchlickBeckman(NoV, NoL, roughness);
            float G_Vis = (G * VoH) / (NoH * NoV);
            float Fc = pow(1 - VoH, 5);
            A += (1 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }

    return float2(A, B) / SampleCount;
}

float2 main(PSInput input) : SV_Target
{
    float2 integratedBRDF = IntegrateBRDF(input.uv.x, input.uv.y);
    return integratedBRDF;
}