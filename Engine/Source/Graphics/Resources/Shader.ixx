export module Graphics:Shader;
import :Device;

import Common;

import <d3d11shader.h>;
import <filesystem>;

export class ShaderResourceLoader;

bool operator==(const D3D_SHADER_MACRO& lhs, const D3D_SHADER_MACRO& rhs)
{
    return (lhs.Name == rhs.Name || (lhs.Name != nullptr && rhs.Name != nullptr && strcmp(lhs.Name, rhs.Name) == 0)) &&
        (lhs.Definition == rhs.Definition ||
         (lhs.Definition != nullptr && rhs.Definition != nullptr && strcmp(lhs.Definition, rhs.Definition) == 0));
}

export struct ShaderKey
{
    std::filesystem::path filepath;
    std::vector<D3D_SHADER_MACRO> defines;
    bool createInputLayout = true;
    bool absolutePath = false;

    bool operator==(const ShaderKey& other) const
    {
        return filepath == other.filepath && defines == other.defines && createInputLayout == other.createInputLayout && absolutePath == other.absolutePath;
    }
};

template<>
struct std::formatter<ShaderKey>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const ShaderKey& key, std::format_context& ctx) const
    {
        using namespace std::literals;

        auto it = std::ranges::find_if(key.defines, [](auto&& define) {
            return std::string{ define.Name } == "SHADING_MODEL"s;
        });

        if (it == key.defines.end())
            return std::format_to(ctx.out(), "{}", key.filepath.string());
        
        return std::format_to(ctx.out(), "{}-{}", key.filepath.string(), it->Definition);
    }
};

template <>
struct std::hash<ShaderKey>
{
    size_t operator()(const ShaderKey& key) const noexcept
    {
        size_t h = std::hash<std::filesystem::path>()(key.filepath);

        std::unordered_set<size_t> defineHashes;
        for (const auto& define : key.defines)
        {
            size_t defineHash =
                std::hash<std::string>()(define.Name) ^ (std::hash<std::string>()(define.Definition) << 1);
            defineHashes.insert(defineHash);
        }

        for (const auto& defineHash : defineHashes)
        {
            h ^= defineHash + 0x9e3779b9 + (h << 6) + (h >> 2);
        }

        return h;
    }
};

export class Shader
{
    static inline const std::string ShadersFolder = "Source\\Shaders\\";

public:
    Shader(ShaderKey shaderKey, const std::string& entryPoint, const std::string& target);
    virtual ~Shader() = default;

    int GetStructuredBufferSlot(const std::string& bufferSlotName);
    int GetConstantBufferSlot(const std::string& bufferSlotName);
    int GetTextureSlot(const std::string& textureSlotName);

    virtual std::optional<std::string> RecompileShader(Device&);
    bool ImGuiDraw();

    bool IsValid() const { return shaderBytecode != nullptr; }

    std::string GetName();
protected:

    std::optional<std::string> Compile();
    void GenerateReflection(const ComPtr<ID3DBlob>& bytecode);

    ShaderKey key;
    ComPtr<ID3DBlob> shaderBytecode;

    std::string entryPoint;
    std::string target;

    std::map<std::string, int> constantBufferMapping;
    std::map<std::string, int> structuredBufferMapping;
    std::map<std::string, int> textureMapping;

    friend ShaderResourceLoader;
};

export class VertexShader : public Shader
{
public:
    VertexShader(ShaderKey shaderKey, Device& device);

    std::optional<std::string> RecompileShader(Device& device) override;

private:
    ComPtr<ID3D11VertexShader> vertexShader;
    ComPtr<ID3D11InputLayout> inputLayout;

    friend class Device;
};

export class PixelShader : public Shader
{
public:
    PixelShader(ShaderKey shaderKey, Device& device);

    std::optional<std::string> RecompileShader(Device& device) override;

private:
    ComPtr<ID3D11PixelShader> pixelShader;

    friend class Device;
};

export class ComputeShader : public Shader
{
public:
    ComputeShader(ShaderKey shaderKey, Device& device);
    
    std::optional<std::string> RecompileShader(Device& device) override;
private:
    ComPtr<ID3D11ComputeShader> computeShader;

    friend class Device;
};

export class GeometryShader : public Shader
{
public:
    GeometryShader(ShaderKey shaderKey, Device& device);
    
    std::optional<std::string> RecompileShader(Device& device) override;
private:
    ComPtr<ID3D11GeometryShader> geometryShader;

    friend class Device;
};

export class EntityComponentSystem;

class ShaderResourceLoader
{
    std::unordered_map<ShaderKey, std::unique_ptr<Shader>> cache;
    std::vector<Shader*> recompileList;
    std::vector<std::pair<Shader*, std::string>> failedToCompile;
public:
    void RecompileDirty(Device& device);

    void SubscribeForRecompile(Shader* shader);

    void SubmitFailedCompilation(Shader* shader, std::string errMsg);

    template <class TChild, class... Params> requires std::derived_from<TChild, Shader>
    TChild* Load(ShaderKey&& key, Params&&... params);
    
#ifdef IMGUI_ENABLED
    bool ImGuiDraw();
    void HandleShaderErrors();
#endif
private:
    void SubscribeAllForRecompile();
};

template <class TChild, class ... Params> requires std::derived_from<TChild, Shader>
TChild* ShaderResourceLoader::Load(ShaderKey&& key, Params&&... params)
{
    if (cache.contains(key))
        return static_cast<TChild*>(cache[key].get());

    auto shaderPtr = std::make_unique<TChild>(key, params...);
    auto ret = shaderPtr.get();
    cache[key] = std::move(shaderPtr);
    return static_cast<TChild*>(ret); 
}
