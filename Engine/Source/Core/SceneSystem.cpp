module;

#include "Common/Macros.hpp"
#include "ImGui/Icons/IconsFontAwesome6.h"

module Core:SceneSystem;

#if EDITOR
import Editor;
#endif

import :SceneSystem;
import :EntityComponentSystem;
import :SceneSerializer;

bool SceneSystem::LoadScene(const std::string& sceneName, EntityComponentSystem& ecs)
{
    if (!scenes.contains(sceneName))
    {
        DebugPrint("Scene \"{}\" wasn't registered", sceneName);
        return false;
    }
    
    SceneLoad& load = scenes[sceneName];

    Assert(load.loader.get(), "Tried loading a scene without data or a func");

    if (!load.sceneData)
    {
        std::optional<Scene> scene = load.loader->LoadScene();
        if (!scene.has_value())
        {
            DebugPrint("Scene failed to load");
            return false;
        }
        
        load.sceneData = *scene;
        load.sceneData->sceneSettings.worldName = sceneName;
    }
    
    ecs.LoadScene(*load.sceneData);

    return true;
}

bool SceneSystem::LoadScene(const std::filesystem::path& scenePath, EntityComponentSystem& ecs)
{
    auto loader = std::make_unique<SceneFileLoader>(scenePath);
    
    std::optional<Scene> scene = loader->LoadScene();
    if (!scene.has_value())
    {
        DebugPrint("Scene failed to load");
        return false;
    }
    
    auto& loadScene = scenes[scene->sceneSettings.worldName];
    loadScene.loader = std::move(loader);
    loadScene.sceneData = std::move(*scene);
    
    ecs.LoadScene(*loadScene.sceneData);

    return true;
}

void SceneSystem::LoadSceneOrDefault(const SceneIdentifier& sceneIdentifier, EntityComponentSystem& ecs)
{
    bool sucess = std::visit(Visitor{
        [&](const std::filesystem::path& val)
        {
            return LoadScene(val, ecs);
        },
        [&](const std::string& val)
        {
            return LoadScene(val, ecs);
        }
    }, sceneIdentifier);

    if (!sucess)
    {
        ecs.LoadScene(Scene{});
    }
}

SceneSystem::SceneSystem()
{
    IMGUI_REGISTER_WINDOW("Scene Manager", EDIT, SceneSystem::DrawImGuiWindow);
}

SceneSystem::~SceneSystem()
{}

void SceneSystem::RegisterScene(const std::string& sceneName, std::unique_ptr<SceneLoader>&& createScene)
{
    if (scenes.contains(sceneName))
        DebugPrint("Scene {} already exists and will be overwritten", sceneName);
    
    scenes[sceneName] = { std::move(createScene) };
}

void SceneSystem::RegisterScene(const std::string& sceneName, Scene&& scene)
{
    scene.sceneSettings.worldName = sceneName;
    if (scenes.contains(sceneName))
        DebugPrint("Scene {} already exists and will be overwritten", sceneName);
    
    scenes[sceneName] = { std::make_unique<SceneFileLoader>(), std::move(scene) };
}

void SceneSystem::Unregister(const std::string& sceneName)
{
    scenesToRemove.push_back(sceneName);
}

void SceneSystem::LoadSceneOnNextFrame(const SceneIdentifier& sceneIdentifier)
{
    sceneToLoadOnNextFrame = sceneIdentifier;
}

void SceneSystem::LoadSceneImmediate(const SceneIdentifier& sceneIdentifier, EntityComponentSystem& ecs)
{
    LoadSceneOrDefault(sceneIdentifier, ecs);
}

void SceneSystem::SaveScene(const std::string& sceneName)
{
    if (!scenes.contains(sceneName))
        return;
    
    SaveScene(sceneName, scenes[sceneName]);
}

void SceneSystem::SaveScene(const std::string& sceneName, const Scene& sceneToSave)
{
    if (!scenes.contains(sceneName))
        return;

    SceneLoad& load = scenes[sceneName];
    
    Assert(load.loader.get());

    load.sceneData = sceneToSave;
    
    SaveScene(sceneName, load);
}

void SceneSystem::SaveScene(const std::string& sceneName, const SceneLoad& load) const
{
    std::optional<std::filesystem::path> exportLocation = load.loader->GetSaveFilePath(load.sceneData->sceneSettings.worldName);
    if (!exportLocation)
        return;
    
    std::ofstream jsonFile{exportLocation->has_filename() ? *exportLocation : *exportLocation / std::format("{}.json", sceneName)};
    jsonFile << SceneSerializer::SerializeToJson(*load.sceneData);
}

void SceneSystem::Init()
{
    System::Init();

#ifdef IMGUI_ENABLED
    ecs = &GetEcs();
#endif
}

void SceneSystem::Update(double delta)
{
    System::Update(delta);
    auto& ecs = GetEcs();

    if (!scenesToRemove.empty())
    {
        for (const auto& scene : scenesToRemove)
        {
            scenes.erase(scene);
        }
        scenesToRemove.clear();
    }

    if (sceneToLoadOnNextFrame)
    {
        LoadSceneImmediate(*sceneToLoadOnNextFrame, ecs);
        sceneToLoadOnNextFrame.reset();
    }
}

#ifdef IMGUI_ENABLED

import ImGui;

void SceneSystem::DrawImGuiWindow(bool* opened)
{
    if (ImGui::Begin("Scene Manager", opened))
    {
        ScytheGui::InfoBubble("Green means loaded in memory. Like a cache.");
        ImGui::SameLine();
        if (ImGui::CollapsingHeader("Saved Scenes"))
        {
            for (const auto& [name, loadData] : scenes)
            {
                std::optional<ImVec4> buttonColor;
                if (loadData.sceneData)
                    buttonColor = ImVec4{ 0, 0.8, 0, 1 };
                
                if (buttonColor)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, *buttonColor);
                }
                
                if (ImGui::Button(name.c_str()))
                {
                    LoadSceneOnNextFrame(name);
                }

                if (buttonColor)
                {
                    ImGui::PopStyleColor();    
                }

                ImGui::SameLine();

                ImGui::PushID(name.c_str());

                bool openPopup = ImGui::Button(ICON_FA_TRASH_CAN);
                if (ScytheGui::BeginModal("Unregister Scene", openPopup))
                {
                    ImGui::Text("This will unregister the scene from the SceneManager.\nDo you wish to proceed?");
                    
                    if (auto result = ScytheGui::EndModal(ScytheGui::ModalType::YES_NO))
                    {
                        if (result == ScytheGui::YES)
                            Unregister(name);
                    }
                }
                ImGui::PopID();
            }    
        }
        
        ImGui::Separator();

        static std::string newSceneName;
        ImGui::InputText("Saved Scene Name", &newSceneName);
        
        if (ImGui::Button("Create Copy"))
        {
            if (!newSceneName.empty())
            {
                RegisterScene(newSceneName, ecs->SaveToScene());
                newSceneName.clear();
            }
        }
    }
    ImGui::End();
}

#endif