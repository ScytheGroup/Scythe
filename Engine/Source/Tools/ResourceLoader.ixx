module;

#include <winstring.h>

#include "ImGui/Icons/IconsFontAwesome6.h"
#include "Serialization/SerializationMacros.h"
#include "nlohmann/json.hpp"

export module Tools:ResourceLoader;
import Common;
import Reflection;
import :JobScheduler;

import <unordered_set>;
import <ranges>;
import <memory>;
import <algorithm>;
import <string>;
import <unordered_map>;
import <numeric>;

export class Device;
export class Editor;

export template <class T>
class ResourceLoader;

export struct AssetMetadata
{
    static AssetMetadata CreateFromPath(const std::filesystem::path& path);
    
    // Display name of the asset
    std::string name = "";
    
    // Path relative to the project root of the source file of that asset
    std::filesystem::path relativeSourceFilepath = "";

    // Whether or not the associated asset has been loaded from disk
    bool isLoaded = false;

    // Should this asset be saved or not good for code defined assets
    bool isTransient = false;

    bool ImGuiDraw();
};


export struct IAssetRef
{
    virtual Guid GetId() const = 0;
    virtual bool HasId() const = 0;
    virtual Type GetAssetType() const noexcept = 0;
    virtual ~IAssetRef() {};
    virtual void MarkDirty() = 0;
    virtual void LoadAsset() const = 0;
    virtual bool HasAsset() const = 0;
    virtual AssetMetadata& GetMetadata() = 0;

#ifdef IMGUI_ENABLED
    virtual bool ImGuiDrawEditContent() { return false; };
#endif
};

export template<class T>
struct AssetRef : public IAssetRef
{
    Guid assetId{};
    T* asset{};

    using AssetType = T;

    AssetRef() = default;
    
    AssetRef(const Guid& id, T* ptr = nullptr)
        : assetId{id}
        , asset{ptr}
    {}

    AssetRef(const std::string& str)
        : AssetRef{Guid(str)}
    {}
    
    AssetRef(const std::string_view& view)
        : AssetRef{Guid(view)}
    {}

    AssetRef(const AssetRef& other) = default;
    AssetRef& operator=(const AssetRef& other) = default;
    bool operator==(const IAssetRef&) const;
    bool operator==(const AssetRef&) const;

    T* operator->();
    const T* operator->() const;

    operator T*();
    operator const T*() const;

    bool HasAsset() const override;
    void LoadAsset() const override;
    bool IsEmpty() const noexcept { return !HasId() && !asset; }
    bool HasId() const override { return assetId.IsValid(); };
    void MarkDirty() override;
    Guid GetId() const override;
    Type GetAssetType() const noexcept override { return Type::GetType<AssetType>(); }
    AssetMetadata& GetMetadata() override;

    T* GetLoadedAsync() const;

#ifdef IMGUI_ENABLED
    bool ImGuiDrawEditContent() override;
#endif
    
    friend ResourceLoader;
};

template <typename T>
concept is_hashable = requires(T a)
{
    { std::hash<T>{}(a) } -> std::convertible_to<std::size_t>;
};

export class EntityComponentSystem;


AssetMetadata AssetMetadata::CreateFromPath(const std::filesystem::path& path)
{
    return AssetMetadata{
        .name = path.stem().string(),
        .relativeSourceFilepath = std::filesystem::relative(path, ProjectSettings::Get().projectRoot),
    };
}

export template <class T>
class ResourceLoader : public InstancedSingleton<ResourceLoader<T>>
{
    std::mutex jobMutex;
    std::unordered_map<Guid, JobHandle> pendingHandledJobs;
public:
    struct LoaderEntry
    {
        AssetMetadata metadata;
        std::unique_ptr<T> asset;
    };
    using CacheType = std::unordered_map<Guid, LoaderEntry>;
private:
    
    T* GetCached(const Guid& key);
    void LoadAssetFromDisk(const Guid& assetId);

    bool allDirty = false;
    std::unordered_set<Guid> dirtySet;
    std::unordered_set<Guid> assetsToDelete;
    CacheType cache;
    
    // Pattern from https://sean-parent.stlab.cc/presentations/2017-01-18-runtime-polymorphism/2017-01-18-runtime-polymorphism.pdf
    // Gives a better solution to the LoaderUtils pattern
    // Allows to enforce the interface of the LoaderUtils while also type erasing it from the actual type
    struct Concept
    {
        virtual ~Concept() = default;

