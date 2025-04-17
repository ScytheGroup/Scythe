module;
#include "Common/Macros.hpp"
#include "ImGui/Icons/IconsFontAwesome6.h"
#include <AsyncInfo.h>
module Editor;
#ifdef EDITOR

import Graphics;
import Systems.Physics;
import Common;
import Core;
import Components;
import ImGui;
#ifdef PROFILING
import Profiling;
#endif
import Generated;
import Systems;

namespace Editor_Private
{
    static Scene pieScene;
    static Inputs::MouseMode gameMouseMode = Inputs::MouseMode::MODE_RELATIVE;
}

void SelectionState::UpdateState()
{
    auto keyboardState = InputSystem::Get().GetKeyboardTracker();

    if (keyboardState.IsKeyPressed(Inputs::Keys::W))
        currentOperation = ImGuizmo::TRANSLATE;
    if (keyboardState.IsKeyPressed(Inputs::Keys::E))
        currentOperation = ImGuizmo::ROTATE;
    if (keyboardState.IsKeyPressed(Inputs::Keys::R))
        currentOperation = ImGuizmo::SCALE;
    if (keyboardState.IsKeyPressed(Inputs::Keys::T))
        currentOperation = ImGuizmo::UNIVERSAL;
    if (keyboardState.IsKeyPressed(Inputs::Keys::G))
        snapToGrid = !snapToGrid;
    if (keyboardState.IsKeyPressed(Inputs::Keys::Q))
    {
        if (currentOperation != ImGuizmo::SCALE)
        {
            if (currentMode == ImGuizmo::LOCAL)
                currentMode = ImGuizmo::WORLD;
            else if (currentMode == ImGuizmo::WORLD)
                currentMode = ImGuizmo::LOCAL;
        }
    }
}

Editor::Editor(EntityComponentSystem& ecs)
    : ecs(ecs)
    , imGuiManager{ ImGuiManager::Get() }
    , editorCameraEntity(ecs.AddEntity())
{
    InitIcons();
    
    InjectNewCameraEditor();
    
    IMGUI_REGISTER_WINDOW_OPEN("Entity Details", SCENE, Editor::DrawEntityDetails);

    IMGUI_REGISTER_WINDOW_LAMBDA("Import Asset", FILE, [&](bool* opened)
    {
        if (*opened)
        {
            ImportAsset();
            *opened = false;
        }
    });
    
    IMGUI_REGISTER_WINDOW_LAMBDA("New Scene", FILE, [&](bool* opened)
    {
        static std::string error = "";

        auto close = [&]
        {
            *opened = false;
            error = "";
        };
        
        if (*opened)
        {
            if (ScytheGui::BeginModal("New Scene", opened))
            {
                static std::string newSceneName = "";
                ImGui::InputText("##", &newSceneName);

                if (!error.empty())
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, 0xFF0000FF);
                    ImGui::Text(error.c_str());
                    ImGui::PopStyleColor();
                }

                if (auto result = ScytheGui::EndModal(ScytheGui::ModalType::CONFIRM_CANCEL); result != ScytheGui::NONE)
                {
                    if (result == ScytheGui::CONFIRM)
                    {
                        if (newSceneName.empty())
                        {
                            error = "Scene name cannot be empty";
                        }
                        else
                        {
                            SceneSystem& sceneSystem = ecs.GetSystem<SceneSystem>();
                            SaveCurrentScene();
                            sceneSystem.RegisterScene(newSceneName, Scene{});
                            sceneSystem.LoadSceneOnNextFrame(newSceneName);
                            needToSaveScene = true;
                            close();
                        }
                    }
                    else
                    {
                        close();
                    }
                }
            }
        }
    });

    IMGUI_REGISTER_WINDOW_LAMBDA("Open Scene", FILE, [&](bool* opened)
    {
        // wierd shenanigans until we have actions on the menu bar
        if (*opened)
        {
            if (auto scenePath = FileBrowser::BrowseForFile(ProjectSettings::Get().projectRoot / "Resources"))
            {
                ecs.GetSystem<SceneSystem>().LoadSceneOnNextFrame(*scenePath);    
            }
            else
            {
                DebugPrint("Couldn't load scene!");
            }
            *opened = false;
        }
    });

    IMGUI_REGISTER_WINDOW_LAMBDA("Save Scene", FILE, [&](bool* opened)
    {
       if (*opened)
       {
           SaveCurrentScene();
           *opened = false;
       }
    });

    IMGUI_REGISTER_WINDOW_LAMBDA("Exit", FILE,
        [&](bool* opened)
    {
        // wierd shenanigans until we have actions on the menu bar
        if (*opened)
        {
            OnExit();

            // TODO: Replace this with proper exit code
            std::exit(0);
        }
    });

    ecs.sceneLoadedDelegate.Subscribe(BindMethod(&Editor::OnNewSceneLoaded, this));
    InputSystem::SetMouseModeAbsolute();

    assetBrowser.ReloadAssets();
}

void Editor::BeginDrawImGui()
{
    if (firstFrame)
    {
        Editor_Private::gameMouseMode = InputSystem::GetMouseMode();
        InputSystem::SetMouseModeAbsolute();
        firstFrame = false;
    }

    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    UpdateEditorFocus(Other);
}

