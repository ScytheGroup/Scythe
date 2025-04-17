export module Graphics:ResourceManager;

import :Mesh;
import :Material;
import :Device;
import :Shader;

export struct ResourceManager : InstancedSingleton<ResourceManager>
{
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    ResourceManager(ResourceManager&&) = delete;
    ResourceManager& operator=(ResourceManager&&) = delete;

    static constexpr size_t MaxAssetsToLoadPerFrame = 10000;

    std::mutex lazyLoadMutex;
    std::unordered_set<ILazyLoadedResource*> resourcesToLazyLoad;

    ShaderResourceLoader shaderLoader;
    ResourceLoader<Mesh> meshLoader{ MeshLoaderUtils{} };
    ResourceLoader<Texture> textureLoader{ TextureLoaderUtils{} };
    ResourceLoader<Material> materialLoader{ MaterialLoaderUtils{} };
    ShadingModelStore shadingModelStore;

    void LazyLoadResources(Device& device);
    void CleanupAssetLoaders();

    void LoadAllAssetMetadata();
    void Import(const std::filesystem::path& path);
    void SaveAll(EntityComponentSystem& ecs);

    void SaveDirtyAssets(EntityComponentSystem& ecs);

    bool HasDirtyAssets() const noexcept;

    ResourceManager(Device& device);

    Delegate<const Guid&> OnAssetDeleted;
    
    Delegate<> OnAssetsMetadataLoaded;
    Delegate<IAssetRef*> OnAssetImported;
};
