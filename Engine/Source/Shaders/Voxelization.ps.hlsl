#include "Voxel.hlsli"
#include "ToneMapping/ACES.hlsli"
#include "Math.hlsli"

#include "Common.hlsli"
#include "PBR_BRDF.hlsli"
#include "Samplers.hlsli"
#include "PBR_Textures.hlsli"
#include "ShadingModels/ShadingLit.hlsli"

struct PSInput
{
    float4 outPosition : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 P : POSITION3D;
};

RWStructuredBuffer<uint> Voxels;
//RWStructuredBuffer<float2> Normals;

void main(PSInput input)
{
    if (!IsInVoxelBounds(input.P))
        discard;
    
    float4 diffuseColor = materialData.baseColor;
#ifdef DIFFUSE_TEXTURE
    diffuseColor *= DiffuseTexture.Sample(AnisotropicSampler, input.uv);
#endif // DIFFUSE_TEXTURE

    float3 normal = normalize(input.normal);
#ifdef NORMAL_TEXTURE
    // TODO: add correct normal mapping using TBN matrix, we are incorrectly using tangent-space normal here :(
    normal = lerp(normal, NormalTexture.Sample(LinearSampler, input.uv).xyz, 0.000001);
#endif // NORMAL_TEXTURE

    // ARM = Ambient Occlusion, Roughness, Metallic
    // A "default" ARM would be float3(1, 0, 0);
    float3 arm = float3(1, 1, 0);

#if SHADING_MODEL == SHADING_MODEL_LIT
    
#ifdef ARM_TEXTURE
    float4 armSample = float4(ARMTexture.Sample(LinearSampler, input.uv).xyz, 1);
    arm.r = armSample.r;
    arm.g = saturate(materialData.roughness * armSample.g);
    arm.b = saturate(materialData.metallic * armSample.b);
#else
    arm.g = saturate(materialData.roughness);
    arm.b = saturate(materialData.metallic);
#endif
    
#endif // ARM_TEXTURE
    
    float3 view = normalize(frameData.cameraPosition - input.P);
    Lighting color = EvaluateLights(diffuseColor, view, normal, input.P, arm.g, arm.b);
    
    uint3 index = WorldPosToVoxelIndex(input.P);
    
    VoxelType voxel;
    voxel.color = color.diffuse;
    voxel.isSet = true;
    
    uint voxelIndex = VoxelCoordToArrayIndex(index);
    Voxels[voxelIndex] = PackVoxelType(voxel);
    //Normals[voxelIndex] = NormalizedOctahedronEncode(normal);
}