void Editor::DrawEditorWindows()
{
    DrawSceneGraphWindow();

    for (auto&& editorCallback : editorCallbacks)
        editorCallback();
}

void Editor::EndDrawImGui()
{
    UpdateEditorControls();
}

void Editor::InitIcons()
{
    EditorIcons::CameraIcon = std::make_unique<Texture>(Texture::TextureData{ 
        ProjectSettings::Get().engineRoot / "Resources\\Editor\\Icons\\CameraIcon.png", Texture::TEXTURE_2D, Texture::DEFAULT, Texture::TEXTURE_SRV | Texture::TEXTURE_RTV, false
    });
    
    EditorIcons::PointLightIcon = std::make_unique<Texture>(Texture::TextureData{ 
        ProjectSettings::Get().engineRoot / "Resources\\Editor\\Icons\\PointLightIcon.png", Texture::TEXTURE_2D, Texture::DEFAULT, Texture::TEXTURE_SRV | Texture::TEXTURE_RTV, false 
    });
    
    EditorIcons::SpotLightIcon = std::make_unique<Texture>(Texture::TextureData{ 
        ProjectSettings::Get().engineRoot / "Resources\\Editor\\Icons\\SpotLightIcon.png", Texture::TEXTURE_2D, Texture::DEFAULT, Texture::TEXTURE_SRV | Texture::TEXTURE_RTV, false 
    });
    
    EditorIcons::AssetBrowserMeshIcon = std::make_unique<Texture>(Texture::TextureData{
        ProjectSettings::Get().engineRoot / "Resources\\Editor\\Icons\\MeshIcon.png", Texture::TEXTURE_2D, Texture::DEFAULT, Texture::TEXTURE_SRV | Texture::TEXTURE_RTV, false
    });

    EditorIcons::AssetBrowserMaterialsIcon = std::make_unique<Texture>(Texture::TextureData{
        ProjectSettings::Get().engineRoot / "Resources\\Editor\\Icons\\MaterialIcon.png", Texture::TEXTURE_2D, Texture::DEFAULT, Texture::TEXTURE_SRV | Texture::TEXTURE_RTV, false
    });

    EditorIcons::AssetBrowserTextureIcon = std::make_unique<Texture>(Texture::TextureData{
        ProjectSettings::Get().engineRoot / "Resources\\Editor\\Icons\\TextureIcon.png", Texture::TEXTURE_2D, Texture::DEFAULT, Texture::TEXTURE_SRV | Texture::TEXTURE_RTV, false
    });

    EditorIcons::CameraIcon->RegisterLazyLoadedResource(true);
    EditorIcons::PointLightIcon->RegisterLazyLoadedResource(true);
    EditorIcons::SpotLightIcon->RegisterLazyLoadedResource(true);
    EditorIcons::AssetBrowserMeshIcon->RegisterLazyLoadedResource(true);
    EditorIcons::AssetBrowserMaterialsIcon->RegisterLazyLoadedResource(true);
    EditorIcons::AssetBrowserTextureIcon->RegisterLazyLoadedResource(true);
}

void Editor::Subscribe(const std::function<void()>& sub)
{
    editorCallbacks.push_back(sub);
}

void Editor::HandleDeferredAction(const EntityEditorAction& action)
{
    switch (action.type)
    {
    case EntityEditorAction::DUPLICATE_ENTITY: selection.selectedEntity = DuplicateEntity(action.entity); break;
    case EntityEditorAction::REMOVE_ENTITY:
        if (action.entity)
        {
            RemoveEntity(action.entity); 
        }
        break;
    case EntityEditorAction::CREATE_CHILD_ENTITY: selection.selectedEntity = CreateChildEntity(action.entity); break;
    case EntityEditorAction::PASTE_ENTITY:
        if (copiedEntity)
            selection.selectedEntity = PasteEntityTo(*copiedEntity, action.entity);
        break;
    }
}

void Editor::HandleDeferredActions()
{
    for (const auto& action : deferredEditorActions)
        HandleDeferredAction(action);
    deferredEditorActions.clear();
}

