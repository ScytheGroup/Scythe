module;
#ifdef IMGUI_ENABLED
#include <AsyncInfo.h>

export module ImGui;

export import :ScytheGui;
export import "AllImgui.h";

export import :Globals;

import Common;

export
enum class WindowCategories
{
    FILE,
    EDIT,
    ASSETS,
    TOOLS,
    SCENE,
    DEBUG,
    ENUM_SIZE
};

export class Graphics;

export class ImGuiManager
{
    // For ImGui windows tied to the toolbar button.
    struct ImGuiWindowInfo
    {
        bool opened = false;
        std::string name;
        std::function<void(bool*)> drawFunc;
    };
    std::vector<std::vector<ImGuiWindowInfo>> windowsList;

    // For simple commands (callbacks) to be executed upon clicking the option.
    struct ImGuiCommandInfo
    {
        std::string name;
        std::function<void()> commandFunc;
    };
    std::vector<std::vector<ImGuiCommandInfo>> commandsList;

    bool imGuiTriggered = true;
public:
    static ImGuiManager& Get()
    {
        static ImGuiManager instance;
        return instance;
    };
    
    ImGuiManager();
    ~ImGuiManager();
    void NewFrame();
    void InitFromGraphics(Graphics& graphics);
    void HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void DrawImGui();
    void Render();

    static void RegisterWindow(std::string_view name, WindowCategories category, std::function<void(bool*)> callback, bool openedByDefault = false);
    static void RegisterCommand(std::string_view name, WindowCategories category, std::function<void()> callback);

    static ImRect GetCurrentWindowBounds();
    static ImVec2 GetCurrentWindowSize();
};

export
{
    inline Vector2 Convert(const ImVec2& vec) { return { vec.x, vec.y }; }
    inline ImVec2 Convert(const Vector2& vec) { return { vec.x, vec.y }; }
}

#endif