        virtual std::filesystem::path GetProjectRelativeStoreFolder() const noexcept = 0;
        virtual std::string_view GetAssetDisplayName() const noexcept = 0;
        
        virtual void LoadPresets(Device& device, CacheType& cache) = 0;
        virtual std::unique_ptr<T> LoadAsset(ArchiveStream& loadStream) = 0;
        virtual void SaveAsset(const T& asset, ArchiveStream& saveStream, EntityComponentSystem& ecs) = 0;
        virtual bool HandlesExtension(const std::string& extension) = 0;
        virtual bool CanLoadAsync() const noexcept = 0;
    };

    template <typename LoaderUtil>
    struct Model final : Concept {
        Model(LoaderUtil x) : data(std::move(x)) { }

        std::filesystem::path GetProjectRelativeStoreFolder() const noexcept override  { return data.GetProjectRelativeStoreFolder(); }
        std::string_view GetAssetDisplayName() const noexcept override { return data.GetAssetDisplayName(); };
        
        void LoadPresets(Device& device, CacheType& cache) override { data.LoadPresets(device, cache); }
        std::unique_ptr<T> LoadAsset(ArchiveStream& loadStream) override { return data.LoadAsset(loadStream); };
        void SaveAsset(const T& asset, ArchiveStream& saveStream, EntityComponentSystem& ecs) override { data.SaveAsset(asset, saveStream, ecs); };
        bool HandlesExtension(const std::string& extension) override { return data.HandlesExtension(extension); };
        bool CanLoadAsync() const noexcept override { return data.CanLoadAsync(); }
        
        LoaderUtil data;
    };
    
    std::unique_ptr<Concept> loaderUtils;
public:

    Delegate<const Guid&, T*> OnAssetLoaded;
    Delegate<const Guid&> OnAssetDeleted;

    template<class LoaderUtil>
    ResourceLoader(LoaderUtil utils)
        : loaderUtils{ std::make_unique<Model<LoaderUtil>>(std::move(utils)) }
    {
    }

    bool HasDirtyAssets() const noexcept { return !dirtySet.empty(); }
    
    virtual ~ResourceLoader() = default;

    template <class TChild = T, class... Params> requires std::derived_from<TChild, T>
    AssetRef<TChild> ImportFromDisk(const std::filesystem::path& file, Params&&... params);

    template <class TChild = T, class... Params> requires std::derived_from<TChild, T>
    AssetRef<TChild> ImportFromMemory(const AssetMetadata& file, Params&&... params);

    template <class TChild = T> requires std::derived_from<TChild, T>
    void FillAsset(AssetRef<TChild>& ref);

    bool HasAsset(const IAssetRef& ref) const;

    const char* GetName(const AssetRef<T>& assetRef) const;
    AssetMetadata& GetMetadata(const AssetRef<T>& assetRef);

    void LoadDefaults(Device& device);

    template <class TChild> requires std::derived_from<TChild, T>
    void SaveAsset(const AssetRef<TChild>& assetRef, EntityComponentSystem& ecs);

    void SaveAllAssets(EntityComponentSystem& ecs);
    void SaveDirtyAssets(EntityComponentSystem& ecs);

    void LoadAllAssets();
    void LoadAssetsMetadata();

    template <class TChild> requires std::derived_from<TChild, T>
    bool IsDirty(const AssetRef<TChild>& ref);

    template <class TChild> requires std::derived_from<TChild, T>
    void MarkDirty(const AssetRef<TChild>& ref);
    
    template <class TChild> requires std::derived_from<TChild, T>
    void MarkToDelete(const AssetRef<TChild>& ref);

    void DeleteMarkedAssets();
    // This deletes this asset from the resource loader. This should never be called directly. Use MarkToDelete instead
    void DeleteAsset(const AssetRef<T>& ecs);

    bool HandlesExtension(const std::string& extension);

    std::string_view GetAssetDisplayName();

    Array<AssetRef<T>> GetAllAssetRefs(); 

#ifdef IMGUI_ENABLED
    virtual bool ImGuiDraw();
#endif

    void CleanupPendingLoads();

    auto begin() { return cache.begin(); }
    auto end() { return cache.end(); }
};

template <class T>
bool AssetRef<T>::operator==(const IAssetRef& other) const
{
    return assetId == other.GetId();
}

template <class T>
bool AssetRef<T>::operator==(const AssetRef& other) const
{
    return assetId == other.assetId;
}

template <class T>
T* AssetRef<T>::operator->()
{
    return asset;
}