void Editor::DrawSceneGraphWindow()
{
#if 0 // For later reference
    bool b = true;
    ImGui::ShowDemoWindow(&b);
#endif
    if (ImGui::Begin("Entities"))
    {
        {
            bool addEntity = ImGui::Button(ICON_FA_PLUS);
            ScytheGui::TextTooltip("Add an entity to the selected entity or scene if none is selected", true);
            if (addEntity)
            {
                deferredEditorActions.emplace_back(EntityEditorAction::CREATE_CHILD_ENTITY, selection.selectedEntity ? *selection.selectedEntity : Handle{});
            }

            ImGui::SameLine();

            ImGui::BeginDisabled(!selection.selectedEntity.has_value());
            bool removeEntity = ImGui::Button(ICON_FA_TRASH_CAN);
            ScytheGui::TextTooltip("Remove selected entity and it's children from the scene (Del)", true);
            if (removeEntity)
            {
                deferredEditorActions.emplace_back(EntityEditorAction::REMOVE_ENTITY, *selection.selectedEntity);
                selection.selectedEntity.reset();
            }
            ImGui::EndDisabled();

            ImGui::SameLine();

            ImGui::BeginDisabled(!selection.selectedEntity.has_value());
            bool duplicateEntity = ImGui::Button(ICON_FA_CLONE);
            ScytheGui::TextTooltip("Duplicates the selected entity (Ctrl + D)", true);
            if (duplicateEntity)
            {
                deferredEditorActions.emplace_back(EntityEditorAction::DUPLICATE_ENTITY, *selection.selectedEntity);
            }
            ImGui::EndDisabled();

            ImGui::SameLine();
            ImGui::Text("|");

            ImGui::SameLine();

            ImGui::BeginDisabled(!selection.selectedEntity.has_value());
            bool copyEntity = ImGui::Button(ICON_FA_COPY);
            ScytheGui::TextTooltip("Copies the selected entity (Ctrl + C)", true);
            if (copyEntity)
            {
                copiedEntity = *selection.selectedEntity;
            }
            ImGui::EndDisabled();

            ImGui::SameLine();

            ImGui::BeginDisabled(!selection.selectedEntity.has_value() || !copiedEntity.has_value());
            bool pasteEntity = ImGui::Button(ICON_FA_PASTE);
            ScytheGui::TextTooltip("Pastes the earlier copied entity below the selected one (Ctrl + V)", true);
            if (pasteEntity)
            {
                deferredEditorActions.emplace_back(EntityEditorAction::PASTE_ENTITY, *selection.selectedEntity);
            }
            ImGui::EndDisabled();

            ImGui::SameLine();
            ImGui::Text("|");

            ImGui::SameLine();
        }
        
        bool isSimulationActive = ecs.IsSimulationActive();

        ImGui::PushStyleColor(ImGuiCol_Text, !isSimulationActive ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        if (ImGui::Button(isSimulationActive ? ICON_FA_STOP : ICON_FA_PLAY))
        {
            if (isSimulationActive)
                StopPlayInEditor();
            else
                StartPlayInEditor();
        }
        ImGui::PopStyleColor();
        ScytheGui::TextTooltip(!isSimulationActive ? "Starts play in editor (Alt + P)" : "Stops play in editor (Alt + P or Esc)", true);

        ImGui::SameLine();
        ImGui::BeginDisabled(!isSimulationActive);
        if (ImGui::Button(ICON_FA_EJECT))
        {
            EjectCamera();
        }
        ImGui::EndDisabled();
        ScytheGui::TextTooltip("Detaches the camera from the game's camera (F8). Press again to reattach.", true); 

        if (ImGui::BeginChild("Entities##"))
        {
            if (ImGui::IsWindowFocused())
            {
                UpdateEditorFocus(Entities);
            }
            
            std::string name = "Scene";
            if (needToSaveScene)
                name += ICON_FA_ASTERISK;
        
            bool treeNodeOpened = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow);

            if (!ImGui::IsDragDropActive() && ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            {
                // force scene settings here
                selection.selectedEntity.reset();
            }
            
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Entity"))
                {
                    Handle droppedEntity = *static_cast<const Handle*>(payload->Data);
                    ecs.Reparent(droppedEntity, {});

                    needToSaveScene = true;
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::PushID("SceneRoot");
            DrawEntityContextMenu({});
            ImGui::PopID();
            
            if (treeNodeOpened)
            {
                for (auto it = ecs.GetEntities().begin(); it != ecs.GetEntities().end(); ++it)
                {            
                    // if not root parent continue 
                    Entity& entity = *it;
                    auto parentComp = entity.find(Type::GetType<ParentComponent>());
                    if (parentComp != entity.end() && parentComp->second->Cast<ParentComponent>()->parent != Handle{})
                        continue;

                    DrawEntityTreeNode(it.AbsoluteIndex());
                }
            
                ImGui::TreePop();
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void Editor::DrawEntityTreeNode(Handle entity)
{
    Entity& entityRef = ecs.GetEntity(entity);
    std::string name = std::format("Entity {}", entity.id);
    
    // skip editor camera
    if (entityRef.contains(Type::GetType<EditorCameraComponent>()))
        return;

    ImGui::PushID(entity.id);
    
    EntityInfo& props = ecs.GetEntityInfo(entity);
    if (!props.name.empty())
        name = props.name;

    int treeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow;
    
    if (selection.selectedEntity.has_value() && entity == *selection.selectedEntity)
    {
        treeFlags |= ImGuiTreeNodeFlags_Selected;
    }
    if (dirtySet.contains(entity))
    {
        name += ICON_FA_ASTERISK;
    }

    bool isTreeOpened = ImGui::TreeNodeEx(name.c_str(), treeFlags, name.c_str());

    if (!ImGui::IsDragDropActive() && ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        selection.selectedEntity.emplace(entity);
    }

    if (ImGui::BeginDragDropTarget())
    {        
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Entity"))
        {            
            Handle droppedEntity = *static_cast<const Handle*>(payload->Data);
            if (droppedEntity != entity)
            {
                ecs.Reparent(droppedEntity, entity);

                needToSaveScene = true;
            }
        }
        ImGui::EndDragDropTarget();
    }
    
    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("Entity", &entity, sizeof(Handle));
        ImGui::Text(name.c_str());
        ImGui::EndDragDropSource();
    }

    DrawEntityContextMenu(entity);
    
    if (isTreeOpened)
    {
        if (entityRef.contains(Type::GetType<ChildrenComponent>()))
        {
            ChildrenComponent* children = ecs.GetComponent<ChildrenComponent>(entityRef);
            for (Handle child : children->children)
            {
                DrawEntityTreeNode(child);
            }
        }
        
        ImGui::TreePop();
    }

   ImGui::PopID();
}

bool IsHiddenComponent(Component* component)
{
    return component->Cast<ParentComponent>()
        || component->Cast<ChildrenComponent>();
}

void Editor::DrawEntityDetails(bool* opened)
{
    if (ImGui::Begin("Entity Details", opened))
    {
        if (ImGui::IsWindowFocused())
        {
            UpdateEditorFocus(Details);
        }
        
        if (selection.selectedEntity.has_value())
        {
            bool isDirty = false;
            // Name
            EntityInfo& info = ecs.GetEntityInfo(*selection.selectedEntity);
            isDirty |= ImGui::InputText("##Name", &info.name);
            
            Entity& entity = ecs.GetEntity(*selection.selectedEntity);

            isDirty |= ComponentModal();
            static bool showHidden = false;
            ImGui::Checkbox("show hidden", &showHidden);
            ScytheGui::TextTooltip("Show hidden components. This is mostly for debugging engine components.");

            if (showHidden)
            {
                // show handle
                ImGui::Text("Handle: %d", selection.selectedEntity->id);
            }

            ImGui::Separator();
            
            if (ImGui::BeginChild("Components##"))
            {
                std::vector<Ref<Component>> compsToRemove;

                Array<Ref<Component>> components = entity | std::views::values | std::ranges::to<std::vector>();
                std::ranges::sort(components, [](const auto& a, const auto& b) {
                    return std::string_view(a->GetTypeName().data()) < std::string_view(b->GetTypeName().data());
                });
                
                int i = 0;
                for (Ref<Component>& component : components)
                {
                    if (!showHidden && IsHiddenComponent(component.Get()))
                    {
                        continue;
                    }
                    
                    ImGui::PushID(i++);

                    bool removeComponent = ImGui::Button(ICON_FA_TRASH_CAN);
                    ScytheGui::TextTooltip("Remove this component from this entity");

                    ImGui::SameLine();
                    ScytheGui::Header(component->GetTypeName().data());

                    if (removeComponent)
                    {
                        compsToRemove.push_back(component);
                    }
                    else
                    {
                        bool componentDirty = component->DrawEditorInfo();
                        
                        if (componentDirty)
                        {
                            ecs.NotifyComponentChanged(*component);
                        }
                        
                        isDirty |= componentDirty;
                    }
                    ImGui::PopID();
                    ImGui::Separator();
                }

                for (auto& comp : compsToRemove)
                    ecs.RemoveComponent(comp);

                isDirty |= !compsToRemove.empty(); 
            }
            ImGui::EndChild();

            if (isDirty)
            {
                dirtySet.insert(*selection.selectedEntity);
            }
        }
        else
        {
            needToSaveScene |= ecs.GetSceneSettings().ImGuiDrawContent();
        }
    }
    ImGui::End();
}

void Editor::DrawEntityContextMenu(Handle entity)
{
    if (ScytheGui::BeginContextMenu("EntityContextMenu"))
    {
        if (ScytheGui::ContextMenuButton("Create Child"))
        {
            deferredEditorActions.emplace_back(EntityEditorAction::CREATE_CHILD_ENTITY, entity);
        }

        if (entity.IsValid())
        {
            if (ScytheGui::ContextMenuButton("Duplicate"))
            {
                deferredEditorActions.emplace_back(EntityEditorAction::DUPLICATE_ENTITY, entity);
            }

            if (ScytheGui::ContextMenuButton("Remove"))
            {
                deferredEditorActions.emplace_back(EntityEditorAction::REMOVE_ENTITY, entity);
            }

            if (ScytheGui::ContextMenuButton("Copy"))
            {
                copiedEntity = entity;
            }

            ImGui::BeginDisabled(!copiedEntity.has_value());
            if (ScytheGui::ContextMenuButton("Paste"))
            {
                deferredEditorActions.emplace_back(EntityEditorAction::PASTE_ENTITY, entity);
            }
            ImGui::EndDisabled();
        }
        
        ScytheGui::EndContextMenu();
    }
}

void Editor::StartPlayInEditor()
{
    // Save in playinEditor
    // ecs.GetSystem<SceneSystem>().SaveScene("Play in scythe editor"s, ecs.SaveToScene());
    Editor_Private::pieScene = ecs.SaveToScene();
    
    ecs.SetSimulationActive(true);
    // Save for reset
    editorCamSavedTransform = ecs.GetComponent<TransformComponent>(editorCameraEntity)->transform;

    selection.selectedEntity = {};

    ecs.LoadScene(Editor_Private::pieScene);

    InjectNewCameraEditor();
    
    ecs.GetSystem<CameraSystem>().SetFirstActiveCameraAsMain<CameraComponent>();
    InputSystem::SetMouseMode(Editor_Private::gameMouseMode);
pieMouseOverriden = false;
    EditorDelegates::PlayInEditorDelegate.Execute(true);
}

void Editor::StopPlayInEditor()
{
    ecs.SetSimulationActive(false);

    EditorDelegates::PlayInEditorDelegate.Execute(false);
    // Reload to the previous scene
    ecs.LoadScene(Editor_Private::pieScene);

    InjectNewCameraEditor();
    InputSystem::SetMouseModeAbsolute();
}

bool Editor::ComponentModal()
{
    static Array hiddenComponents = {
        Type::GetType<ParentComponent>(),
        Type::GetType<ChildrenComponent>(),
        Type::GetType<EditorCameraComponent>(),
    };
    static Array<Type> componentList = [this]() {
        const Array<Type>& types = ecs.GetRegisteredComponentList();
        Array<Type> finalTypes;
        
        finalTypes.reserve(types.size() - hiddenComponents.size());
        std::ranges::copy_if(types, std::back_inserter(finalTypes), [](Type t) {
            return std::ranges::find(hiddenComponents, t) == hiddenComponents.end(); 
        });

        std::sort(componentList.begin(), componentList.end(), [](const Type& a, const Type& b) {
            return a.GetName() < b.GetName(); 
        });

        return finalTypes;
    }();
    static std::vector<uint32_t> chosenComponents(componentList.size());
    static std::string searchBuffer = "";

    bool openModal = ImGui::Button("Add Component");
    ScytheGui::TextTooltip("Adds new components to this entity");
    if (ScytheGui::BeginModal("Add Component", openModal))
    {
        auto currentEntity = ecs.GetEntity(*selection.selectedEntity); 

        // Search bar
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##search", ICON_FA_MAGNIFYING_GLASS " Search Components", &searchBuffer);

        // Create a list of components that can be added
        if (ImGui::BeginListBox("##components", ImVec2(-1, 300)))
        {
            auto ToLowercase = [](const std::string& s) -> std::string
            {
                std::string copy = s;
                for (auto& c : copy)
                {
                    c = std::tolower(c);
                }
                return copy;
            };
            std::string searchStr = ToLowercase(searchBuffer);
            for (size_t i = 0; i < componentList.size(); ++i)
            {
                if (currentEntity.contains(componentList[i]))
                    continue;

                std::string componentName = ToLowercase(std::string(componentList[i].GetName().data()));
                // Skip if doesn't match search
                if (!searchStr.empty() && componentName.find(searchStr) == std::string::npos) continue;

                if (ImGui::Selectable(componentList[i].GetName().data(), static_cast<bool>(chosenComponents[i])))
                { 
                    chosenComponents[i] = (chosenComponents[i] + 1) % 2;
                }
            }
            ImGui::EndListBox();
        }
        
        static bool addRequirements = true;

        // Show summary of selected components
        if (std::any_of(chosenComponents.begin(), chosenComponents.end(), [](auto v) { return v; }))
        {
            ImGui::TextUnformatted("Selected:");
            ImGui::Indent();
            for (size_t i = 0; i < chosenComponents.size(); ++i)
            {
                if (!chosenComponents[i]) continue;
                ImGui::BulletText("%s", componentList[i].GetName().data());
                if (addRequirements)
                {
                    ImGui::Indent();
                    if (auto it = ECS::GetRequirements().find(componentList[i]); it != ECS::GetRequirements().end())
                    {
                        for (Type requiredType : it->second)
                        {
                            ImGui::BulletText("%s", requiredType.GetName().data());
                        }
                    }
                    ImGui::Unindent();
                }
            }
            ImGui::Unindent();
        }

        ImGui::Spacing();

        ImGui::Checkbox("Add Required Components", &addRequirements);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::SetTooltip("Each type of component has a set of required components.\nThis option determines if you should also add the required components (and their required components) of the components you add.");
        }

        if (auto result = ScytheGui::EndModal(ScytheGui::ModalType::OK_CANCEL); result != ScytheGui::NONE)
        {
            if (result == ScytheGui::PopupStatus::OK)
            {
                for (int i = 0; i < chosenComponents.size(); ++i)
                {
                    if (!chosenComponents[i]) continue;

                    uint32_t componentIndex = i;
                    addRequirements ? ecs.AddComponentAndRequirements(componentList[componentIndex], *selection.selectedEntity)
                                    : ecs.AddComponent(componentList[componentIndex], *selection.selectedEntity);
                }
            }

            // Upon exiting, reset the selection...
            chosenComponents = std::vector<uint32_t>(componentList.size());
            // ..., the requirements checkbox...
            addRequirements = true;
            // ...and the search field.
            searchBuffer.clear();

            return result == ScytheGui::PopupStatus::OK;
        }
    }

    return false;
}

void Editor::EjectCamera()
{
    if (!ecs.IsSimulationActive())
        return;
    
    // Set active camera as the editor or game
    CameraComponent* cameraComp = ecs.GetSystem<CameraSystem>().GetActiveCamera();
    Entity& cameraEntity = Editor::ecs.GetEntity(cameraComp->GetOwningEntity());
            
    if (cameraEntity.contains(Type::GetType<EditorCameraComponent>()))
    {
        auto camComps = ecs.GetArchetype<CameraComponent>();
        for (auto [cam] : camComps)
        {
            if (cam->isActiveCamera)
            {
                ecs.GetSystem<CameraSystem>().SetMainCamera(cam);
                break;
            }
        }
        InputSystem::SetMouseModeRelative();
    }
    else
    {
        ecs.GetSystem<CameraSystem>().SetMainCamera(Editor::ecs.GetComponent<EditorCameraComponent>(editorCameraEntity));
        InputSystem::SetMouseModeAbsolute();
        pieMouseOverriden = false;
    }
}

void Editor::UpdateEditorFocus(EditorWindows window)
{
    if (window == currentFocus)
        return;
    
    switch (window)
    {
    case Entities:
        break;
    case Details:
        break;
    case Viewport:
        if (ecs.isSimulationActive)
        {
            if (InputSystem::GetMouseMode() != Editor_Private::gameMouseMode)
               InputSystem::SetMouseMode(Editor_Private::gameMouseMode);
        }
        break;
    case Other:
        break;
    case Count:
        break;
    default: ;
    }

    currentFocus = window;
}

void Editor::UpdateEditorControls()
{
    using namespace Inputs;

    // Common Editor controls

    // Normal editor behaviour
    if (!ecs.IsSimulationActive())
    {        
        // control s to save scene
        if (InputSystem::IsKeyPressed(Keys::S) && InputSystem::IsKeyHeld(Keys::LeftControl))
        {
            SaveCurrentScene();
        }

        // Launching Pie
        if (InputSystem::IsKeyPressed(Keys::P) && InputSystem::IsKeyHeld(Keys::LeftAlt))
        {
            StartPlayInEditor();
        }

        if (currentFocus == Entities || currentFocus == Viewport)
        {
            if (InputSystem::IsKeyPressed(Keys::Delete))
            {
                if (selection.selectedEntity)
                {
                    RemoveEntity(*selection.selectedEntity);
                }
            }

            if (InputSystem::IsKeyPressed(Keys::D) && InputSystem::IsKeyHeld(Keys::LeftControl))
            {
                if (selection.selectedEntity)
                {
                    selection.selectedEntity = DuplicateEntity(*selection.selectedEntity);    
                }
            }

            if (InputSystem::IsKeyPressed(Keys::C) && InputSystem::IsKeyHeld(Keys::LeftControl))
            {
                if (selection.selectedEntity)
                {
                    copiedEntity = selection.selectedEntity;    
                }
            }

            if (InputSystem::IsKeyPressed(Keys::V) && InputSystem::IsKeyHeld(Keys::LeftControl))
            {
                if (copiedEntity)
                {
                    PasteEntityTo(*copiedEntity, selection.selectedEntity ? *selection.selectedEntity : Handle{}); 
                }
            }
        }
    }
    // Game behaviour and limited editor functions 
    else
    {
        // Stopping pie
        if (InputSystem::IsKeyPressed(Keys::P) && InputSystem::IsKeyHeld(Keys::LeftAlt)
            || InputSystem::IsKeyPressed(Keys::Escape))
        {
            StopPlayInEditor();
        }
        
        if (InputSystem::IsKeyPressed(Keys::F8))
        {
            EjectCamera();
        }

        if (InputSystem::IsKeyPressed(Keys::F9))
        {
            if (pieMouseOverriden)
            {
                InputSystem::SetMouseMode(Editor_Private::gameMouseMode);
                pieMouseOverriden = false;
            }
            else
            {
                InputSystem::SetMouseModeAbsolute();
                pieMouseOverriden = true;
            }
        }
    }
}

void Editor::SaveCurrentScene()
{
    if (ecs.IsSimulationActive())
    {
        DebugPrint("Cannot save scene while PIE is ongoing");
        return;
    }

    ResourceManager::Get().SaveDirtyAssets(ecs);
    Scene sceneToSave = ecs.SaveToScene();
    ecs.GetSystem<SceneSystem>().SaveScene(editorSceneName, sceneToSave);
    EditorDelegates::OnSceneSaved.Execute(sceneToSave);
    dirtySet.clear();
    needToSaveScene = false;
}

void Editor::ReloadCurrentScene() 
{
    ecs.GetSystem<SceneSystem>().LoadSceneOnNextFrame(editorSceneName);
}

void Editor::ImportAsset()
{
    std::optional<std::filesystem::path> file = FileBrowser::BrowseForFile();

    if (file)
        ResourceManager::Get().Import(*file);
}

void DuplicateEntityTo(EntityComponentSystem& ecs, Handle fromEntity, Handle toEntity)
{
    auto fromEntityComponents = ecs.GetEntity(fromEntity);
    
    for (auto [type, fromComp] : fromEntityComponents)
    {
        // Make the copy be on the root of the scene by default
        if (type == Type::GetType<ParentComponent>())
        {
            continue;
        }
        
        if (auto fromChildComp = fromComp->Cast<ChildrenComponent>())
        {
            for (auto fromChild : fromChildComp->children)
            {
                Handle toChild = ecs.AddEntity();
                
                ecs.Reparent(toChild, toEntity, false);
                
                DuplicateEntityTo(ecs, fromChild, toChild);

                EntityInfo& toInfo = ecs.GetEntityInfo(toChild);
                EntityInfo& fromInfo = ecs.GetEntityInfo(fromChild);
                toInfo.name = fromInfo.name;
            }
        }
        else
        {
            Ref toComp = fromComp->Copy<Component>().release();
            ecs.EmplaceComponent(type, toEntity, toComp);
        }
    }
}

Handle Editor::CreateDuplicate(Handle source)
{
    Handle toEntity = ecs.AddEntity();

    EntityInfo& toInfo = ecs.GetEntityInfo(toEntity);
    EntityInfo& fromInfo = ecs.GetEntityInfo(source);
    toInfo.name = std::format("{} - Copy", fromInfo.name); 
    
    DuplicateEntityTo(ecs, source, toEntity);

    return toEntity;
}

Handle Editor::PasteEntityTo(Handle fromEntity, Handle newParentEntity)
{
    if (!ecs.IsValidEntity(fromEntity))
        return {};
    
    Handle toEntity = CreateDuplicate(fromEntity);

    if (newParentEntity.IsValid())
    {
        ecs.Reparent(toEntity, newParentEntity);
    }
    
    needToSaveScene = true;

    return toEntity;
}

Handle Editor::DuplicateEntity(Handle fromEntity)
{
    Handle parentEntity;
    if (ParentComponent* parentComponent = ecs.GetComponent<ParentComponent>(fromEntity); parentComponent)
    {
        parentEntity = parentComponent->parent;
    }
    return PasteEntityTo(fromEntity, parentEntity);
}

void Editor::RemoveEntity(Handle entity)
{
    if (copiedEntity && *copiedEntity == entity)
        copiedEntity = {};

    if (selection.selectedEntity.has_value()
        && selection.selectedEntity.value() == entity)
        selection.selectedEntity.reset();
    ecs.RemoveEntity(entity);
    needToSaveScene = true;
}

Handle Editor::CreateChildEntity(Handle parent)
{
    Handle handle = ecs.AddEntity();
    if (parent.IsValid())
    {
        ecs.Reparent(handle, parent);
    }
    needToSaveScene = true;
    return handle;
}

void Editor::OnNewSceneLoaded(const std::string& sceneName)
{
    // if playin in editor do nothing
    if (ecs.IsSimulationActive())
        return;
    
    editorSceneName = sceneName;
    selection = {};

    // reinject the editor camera
    InjectNewCameraEditor();

    copiedEntity = {};
}

void Editor::InjectNewCameraEditor()
{
    // Assert there isn't an existing one
    if (!ecs.GetArchetype<EditorCameraComponent>().empty())
    {
        DebugPrint("Editor camera already exists");
        return;
    }

    editorCameraEntity =  ecs.AddEntity({"EditorCamera", Guid::Create()});
    ecs.AddComponentAndRequirements<TransformComponent>(editorCameraEntity);
    ecs.GetComponent<TransformComponent>(editorCameraEntity)->transform = editorCamSavedTransform;
    auto cameraComp = ecs.AddComponent<EditorCameraComponent>(editorCameraEntity);
    cameraComp->isActiveCamera = true;
    ecs.GetSystem<CameraSystem>().SetMainCamera(cameraComp, ecs);
}

void Editor::DrawEditorWidget(const std::string& panelName, const std::function<void(Editor&)>& func)
{
    ImGui::Begin(panelName.c_str());
    func(*this);
    ImGui::End();
}

void Editor::Update(double dt)
{
    UpdateEditorCamera(dt);

    HandleDeferredActions();
}

void Editor::UpdateSelection()
{
    Graphics& graphics = ecs.GetSystem<Graphics>();

    if (selection.selectedEntity && DirectX::Keyboard::Get().GetState().Escape)
        selection.selectedEntity.reset();

    if (!isViewportHovered || ecs.IsSimulationActive())
        return;

    auto mouseState = InputSystem::Get().GetMouseState();
    auto tracker = InputSystem::Get().GetMouseTracker();

    const ImVec2 mousePos{
        static_cast<float>(mouseState.x),
        static_cast<float>(mouseState.y),
    };

    if (viewportRect.Contains(mousePos) && tracker.leftButton == DirectX::Mouse::ButtonStateTracker::RELEASED)
    {
        Vector2 relativeMousePos(mouseState.x - viewportRect.Min.x, mouseState.y - viewportRect.Min.y);
        if (!selection.selectedEntity || (selection.selectedEntity && !ImGuizmo::IsOver()))
        {
            if (std::optional<Handle> objectHandle = graphics.GetSelectedEntityFromScreen(relativeMousePos, ecs))
            {
                selection.selectedEntity.emplace(*objectHandle);
            }
            else
            {
                selection.selectedEntity.reset();
            }
        }
    }
}

void Editor::DrawSelectionWidget()
{
    auto mouseState = InputSystem::Get().GetMouseState();
    if (selection.selectedEntity)
    {
        if (!mouseState.rightButton && currentFocus == Viewport)
            selection.UpdateState();
        
        auto* worldTransform = ecs.GetComponent<GlobalTransform>(*selection.selectedEntity);
        CameraComponent* cameraComponent = ecs.GetSystem<CameraSystem>().GetActiveCamera();
        GlobalTransform* cameraTransform = ecs.GetComponent<GlobalTransform>(cameraComponent->GetOwningEntity());
        if (worldTransform)
        {
            Transform newGlobalTransform = worldTransform->transform;
            Transform camWorldTransform{cameraTransform->transform};
            static bool prevState = false;
            bool hasChanged = EditTransform(camWorldTransform, cameraComponent->projection, newGlobalTransform);

            if (!hasChanged && prevState)
            {
                dirtySet.insert(*selection.selectedEntity);
                ecs.NotifyComponentChanged(*worldTransform);
            }
            
            prevState = hasChanged;
            if (hasChanged)
            {
                ecs.GetSystem<TransformSystem>().SetRelativeFromWorld(*selection.selectedEntity, newGlobalTransform);
            }
        }
    }
}

void Editor::OnExit()
{
    if (ecs.IsSimulationActive())
    {
        StopPlayInEditor();
    }

    if (HasDirtyData())
    {
        auto result = MessageBoxA(nullptr, "Would you like to save the current scene before exiting?", "Save and exit?", MB_ICONQUESTION | MB_YESNO);
        if (result == IDYES)
        {
            SaveCurrentScene();
        }    
    }
}

bool Editor::HasDirtyData() const noexcept
{
    return ResourceManager::Get().HasDirtyAssets() || !dirtySet.empty() || needToSaveScene;
}

void Editor::UpdateEditorCamera(double dt)
{
    CameraComponent* activeCam = ecs.GetSystem<CameraSystem>().GetActiveCamera();
    if (!activeCam)
        return;

    EditorCameraComponent* editorComp = ecs.GetComponent<EditorCameraComponent>(activeCam->GetOwningEntity());
    if (!editorComp)
        return;
    
    InputSystem& inputs = ecs.GetSystem<InputSystem>();
    const auto mouseState = inputs.GetMouseState();
    const Inputs::MouseTracker& mouseTracker = inputs.GetMouseTracker();
    if (mouseTracker.rightButton == DirectX::Mouse::ButtonStateTracker::RELEASED)
    {
        InputSystem::Get().SetMouseMode((DirectX::Mouse::MODE_ABSOLUTE));
    }
    else if (mouseTracker.rightButton == DirectX::Mouse::ButtonStateTracker::PRESSED && isViewportHovered)
    {
        InputSystem::Get().SetMouseMode((DirectX::Mouse::MODE_RELATIVE));
    }

    if (InputSystem::Get().GetMouseMode() == DirectX::Mouse::MODE_RELATIVE)
    {
        Vector3 movement = Vector3::Zero;
    
        TransformComponent* transformComponent = ecs.GetComponent<TransformComponent>(activeCam->GetOwningEntity());
        if (mouseTracker.rightButton == DirectX::Mouse::ButtonStateTracker::HELD)
        {
            const Quaternion rotx = Quaternion::CreateFromAxisAngle(Vector3::UnitY, -mouseState.x / 1000.0f);
            const Quaternion roty = Quaternion::CreateFromAxisAngle(transformComponent->transform.GetRightVector(), -mouseState.y / 1000.0f);
            transformComponent->transform.rotation *= roty * rotx;
        
            if (mouseTracker.relativeScrollWheel != 0)
                editorComp->cameraSpeed = std::max(editorComp->cameraSpeed * (mouseTracker.relativeScrollWheel > 0 ? 1.25f : 0.75f), 10.0f);

            const DirectX::Keyboard::State keyboardState = inputs.GetKeyboardState();

            auto [up, right, forward] = transformComponent->transform.GetAxis();

            movement += keyboardState.W * forward * static_cast<float>(dt) * editorComp->cameraSpeed;
            movement -= keyboardState.S * forward * static_cast<float>(dt) * editorComp->cameraSpeed;

            movement += keyboardState.D * right * static_cast<float>(dt) * editorComp->cameraSpeed;
            movement -= keyboardState.A * right * static_cast<float>(dt) * editorComp->cameraSpeed;

            movement += keyboardState.E * up * static_cast<float>(dt) * editorComp->cameraSpeed;
            movement -= keyboardState.Q * up * static_cast<float>(dt) * editorComp->cameraSpeed;
        }

        transformComponent->transform.position += movement;
    }
}

bool Editor::EditTransform(Transform& cameraTransform, CameraProjection& cameraProjection, Transform& objectTransform)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImRect innerRect = window->InnerClipRect;
    ImVec2 size = innerRect.GetSize();
    ImGuizmo::SetRect(innerRect.Min.x, innerRect.Min.y, size.x, size.y);
    
    Matrix view = CameraSystem::GetViewMatrix(cameraTransform);
    Matrix projection = CameraSystem::GetProjectionMatrix(cameraProjection);
    Matrix model = objectTransform.ToMatrixWithScale();
    Matrix og = model;
    Matrix delta;

    bool hasChanged = false;
    
    ImGuizmo::SetDrawlist();    
    hasChanged = ImGuizmo::Manipulate(view.m[0], projection.m[0], selection.currentOperation, selection.currentMode, model.m[0], delta.m[0],
                         selection.snapToGrid ? &selection.snapGrid.x : nullptr,
                         selection.boundSizing ? &selection.bounds[0].x : nullptr,
                         selection.boundSizingSnap ? &selection.boundsSnap.x : nullptr);

    // check that model does not contain a NaN
    for (size_t i = 0; i < 4; ++i)
    for (size_t j = 0; j < 4; ++j)
    {
        if (std::isnan(model.m[i][j]))
        {
            return false;
        }
    }

    if (hasChanged)
    {
        objectTransform = model;
    }
    
    return hasChanged;
}

#else

Editor::Editor() {}
Editor::~Editor() {}
void Editor::Init(const Graphics& graphics) {}
void Editor::StartFrame() {}
void Editor::EndFrame() {}
void Editor::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) { }
void Editor::Subscribe(const std::function<void()>& sub) {}
void Editor::EditTransform(Transform& cameraTransform, CameraProjection& cameraProjection, Transform& objectTransform){};

#endif
