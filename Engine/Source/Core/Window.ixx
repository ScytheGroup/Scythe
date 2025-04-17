module;
// Have to include these here manually as macros do not cross modules :(
#include <winrt/base.h>
#include <wrl.h>
#include "Graphics/Utils/EventScopeMacros.h"
export module Core:Window;

import Common;

#ifdef IMGUI_ENABLED
import ImGui;

#ifdef EDITOR
export class Editor;
#endif // EDITOR 
#endif // IMGUI_ENABLED 

template <typename T>
struct DesktopWindow
{
    virtual ~DesktopWindow() = default;

    static T* GetThisFromHandle(HWND const window) noexcept
    {
        return reinterpret_cast<T*>(GetWindowLongPtr(window, GWLP_USERDATA));
    }

    static LRESULT __stdcall WndProc(HWND const window, UINT const message, WPARAM const wparam,
                                     LPARAM const lparam) noexcept
    {
        WINRT_ASSERT(window);

        if (WM_NCCREATE == message)
        {
            const auto cs = reinterpret_cast<CREATESTRUCT*>(lparam);
            T* that = static_cast<T*>(cs->lpCreateParams);
            WINRT_ASSERT(that);
            WINRT_ASSERT(!that->window);
            that->window = window;
            SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(that));
        }
        else if (T* that = GetThisFromHandle(window))
        {
            return that->MessageHandler(message, wparam, lparam);
        }

        return DefWindowProc(window, message, wparam, lparam);
    }

    virtual LRESULT MessageHandler(UINT const message, WPARAM const wParam, LPARAM const lParam)
    {
        if (WM_DESTROY == message)
        {
            shouldExit = true;
            PostQuitMessage(static_cast<int>(wParam));
            return 0;
        }

        return DefWindowProc(window, message, wParam, lParam);
    }

    void RequestExit() { shouldExit = true; }

protected:
    HWND window = nullptr;
    bool shouldExit = false;
};

export class Window : public DesktopWindow<Window>
{
    friend class Engine;
    std::function<void(uint32_t, uint32_t)> resizeCallback;

public:
    enum class WindowMode
    {
        WINDOWED,
        FULLSCREEN
    };

    Window(const LPCWSTR windowName, const WindowMode initialWindowMode, const std::function<void(uint32_t, uint32_t)>& resizeFunction) noexcept;

    void RefreshResolution();

    virtual void ResizeWindow(const uint32_t newWidth, const uint32_t newHeight);

    virtual void Run();

    void ToggleFullscreen();

    LRESULT MessageHandler(const UINT message, const WPARAM wParam, const LPARAM lParam) override;

    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    HWND GetInternalWindow() const;

#ifdef EDITOR
    void BindEditor(Editor* ptr);

private:
    Editor* editor = nullptr;
#endif

protected:
    uint32_t width, height;
    WindowMode windowMode;

    // Used to store previous windowed resolution when going fullscreen
    RECT windowedModeDimensions;

    // Used to resize the window after all events have been read
    uint32_t resizeWidth = 0, resizeHeight = 0;

};
