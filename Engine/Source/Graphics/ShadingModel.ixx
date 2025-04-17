export module Graphics:ShadingModel;

import Common;
import :Resources;

export class ShaderResourceLoader;
export struct Material;

export struct ShadingModel
{
    ShadingModel() = default;
    ShadingModel(ShaderResourceLoader& shaderLoader, ShaderKey vertexShaderKey, ShaderKey pixelShaderKey, Device& device);

    VertexShader* vertexShader = nullptr;
    PixelShader* pixelShader = nullptr;
};

export class ShadingModelStore
{
public:
    enum ShadingModelName
    {
        NONE,
        CUSTOM,
        LIT,
        UNLIT,
        VOXELIZATION,
    };

    enum class Permutations : uint8_t
    {
        DIFFUSE_TEXTURE = 1 << 0,
        NORMAL_TEXTURE = 1 << 1,
        ARM_TEXTURE = 1 << 2,
    };
    
    ShadingModelStore(ShaderResourceLoader& shaderLoader, Device& device);

    const ShadingModel* GetShadingModel(ShadingModelName name, const Material* material) const;
    
private:
    using PermutationFlags = uint8_t;

    using PairHash = decltype(
        [](const std::pair<ShadingModelName, PermutationFlags>& p) -> size_t
        {
            return std::hash<int>()(static_cast<int>(p.first)) ^ std::hash<PermutationFlags>()(p.second);
        }
    );

    using PairEqual = decltype(
        [](const std::pair<ShadingModelName, PermutationFlags>& lhs, const std::pair<ShadingModelName, PermutationFlags>& rhs)
        {
            return lhs.first == rhs.first && lhs.second == rhs.second;
        }
    );
    std::unordered_map<std::pair<ShadingModelName, PermutationFlags>, ShadingModel, PairHash, PairEqual> shadingModels;
};

