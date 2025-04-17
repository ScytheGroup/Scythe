#include "Color.hlsli"
#include "Common.hlsli"
#include "Math.hlsli"
#include "Samplers.hlsli"

#include "ShadingModels/ShadingLit.hlsli"
#include "ShadingModels/ShadingUnlit.hlsli"

struct PSInput
{
    float4 position: SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D<float> DepthTex : register(t0);
Texture2D<float4> DiffuseTexture : register(t1);
Texture2D<float4> NormalShadingModelTex : register(t2);
Texture2D<float3> ARMTex : register(t3);

float4 main(PSInput input) : SV_Target
{
    int3 pixelCoord = int3(input.uv.x * frameData.screenSize.x, input.uv.y * frameData.screenSize.y, 0);
    
    float3 worldPos = WorldPosFromDepth(DepthTex.Load(pixelCoord), input.uv, frameData.invViewProjection);
    float4 normalAndShadingModel = NormalShadingModelTex.Load(pixelCoord);
    float2 encodedNormal = normalAndShadingModel.xy;
    
    // TODO: add correct normal mapping using TBN matrix, we are incorrectly using tangent-space normal here :(
    float3 normal = NormalizedOctahedronDecode(encodedNormal);

    // R = ambient occlusion / G = roughness / B = metallic
    float3 arm = ARMTex.Load(pixelCoord);

    float4 diffuseColor = DiffuseTexture.Load(pixelCoord);
    uint shadingModel = normalAndShadingModel.z;
    
    float3 outputColor = diffuseColor.rgb;

    if (shadingModel == SHADING_MODEL_LIT)
    {
        outputColor = EvaluateMaterialLit(diffuseColor, arm, worldPos, normal);
    }
    else if (shadingModel == SHADING_MODEL_UNLIT)
    {
        outputColor = EvaluateMaterialUnlit(diffuseColor, worldPos, normal, input.uv);
    }
    
    return float4(outputColor, 1);
}