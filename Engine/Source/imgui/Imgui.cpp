module;
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <Windows.h>
#include <string>
#include "Icons/IconsFontAwesome6.h"

module ImGui;

#ifdef IMGUI_ENABLED

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

import Graphics;
import Common;
import Systems;

ImGuiManager::ImGuiManager() 
    : windowsList(static_cast<int>(WindowCategories::ENUM_SIZE))
    , commandsList(static_cast<int>(WindowCategories::ENUM_SIZE))
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    auto& io = ImGui::GetIO();
    auto path = ProjectSettings::Get().engineRoot / "Resources\\Fonts\\hack\\Hack-Regular.ttf";
    ImGui::defaultFont = io.Fonts->AddFontFromFileTTF(path.string().c_str(), 16, nullptr, io.Fonts->GetGlyphRangesDefault());

    ImFontConfig config;
    config.MergeMode = true;
    config.GlyphMinAdvanceX = 13.0f; // Use if you want to make the icon monospaced
    static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    auto fontPath = ProjectSettings::Get().engineRoot / "Resources" / "Fonts" / "awesome" / "fa-solid-900.ttf";
    // Todo discover how to generate a carray with the font file and simply compile it with the project
    io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(), 13.0f, &config, icon_ranges);
    
    auto headerPath = ProjectSettings::Get().engineRoot / "Resources\\Fonts\\hack\\Hack-Bold.ttf";
    ImGui::smallFont = io.Fonts->AddFontFromFileTTF(headerPath.string().c_str(), 14, nullptr, io.Fonts->GetGlyphRangesDefault());
    ImGui::headerFont = io.Fonts->AddFontFromFileTTF(headerPath.string().c_str(), 18, nullptr, io.Fonts->GetGlyphRangesDefault());
    ImGui::titleFont = io.Fonts->AddFontFromFileTTF(headerPath.string().c_str(), 20, nullptr, io.Fonts->GetGlyphRangesDefault());
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.FontDefault = ImGui::defaultFont;

    ImGui::StyleColorsDark();

    // if no ini file is found, load the default one
    if (!std::filesystem::exists("imgui.ini"))
        ImGui::LoadIniSettingsFromDisk("imgui_default.ini");
}

ImGuiManager::~ImGuiManager()
{
    if (ImGui::GetCurrentContext())
    {
        // Releases COM references that ImGui was given on setup
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }
}

void ImGuiManager::NewFrame()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    // Todo use tracker here once we get a system for inputs
    auto kbTracker = InputSystem::Get().GetKeyboardTracker();
    auto kbState = kbTracker.GetLastState();
    
    if (kbTracker.IsKeyPressed(DirectX::Keyboard::OemTilde) && kbState.IsKeyDown(DirectX::Keyboard::LeftControl))
    {
        imGuiTriggered = !imGuiTriggered;
    }
}

void ImGuiManager::InitFromGraphics(Graphics& graphics)
{
    ImGui_ImplWin32_Init(graphics.window);
    ImGui_ImplDX11_Init(graphics.device.device.Get(), graphics.device.deviceContext.Get());
}

void ImGuiManager::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
}

void ImGuiManager::DrawImGui()
{
    if (imGuiTriggered)
    {
        if (!ImGui::BeginMainMenuBar())
            return;
        for (auto i = 0; i < static_cast<int>(WindowCategories::ENUM_SIZE); ++i)
        {
            auto name = std::string(magic_enum::enum_name(static_cast<WindowCategories>(i)));
            std::transform(name.begin() + 1, name.end(), name.begin() + 1, [](unsigned char c) { return std::tolower(c); });
            if (ImGui::BeginMenu(name.c_str()))
            {
                for (ImGuiWindowInfo& windowInfos : windowsList[i])
                {
                    windowInfos.opened = ImGui::Selectable(windowInfos.name.c_str()) || windowInfos.opened;
                }

                if (!windowsList[i].empty() && !commandsList[i].empty()) { ImGui::Separator(); }

                for (ImGuiCommandInfo& commandInfo : commandsList[i])
                {
                    if (ImGui::Selectable(commandInfo.name.c_str())) 
                    { 
                        commandInfo.commandFunc();
                    }
                }

                ImGui::EndMenu();
            }
        }
        ImGui::EndMainMenuBar();
    }

    for (std::vector<ImGuiWindowInfo>& windows : windowsList)
    {
        for (auto&& window : windows)
        {
            if (window.opened)
                window.drawFunc(&window.opened);
        }        
    }
}

void ImGuiManager::Render()
{
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiManager::RegisterWindow(std::string_view name, WindowCategories category, std::function<void(bool*)> callback, bool openedByDefault)
{
    // Windows are NOT sorted alphabetically, to ensure the order is according to UX preferences (eg for "new scene", "open scene", "save scene").
    Get().windowsList[static_cast<int>(category)].push_back(ImGuiWindowInfo{ openedByDefault, std::string(name), callback });
}

void ImGuiManager::RegisterCommand(std::string_view name, WindowCategories category, std::function<void()> callback)
{
    // Commands are sorted alphabetically.
    std::vector<ImGuiCommandInfo>& categoryCommandsList = Get().commandsList[static_cast<int>(category)];
    categoryCommandsList.push_back(ImGuiCommandInfo{ .name = std::string(name), .commandFunc = callback });

    std::sort(categoryCommandsList.begin(), categoryCommandsList.end(), [](const ImGuiCommandInfo& a, const ImGuiCommandInfo& b) { return a.name < b.name; });
}

ImRect ImGuiManager::GetCurrentWindowBounds()
{
    return ImGui::GetCurrentWindow()->InnerClipRect;
}

ImVec2 ImGuiManager::GetCurrentWindowSize()
{
    return GetCurrentWindowBounds().GetSize();
}

#endif