template <class T>
const T* AssetRef<T>::operator->() const
{
    return asset;
}

template <class T>
void AssetRef<T>::LoadAsset() const
{
    if (!asset)
    {
        ResourceLoader<T>::Get().FillAsset(*const_cast<AssetRef*>(this));
    }
}

template <class T>
AssetRef<T>::operator T*()
{
    return asset;
}

template <class T>
AssetRef<T>::operator const T*() const
{
    return asset;
}

template <class T>
bool AssetRef<T>::HasAsset() const
{
    return asset != nullptr;
}

template <class T>
void AssetRef<T>::MarkDirty()
{
    ResourceLoader<T>::Get().MarkDirty(*this);
}

template <class T>
Guid AssetRef<T>::GetId() const
{
    return assetId;
}

template <class T>
template <class TChild, typename... Params> requires std::derived_from<TChild, T>
AssetRef<TChild> ResourceLoader<T>::ImportFromDisk(const std::filesystem::path& path, Params&&... params)
{
    auto ptr = std::make_unique<TChild>(path, std::forward<Params>(params)...);
    AssetRef<TChild> ref{Guid::Create(), ptr.get()};

    cache[ref.GetId()] = {
        AssetMetadata{
            .name = path.stem().string(),
            .relativeSourceFilepath = std::filesystem::relative(path, ProjectSettings::Get().projectRoot),
            .isLoaded = true,
            .isTransient = false
        },
        std::move(ptr),
    };
    MarkDirty(ref);
    return ref;
}

template <class T>
template <class TChild, class ... Params> requires std::derived_from<TChild, T>
AssetRef<TChild> ResourceLoader<T>::ImportFromMemory(const AssetMetadata& metadata, Params&&... params)
{
    auto ptr = std::make_unique<TChild>(std::forward<Params>(params)...);
    AssetRef<TChild> ref{Guid::Create(), ptr.get()};
    AssetMetadata copy = metadata;
    copy.isLoaded = true;
    cache[ref.GetId()] = { std::move(copy), std::move(ptr), };
    MarkDirty(ref);
    return ref;
}

template <class T>
template <class TChild> requires std::derived_from<TChild, T>
void ResourceLoader<T>::FillAsset(AssetRef<TChild>& ref)
{
    if (!ref.HasId())
        return;

    Guid id = ref.GetId();
    if (!cache.contains(id))
    {
        DebugPrint("AssetId={} wasn't in the cache. Resetting asset ref to empty", id.ToString());
        ref = {};
    }
    else
    {
        ref.asset = GetCached(id);
    }
}

template <class T>
bool ResourceLoader<T>::HasAsset(const IAssetRef& ref) const
{
    if (!ref.HasId())
        return false;
    
    return cache.contains(ref.GetId());
}

template <class T>
const char* ResourceLoader<T>::GetName(const AssetRef<T>& assetRef) const
{
    if (!assetRef.HasId())
        return "none";
    
    return cache.at(assetRef.GetId()).metadata.name.c_str();
}

template <class T>
AssetMetadata& ResourceLoader<T>::GetMetadata(const AssetRef<T>& assetRef)
{
    return cache[assetRef.GetId()].metadata;
}

template <class T>
void ResourceLoader<T>::LoadDefaults(Device& device)
{
    loaderUtils->LoadPresets(device, cache);
}

template<>
ArchiveStream& operator<<(ArchiveStream& ar, const AssetMetadata& data)
{
    ar << data.name;
    ar << data.relativeSourceFilepath;
    return ar;
}

template<>
ArchiveStream& operator>>(ArchiveStream& ar, AssetMetadata& data)
{
    ar >> data.name;
    ar >> data.relativeSourceFilepath;
    return ar;
}

template <class T>
template <class TChild> requires std::derived_from<TChild, T>
void ResourceLoader<T>::SaveAsset(const AssetRef<TChild>& assetRef, EntityComponentSystem& ecs)
{
    if (!assetRef || !HasAsset(assetRef))
        return;

    Guid key = assetRef.GetId();
    LoaderEntry& entry = cache[key];

    if (entry.metadata.isTransient)
        return;

    ArchiveStream saveStream{ loaderUtils->GetProjectRelativeStoreFolder() / key.ToString(), ArchiveStream::Out{} };
    saveStream << entry.metadata;
    loaderUtils->SaveAsset(*assetRef.asset, saveStream, ecs);
}

