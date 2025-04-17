module Graphics:ResourceManager;

#if EDITOR
import Editor;
#endif

import :ResourceManager;

class EntityComponentSystem;

void ResourceManager::LazyLoadResources(Device& device)
{
    std::vector<ILazyLoadedResource*> toLoad;
    {
        std::unique_lock lock{lazyLoadMutex};
        toLoad = resourcesToLazyLoad
            | std::views::take(std::min(MaxAssetsToLoadPerFrame, resourcesToLazyLoad.size()))
            | std::ranges::to<std::vector<ILazyLoadedResource*>>();
    }
    
    for (auto&& load : toLoad)
    {
        if (!load->IsLoaded())
            load->LoadResource(device);
    }
    
    std::unique_lock lock{lazyLoadMutex};
    for (auto&& toErase : toLoad)
    {
        resourcesToLazyLoad.erase(toErase);
    }
}

void ResourceManager::CleanupAssetLoaders()
{
    textureLoader.DeleteMarkedAssets();
    materialLoader.DeleteMarkedAssets();
    meshLoader.DeleteMarkedAssets();

    textureLoader.CleanupPendingLoads();
    materialLoader.CleanupPendingLoads();
    meshLoader.CleanupPendingLoads();
}

void ResourceManager::LoadAllAssetMetadata()
{
    textureLoader.LoadAssetsMetadata();
    materialLoader.LoadAssetsMetadata();
    meshLoader.LoadAssetsMetadata();

    OnAssetsMetadataLoaded.Execute();
}

void ResourceManager::Import(const std::filesystem::path& path)
{
    // removes first . of the extension
    auto ext = path.extension().string().substr(1);

    if (textureLoader.HandlesExtension(ext))
    {
        Texture::TextureData data(path, Texture::TEXTURE_2D, Texture::DEFAULT, Texture::TEXTURE_RTV | Texture::TEXTURE_SRV, true);
        
        AssetRef<Texture> tex = textureLoader.ImportFromMemory(AssetMetadata::CreateFromPath(path), data);
        tex->RegisterLazyLoadedResource(true);
        OnAssetImported.Execute(&tex);
    }
    else if (meshLoader.HandlesExtension(ext))
    {
        AssetRef<Mesh> mesh = meshLoader.ImportFromDisk(path);
        mesh->RegisterLazyLoadedResource(false);
        OnAssetImported.Execute(&mesh);
    }
}

void ResourceManager::SaveAll(EntityComponentSystem& ecs)
{
    textureLoader.SaveAllAssets(ecs);
    materialLoader.SaveAllAssets(ecs);
    meshLoader.SaveAllAssets(ecs);
}

void ResourceManager::SaveDirtyAssets(EntityComponentSystem& ecs)
{
    textureLoader.SaveDirtyAssets(ecs);
    materialLoader.SaveDirtyAssets(ecs);
    meshLoader.SaveDirtyAssets(ecs);
}

bool ResourceManager::HasDirtyAssets() const noexcept
{
    return textureLoader.HasDirtyAssets()
        || materialLoader.HasDirtyAssets()
        || meshLoader.HasDirtyAssets();
}

ResourceManager::ResourceManager(Device& device)
    : shadingModelStore{ shaderLoader, device }
{
    textureLoader.LoadDefaults(device);
    materialLoader.LoadDefaults(device);
    meshLoader.LoadDefaults(device);

    auto callAssetDelegate = [this](const Guid& id) { OnAssetDeleted.Execute(id); };
    textureLoader.OnAssetDeleted.Subscribe(callAssetDelegate);
    meshLoader.OnAssetDeleted.Subscribe(callAssetDelegate);
    materialLoader.OnAssetDeleted.Subscribe(callAssetDelegate);

    LoadAllAssetMetadata();
}
