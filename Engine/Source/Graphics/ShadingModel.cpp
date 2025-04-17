module Graphics:ShadingModel;

import :ShadingModel;
import :ResourceManager;

using PermutationFlags = uint8_t;

void AddPBRTextures(std::vector<D3D_SHADER_MACRO>& outMacros, PermutationFlags permutationFlags)
{
    if (permutationFlags & std::to_underlying(ShadingModelStore::Permutations::DIFFUSE_TEXTURE))
        outMacros.emplace_back("DIFFUSE_TEXTURE", "1");
    if (permutationFlags & std::to_underlying(ShadingModelStore::Permutations::NORMAL_TEXTURE))
        outMacros.emplace_back("NORMAL_TEXTURE", "1");
    if (permutationFlags & std::to_underlying(ShadingModelStore::Permutations::ARM_TEXTURE))
        outMacros.emplace_back("ARM_TEXTURE", "1");
}

ShadingModel CreateLitShadingModel(ShaderResourceLoader& shaderLoader, PermutationFlags permutationFlags, Device& device)
{
    std::vector<D3D_SHADER_MACRO> litMacroDefines{ { "SHADING_MODEL", "SHADING_MODEL_LIT" } };
    AddPBRTextures(litMacroDefines, permutationFlags);
    ShadingModel litShadingModel(shaderLoader, { "Mesh.vs.hlsl" }, { "Mesh.ps.hlsl", litMacroDefines }, device);
    return litShadingModel;
}

ShadingModel CreateVoxelizationShadingModel(ShaderResourceLoader& shaderLoader, PermutationFlags permutationFlags, Device& device)
{
    std::vector<D3D_SHADER_MACRO> litMacroDefines{
        { "SHADING_MODEL", "SHADING_MODEL_LIT" },

        // Find a way to match these two with real settings
        { "DIRECTIONAL_PROJECTED_SHADOWS", "1" },
        { "POINT_SHADOWS", "1" },
        { "SPOT_SHADOWS", "1" },
        
        { "PCF", "0" },
        { "ENABLE_IBL", "0" }, 
        { "DIFFUSE_IRRADIANCE", "DIFFUSE_IRRADIANCE_SH" },
    };
    AddPBRTextures(litMacroDefines, permutationFlags);
    ShadingModel litShadingModel(shaderLoader, { "Voxelization.vs.hlsl" }, { "Voxelization.ps.hlsl", litMacroDefines }, device);
    return litShadingModel;
}

ShadingModel CreateUnlitShadingModel(ShaderResourceLoader& shaderLoader, PermutationFlags permutationFlags, Device& device)
{
    std::vector<D3D_SHADER_MACRO> unlitMacroDefines{ { "SHADING_MODEL", "SHADING_MODEL_UNLIT" } };
    if (permutationFlags & std::to_underlying(ShadingModelStore::Permutations::DIFFUSE_TEXTURE))
        unlitMacroDefines.emplace_back("DIFFUSE_TEXTURE", "1");
    ShadingModel unlitShadingModel(shaderLoader, { "Mesh.vs.hlsl" }, { "Mesh.ps.hlsl", unlitMacroDefines }, device);
    return unlitShadingModel;
}

ShadingModelStore::ShadingModelStore(ShaderResourceLoader& shaderLoader, Device& device)
{
    // Generate all permutations of all shaders to avoid runtime cost (but increase startup time)
    int maxFlag = std::to_underlying(magic_enum::enum_values<Permutations>().back());
    int upperBound = maxFlag * 2;
    for (int i = 0; i < upperBound; ++i)
    {
        shadingModels[{LIT, i}] = CreateLitShadingModel(shaderLoader, i, device);
        shadingModels[{VOXELIZATION, i}] = CreateVoxelizationShadingModel(shaderLoader, i, device);
        shadingModels[{UNLIT, i}] = CreateUnlitShadingModel(shaderLoader, i, device);
    }
}

PermutationFlags GetShadingModelPermutationsFromMaterial(const Material* material)
{
    uint8_t permutationFlags = 0;
    if (!material)
        return permutationFlags;
    
    if (material->diffuseTexture.HasId())
    {
        permutationFlags |= std::to_underlying(ShadingModelStore::Permutations::DIFFUSE_TEXTURE);
    }
    
    if (material->normalTexture.HasId())
    {
        permutationFlags |= std::to_underlying(ShadingModelStore::Permutations::NORMAL_TEXTURE);
    }
    
    if (material->armTexture.HasId())
    {
        permutationFlags |= std::to_underlying(ShadingModelStore::Permutations::ARM_TEXTURE);
    }

    return permutationFlags;
}

const ShadingModel* ShadingModelStore::GetShadingModel(ShadingModelName name, const Material* material) const
{
    return &shadingModels.at({ name, GetShadingModelPermutationsFromMaterial(material) });
}

ShadingModel::ShadingModel(ShaderResourceLoader& shaderLoader, ShaderKey vertexShaderKey, ShaderKey pixelShaderKey, Device& device)
{
    vertexShader = shaderLoader.Load<VertexShader>(std::move(vertexShaderKey), device);
    pixelShader = shaderLoader.Load<PixelShader>(std::move(pixelShaderKey), device);
}
