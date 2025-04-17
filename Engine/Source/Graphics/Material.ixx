export module Graphics:Material;

export import :ShadingModel;
import :Device;
import :Resources;
import Common;
import Tools;

export struct Material : ILazyLoadedResource
{
    struct MaterialData;
    
    Material() = default;
    Material(MaterialData initialData);

    Material(const Material& other);
    Material(Material&& other) noexcept = default;
    Material& operator=(const Material& other);
    Material& operator=(Material&& other) noexcept = default;

    AssetRef<Texture> diffuseTexture;
    AssetRef<Texture> normalTexture;
    AssetRef<Texture> emissiveTexture;
    AssetRef<Texture> armTexture;
    
    // AO, Roughness and Metallic are often stored in one texture called ARM texture
    std::optional<Texture::TextureData> aoTextureData;
    std::optional<Texture::TextureData> roughnessTextureData;
    std::optional<Texture::TextureData> metallicTextureData;
    
    ShadingModelStore::ShadingModelName shadingModelName = ShadingModelStore::NONE;

    struct MaterialData
    {
        Color diffuseColor = Color(.8, .8, .8, 1);
        float roughness = 1;
        float metallic = 0;
        float emissive = 0;
    } data{};

    static ConstantBuffer<MaterialData> constantBuffer;
    
    void LoadResource(Device& device) override;

#ifdef IMGUI_ENABLED
    bool ImGuiDraw();
#endif
};

export struct MaterialLoaderUtils
{
    std::filesystem::path GetProjectRelativeStoreFolder() const noexcept { return "Resources/Imported/Materials"; };
    std::string_view GetAssetDisplayName() const noexcept { return "Material"; };
    
    void LoadPresets(Device& device, ResourceLoader<Material>::CacheType& cache);
    std::unique_ptr<Material> LoadAsset(ArchiveStream& readStream);
    void SaveAsset(const Material& asset, ArchiveStream& saveStream, EntityComponentSystem& ecs);
    bool HandlesExtension(const std::string& extension);
    bool CanLoadAsync() const noexcept { return true; };
};