template <class T>
void ResourceLoader<T>::SaveAllAssets(EntityComponentSystem& ecs)
{
    for (const auto& [k, v] : cache)
        SaveAsset(AssetRef{ k, v.asset.get() }, ecs);
    
    allDirty = false;
}

template <class T>
void ResourceLoader<T>::SaveDirtyAssets(EntityComponentSystem& ecs)
{
    if (allDirty)
        SaveAllAssets(ecs);

    for (const auto& dirtyAsset : dirtySet)
    {
        AssetRef<T> ref{dirtyAsset};
        FillAsset(ref);
        SaveAsset(ref, ecs);
    }

    dirtySet.clear();
}

template <class T>
void ResourceLoader<T>::LoadAllAssets()
{
    for (const auto& [k, v] : cache)
    {
        if (v.metadata.isLoaded)
            continue;
        
        LoadAssetFromDisk(k);
    }    
}

template <class T>
void ResourceLoader<T>::LoadAssetsMetadata()
{
    std::filesystem::path assetFolder = ProjectSettings::Get().projectRoot / loaderUtils->GetProjectRelativeStoreFolder();

    std::filesystem::create_directories(assetFolder);
     for (auto entry : std::filesystem::directory_iterator(assetFolder))
     {
         ArchiveStream reader{ entry.path(), ArchiveStream::In{} };
         AssetMetadata metadata;
         reader >> metadata;
         
         cache[entry.path().filename().string()] = { metadata, nullptr };
     }
}

template <class T>
template <class TChild> requires std::derived_from<TChild, T>
bool ResourceLoader<T>::IsDirty(const AssetRef<TChild>& ref)
{
    return allDirty || (ref.HasId() && dirtySet.contains(ref.GetId()));
}

template <class T>
template <class TChild> requires std::derived_from<TChild, T>
void ResourceLoader<T>::MarkDirty(const AssetRef<TChild>& ref)
{
    if (ref.HasId())
    {
        dirtySet.insert(ref.GetId());
    }
}

template <class T>
template <class TChild> requires std::derived_from<TChild, T>
void ResourceLoader<T>::MarkToDelete(const AssetRef<TChild>& ref)
{
    assetsToDelete.insert(ref.GetId());
}

template <class T>
void ResourceLoader<T>::DeleteMarkedAssets()
{
    for (auto& asset : assetsToDelete)
    {
        DeleteAsset(asset);
    }
    assetsToDelete.clear();
}

template <class T>
void ResourceLoader<T>::DeleteAsset(const AssetRef<T>& ref)
{
    if (DebugPrintWindow("WARNING : Deleting this asset will most likely cause corruption/crashes if it is used in the scene. Are you sure you wan't to proceed to delete?") == IDOK)
    {
        auto assetPath = loaderUtils->GetProjectRelativeStoreFolder() / ref.GetId().ToString();
        std::filesystem::remove(assetPath);
        cache.erase(ref.GetId());

        OnAssetDeleted.Execute(ref.GetId());
        
        DebugPrint("Deleted asset at {}", assetPath.string());
    }
}

template <class T>
bool ResourceLoader<T>::HandlesExtension(const std::string& extension)
{
    return loaderUtils->HandlesExtension(extension);
}

template <class T>
std::string_view ResourceLoader<T>::GetAssetDisplayName()
{
    return loaderUtils->GetAssetDisplayName();
}

template <class T>
Array<AssetRef<T>> ResourceLoader<T>::GetAllAssetRefs()
{
    Array<AssetRef<T>> refs;
    for (auto& [id, ptr] : cache)
    {
        refs.emplace_back(id, ptr.asset.get());
    }
    return refs;
}

template <class T>
T* ResourceLoader<T>::GetCached(const Guid& key)
{
    Assert(cache.contains(key));

    if (!cache[key].metadata.isLoaded && !pendingHandledJobs.contains(key)) 
    {
        LoadAssetFromDisk(key);
        return nullptr;
    }
    
    return cache[key].asset.get();
}

