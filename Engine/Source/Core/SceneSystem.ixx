module;
#include "Common/Macros.hpp"
export module Core:SceneSystem;

import Systems;
import Common;
import :Scene;
import :SceneSerializer;

export class SceneLoader
{
public:
    virtual std::optional<Scene> LoadScene() = 0;
    virtual std::optional<std::filesystem::path> GetSaveFilePath(const std::string& sceneName)
    {
        std::string strippedString = sceneName;
        strippedString.erase(std::ranges::remove_if(strippedString, isspace).begin(), strippedString.end());
        return FileBrowser::PickSaveLocation(std::format("{}.json", strippedString), ProjectSettings::Get().projectRoot / "Resources");
    }
    virtual ~SceneLoader() {}
};

export class SceneFileLoader : public SceneLoader
{
    std::optional<std::filesystem::path> mapFilePath;   
public:
    SceneFileLoader() = default;
    SceneFileLoader(const std::filesystem::path& path) : mapFilePath{path} {};
    std::optional<Scene> LoadScene() override
    {
        if (!mapFilePath)
        {
            mapFilePath = FileBrowser::BrowseForFile(ProjectSettings::Get().projectRoot / "Resources");
        }

        if (!mapFilePath)
        {
            DebugPrint("Path of the scene file was invalid");
            return {};
        }
        
        std::ifstream file(*mapFilePath);
        if (!file)
        {
            DebugPrint("Error opening file");
            return {};
        }
        
        std::string jsonContent{
            std::istreambuf_iterator{file},
            std::istreambuf_iterator<char>{}
        };
        return SceneSerializer::ParseFromJson(jsonContent);
    }
    
    std::optional<std::filesystem::path> GetSaveFilePath(const std::string& sceneName) override
    {
        if (!mapFilePath)
            mapFilePath = SceneLoader::GetSaveFilePath(sceneName);
        
        return mapFilePath;
    }
};

export class SceneFuncLoader : public SceneLoader
{
public:
    using SceneLoadFunc = std::function<std::optional<Scene>()>;
    SceneLoadFunc loadFunc;
    SceneFuncLoader(const SceneLoadFunc& func) : loadFunc{func} {}
    std::optional<Scene> LoadScene() override { return loadFunc(); }
};

export using SceneIdentifier = std::variant<std::string, std::filesystem::path>;

export class SceneSystem : public System
{
protected:
    struct SceneLoad
    {
        std::unique_ptr<SceneLoader> loader;
        std::optional<Scene> sceneData;
    };
    
    std::map<std::string, SceneLoad> scenes;

    // If its a string load from registered scenes. If path load from disk
    std::optional<std::variant<std::string, std::filesystem::path>> sceneToLoadOnNextFrame;
    
    // Loads from registered scenes
    // Returns true if scene load succeded
    bool LoadScene(const std::string& sceneName, EntityComponentSystem& ecs);

    // Loads from disk first. then register scene and load
    // Returns true if scene load succeded
    bool LoadScene(const std::filesystem::path& scenePath, EntityComponentSystem& ecs);

    void LoadSceneOrDefault(const SceneIdentifier& sceneIdentifier, EntityComponentSystem& ecs);

    void SaveScene(const std::string& sceneName, const SceneLoad& loader) const;

    // Very dirty workaround for having ECS in draw imgui. It's relatively safe tho since the ECS doesnt change during runtime
    EntityComponentSystem* ecs;

    std::vector<std::string> scenesToRemove;
public:
    SceneSystem();
    ~SceneSystem();
    void RegisterScene(const std::string& sceneName, std::unique_ptr<SceneLoader>&& createScene);
    void RegisterScene(const std::string& sceneName, Scene&& scene);

    // This won't release all of the scenes assets, only it's existence with regards to the SceneSystem
    void Unregister(const std::string& sceneName);

    // Loads scene at next update
    void LoadSceneOnNextFrame(const SceneIdentifier& sceneIdentifier);

    // Loads scene immediately in the ECS
    void LoadSceneImmediate(const SceneIdentifier& sceneIdentifier, EntityComponentSystem& ecsRef);
    
    // Could move this to SceneLoader since it could depend on the loader type
    void SaveScene(const std::string& sceneName);
    // Saves the scene in memory then saves to disk
    void SaveScene(const std::string& sceneName, const Scene& sceneToSave);

    void Init() override;
    void Update(double delta) override;

#ifdef IMGUI_ENABLED
    void DrawImGuiWindow(bool* opened);
#endif
};
