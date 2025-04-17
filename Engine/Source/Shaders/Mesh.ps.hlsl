#include "Common.hlsli"
#include "Samplers.hlsli"
#include "PBR_Textures.hlsli"
#include "GBuffer.hlsli"
#include "Color.hlsli"
#include "Math.hlsli"

struct PSInput
{
    float4 position: SV_POSITION;

    float3 worldPosition : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

GBuffer main(PSInput input)
{
    GBuffer output = (GBuffer) 0;
    output.diffuseColor = materialData.baseColor;
    
#if SHADING_MODEL == SHADING_MODEL_LIT
    
#ifdef DIFFUSE_TEXTURE
    output.diffuseColor *= DiffuseTexture.Sample(WrapLinearSampler, input.uv);
#endif // DIFFUSE_TEXTURE

    float3 normal = normalize(input.normal);
#ifdef NORMAL_TEXTURE
    // TODO: add correct normal mapping using TBN matrix, we are incorrectly using tangent-space normal here :(
    normal = lerp(normal, NormalTexture.Sample(WrapLinearSampler, input.uv).xyz, 0.000001);
#endif // NORMAL_TEXTURE
    output.normal.xy = NormalizedOctahedronEncode(normal);

    // ARM = Ambient Occlusion, Roughness, Metallic
    // A "default" ARM would be float3(1, 0, 0);
#ifdef ARM_TEXTURE
    float4 arm = float4(ARMTexture.Sample(WrapLinearSampler, input.uv).xyz, 1);
    output.arm.r = arm.r;
    output.arm.g = saturate(materialData.roughness * arm.g);
    output.arm.b = saturate(materialData.metallic * arm.b);
#else
    output.arm.r = 1;
    output.arm.g = saturate(materialData.roughness);
    output.arm.b = saturate(materialData.metallic);
#endif // ARM_TEXTURE

#elif SHADING_MODEL == SHADING_MODEL_UNLIT
    
#ifdef DIFFUSE_TEXTURE
    output.diffuseColor *= DiffuseTexture.Sample(WrapLinearSampler, input.uv);
#endif // DIFFUSE_TEXTURE

#endif
    output.normal.z = SHADING_MODEL;
    
    // to see in the editor
    output.normal.w = 1.0f;

    return output;
}