template <class T>
void ResourceLoader<T>::LoadAssetFromDisk(const Guid& assetId)
{
    auto loadTask = [this, assetId]() {
        ArchiveStream loadStream{ loaderUtils->GetProjectRelativeStoreFolder() / assetId.ToString(), ArchiveStream::In{} };
        // Just to skip the header of the file
        AssetMetadata temp;
        loadStream >> temp;
        cache[assetId].asset = loaderUtils->LoadAsset(loadStream);
        cache[assetId].metadata.isLoaded = true;   
    };

    if (!loaderUtils->CanLoadAsync())
    {
        loadTask();
        return;
    }
    
    JobScheduler& scheduler = JobScheduler::Get();
    
    std::unique_lock lock{jobMutex};
    
    pendingHandledJobs[assetId] = scheduler.CreateJob([this, assetId]() {
        ArchiveStream loadStream{ loaderUtils->GetProjectRelativeStoreFolder() / assetId.ToString(), ArchiveStream::In{} };
        // Just to skip the header of the file
        AssetMetadata temp;
        loadStream >> temp;
        cache[assetId].asset = loaderUtils->LoadAsset(loadStream);
        cache[assetId].metadata.isLoaded = true;   
    });
}

template <class T>
AssetMetadata& AssetRef<T>::GetMetadata()
{
    return ResourceLoader<T>::Get().GetMetadata(*this);
}

template <class T>
T* AssetRef<T>::GetLoadedAsync() const
{
    if (IsEmpty() || !HasId())
        return nullptr;

    if (!HasAsset())
        LoadAsset();

    return asset;
}

export template<class T>
ArchiveStream& operator<<(ArchiveStream& stream, const AssetRef<T>& ref)
{
    stream << ref.GetId().ToString();
    return stream;
}

export template<class T>
ArchiveStream& operator>>(ArchiveStream& stream, AssetRef<T>& ref)
{
    std::string s;
    stream >> s;
    ref = {Guid(s)};
    return stream;
}

template <class T>
void ResourceLoader<T>::CleanupPendingLoads()
{
    std::unique_lock lock{jobMutex};
    
    std::vector<Guid> jobsToRemove;
        
    for (const auto& [key, job] : pendingHandledJobs)
    {
        if (job->GetState() == Job::FINISHED)
        {
            jobsToRemove.push_back(key);
            OnAssetLoaded.Execute(key, cache[key].asset.get());
        }
    }

    for (const auto& id : jobsToRemove)
    {
        pendingHandledJobs.erase(id);
    }
}

#ifdef IMGUI_ENABLED

import ImGui;

template <class T>
bool ResourceLoader<T>::ImGuiDraw()
{
    ImGui::PushID(Type::GetTypeName<T>().data());
    
    bool loadAll = ImGui::Button("Load All");
    ImGui::SameLine();
    if (ImGui::Button("Save All"))
    {
        allDirty = true;
    }
    
    for (const auto& [key, val] : cache) 
    {
        ImGui::PushID(std::format("{}-{}", Type::GetTypeName<T>().data(), key).c_str());
        if (!val.metadata.isLoaded)
        {
            ImGui::Text("%s", key.ToString());
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_FILE_IMPORT))
            {
                GetCached(key);
            }
        }
        else
        {
            AssetRef<T> asset{key, val.asset.get()};
            if (IsDirty(asset))
            {
                ImGui::Text(ICON_FA_ASTERISK);
            }
            else
            {
                if (ImGui::Button(ICON_FA_FLOPPY_DISK))
                {
                    MarkDirty(asset); 
                }    
            }
            ImGui::SameLine();

            if (ImGui::TreeNode(GetName(asset)))
            {
                if (asset.ImGuiDrawEditContent())
                    asset.MarkDirty();
                ImGui::TreePop();
            }
        }
        ImGui::PopID();
    }

    if (loadAll)
    {
        LoadAllAssets();  
    }

    ImGui::PopID();

    return false;
}

template <class T>
bool AssetRef<T>::ImGuiDrawEditContent()
{
    bool result = false;
    if (asset)
    {
        if (result = asset->ImGuiDraw(); result)
        {
            MarkDirty();
        }
    }
    return result;
}

bool AssetMetadata::ImGuiDraw()
{
    bool result = false;

    if (isTransient)
    {
        ImGui::BeginDisabled();
    }
    
    result |= ImGui::InputText("Name", &name);
    if (!isTransient)
    {
        if (ScytheGui::BeginLabeledField())
        {
            std::string path = relativeSourceFilepath.string();
            ImGui::BeginDisabled();
            ImGui::InputText("##", &path);
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_ELLIPSIS))
            {
                if (auto file = FileBrowser::BrowseForFile(); file)
                {
                    relativeSourceFilepath = std::filesystem::relative(ProjectSettings::Get().projectRoot, *file);
                }
            }
            ScytheGui::EndLabeledField("Relative Filepath");
        }
    } 

    if (isTransient)
    {
        ImGui::EndDisabled();
    }
    
    return result;
}

#endif