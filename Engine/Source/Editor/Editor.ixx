export module Editor;

import <Windows.h>;
import <functional>;
import <optional>;
import <string>;
import <d3d11.h>;
import <Mouse.h>;
import "imgui_impl_dx11.h";
import "imgui_impl_win32.h";

export import ImGui;
export import :EditorHelpers;
export import :EditorDelegates;
export import :Icons;

import :EditorDelegates;
import :ResourceBrowserWidget;
import :AssetBrowser;

import Graphics;
import Common;

export class Scene;
export class Editor;
export class EditorSettings;

export struct SelectionState
{
    std::optional<Handle> selectedEntity{};

    ImGuizmo::OPERATION currentOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE currentMode = ImGuizmo::LOCAL;

    bool snapToGrid = false;
    Vector3 snapGrid{ 1.f, 1.f, 1.f };

    bool boundSizing = false;
    std::array<Vector3, 2> bounds{ Vector3{ -0.5f, -0.5f, -0.5f }, Vector3{ 0.5f, 0.5f, 0.5f } };

    bool boundSizingSnap = false;
    Vector3 boundsSnap{ 0.1f, 0.1f, 0.1f };

    void UpdateState();
};

export class Editor
{
    struct EntityEditorAction
    {
        enum ActionType
        {
            DUPLICATE_ENTITY,
            REMOVE_ENTITY,
            CREATE_CHILD_ENTITY,
            PASTE_ENTITY
        } type;

        Handle entity;
    };

public:
    Editor(EntityComponentSystem& ecs);

    // Windows specific init
    void BeginDrawImGui();
    void DrawEditorWindows();
    void EndDrawImGui();
    
    void InitIcons();

    void DrawEditorWidget(const std::string& panelName, const std::function<void(Editor&)>& func);

    void Update(double dt);
    void UpdateSelection();
    void DrawSelectionWidget();

    void Subscribe(const std::function<void()>& sub);

    EntityComponentSystem& ecs;
    ImGuiManager& imGuiManager;
    AssetBrowser assetBrowser;

    SelectionState selection;
    bool isViewportHovered = false;
    ImRect viewportRect{};

    void OnExit();

    void SaveCurrentScene();
    void ReloadCurrentScene();

    void StartPlayInEditor();
    void StopPlayInEditor();

    // Manage window focus
    enum EditorWindows : uint8_t
    {
        Entities = 0,
        Details,
        Viewport,
        Other,
        Count,
    };
    void UpdateEditorFocus(EditorWindows window);
private:
    bool HasDirtyData() const noexcept;
    void UpdateEditorCamera(double dt);
    bool EditTransform(Transform& cameraTransform, CameraProjection& cameraProjection, Transform& objectTransform);

    Handle editorCameraEntity;
    std::vector<std::function<void()>> editorCallbacks;
    ResourceBrowserWidget resourceBrowser;
    std::string editorSceneName = "Default";
    std::optional<Handle> copiedEntity = {};
    Transform editorCamSavedTransform = Transform::CreateLookAt(Vector3{ 15, 15, -15 }, Vector3::Zero, Vector3::Up);

    std::vector<EntityEditorAction> deferredEditorActions;
    void HandleDeferredAction(const EntityEditorAction& action);
    void HandleDeferredActions();

    std::unordered_set<Handle> dirtySet;
    bool needToSaveScene = false;

    // Draws window with the list of entities
    void DrawSceneGraphWindow();
    void DrawEntityTreeNode(Handle entity);
    void DrawEntityDetails(bool* opened);
    void DrawEntityContextMenu(Handle entitiy);
    void UpdateEditorControls();
    void ImportAsset();

    Handle CreateDuplicate(Handle source);
    Handle PasteEntityTo(Handle fromEntity, Handle newParentEntity);
    Handle DuplicateEntity(Handle entity);
    void RemoveEntity(Handle entity);
    Handle CreateChildEntity(Handle parent);

    // Delegate calls
    void OnNewSceneLoaded(const std::string& sceneName);
    void InjectNewCameraEditor();

    bool ComponentModal();
    void EjectCamera();

    EditorWindows currentFocus;

    // dirty but whatever
    bool firstFrame = true;

    bool pieMouseOverriden = false;
};
