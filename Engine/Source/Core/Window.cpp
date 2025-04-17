module;
// Have to include these here manually as macros do not cross modules :(
#include <winrt/base.h>
#include <wrl.h>
#include "Graphics/Utils/EventScopeMacros.h"
module Core;

#ifdef EDITOR
import Editor;
#endif

Window::Window(const LPCWSTR windowName, const WindowMode initialWindowMode, const std::function<void(uint32_t, uint32_t)>& resizeFunction) noexcept
    : windowMode(initialWindowMode)
    // Todo implement a callback class and use it for the different window operations
    , resizeCallback(resizeFunction)
{
    // Windows 10 Creators update adds Per Monitor V2 DPI awareness context.
    // Using this awareness context allows the client area of the window 
    // to achieve 100% scaling while still allowing non-client window content to 
    // be rendered in a DPI sensitive fashion.
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    WNDCLASS wc{};
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hInstance = nullptr;
    wc.lpszClassName = windowName;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    RegisterClass(&wc);
    WINRT_ASSERT(!window);

    WINRT_VERIFY(CreateWindow(wc.lpszClassName,
        windowName,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr, wc.hInstance, this));

    WINRT_ASSERT(window);

    winrt::init_apartment();

    RefreshResolution();

    if (windowMode == WindowMode::FULLSCREEN)
    {
        // TODO: toggle fullscreen
    }
    
    DebugPrint("Window correctly initialized with dimensions: {}x{}", width, height);
}

void Window::RefreshResolution()
{
    RECT screenRect{};
    bool res = false;

    switch (windowMode)
    {
    case WindowMode::WINDOWED:
        res = GetClientRect(window, &screenRect);
        break;
    case WindowMode::FULLSCREEN:
        res = GetWindowRect(window, &screenRect);
        break;
    }

    if (!res)
    {
        // TODO: add better logging solution
        DebugPrint("Unable to get screen resolution.");
        std::exit(-1);
    }

    width = screenRect.right - screenRect.left;
    height = screenRect.bottom - screenRect.top;
}

void Window::ResizeWindow(const uint32_t newWidth, const uint32_t newHeight)
{
    width = newWidth;
    height = newHeight;
    resizeCallback(newWidth, newHeight);
}

void Window::Run()
{
    SC_SCOPED_EVENT("Window::Run");

    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (resizeWidth != 0 && resizeHeight != 0)
    {
        // Could look into adding a mutex here to avoid d3d11 device problems
        // std::lock_guard lock(DeviceMutex);
        DebugPrint("Resized window to {}x{}", resizeWidth, resizeHeight);
        ResizeWindow(resizeWidth, resizeHeight);
        resizeWidth = resizeHeight = 0;
    }
}

void Window::ToggleFullscreen()
{
    windowMode = static_cast<WindowMode>((static_cast<int>(windowMode) + 1) % 2);
    if (windowMode == WindowMode::FULLSCREEN)
    {
        ::GetWindowRect(window, &windowedModeDimensions);
        // Set the window style to a borderless window so the client area fills the entire screen.
        static constexpr UINT WindowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
        ::SetWindowLongW(window, GWL_STYLE, WindowStyle);

        // Query the name of the nearest display device for the window.
        // This is required to set the fullscreen dimensions of the window
        // when using a multi-monitor setup.
        const HMONITOR hMonitor = ::MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
        MONITORINFOEX monitorInfo = {};
        monitorInfo.cbSize = sizeof(MONITORINFOEX);
        ::GetMonitorInfo(hMonitor, &monitorInfo);

        ::SetWindowPos(window, HWND_TOP,
                       monitorInfo.rcMonitor.left,
                       monitorInfo.rcMonitor.top,
                       monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                       monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                       SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ::ShowWindow(window, SW_MAXIMIZE);
    }
    else
    {
        // Restore all the window decorators.
        ::SetWindowLong(window, GWL_STYLE, WS_OVERLAPPEDWINDOW);

        ::SetWindowPos(window, HWND_NOTOPMOST,
                       windowedModeDimensions.left,
                       windowedModeDimensions.top,
                       windowedModeDimensions.right - windowedModeDimensions.left,
                       windowedModeDimensions.bottom - windowedModeDimensions.top,
                       SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ::ShowWindow(window, SW_NORMAL);
    }

    RefreshResolution();
}

LRESULT Window::MessageHandler(const UINT message, const WPARAM wParam, const LPARAM lParam)
{
#ifdef IMGUI_ENABLED
    ImGuiManager::Get().HandleMessage(window, message, wParam, lParam);
#endif

    switch (message)
    {
    case WM_ACTIVATEAPP:
        DirectX::Keyboard::ProcessMessage(message, wParam, lParam);
        DirectX::Mouse::ProcessMessage(message, wParam, lParam);
        break;
    case WM_ACTIVATE:
    case WM_INPUT:
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEWHEEL:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_MOUSEHOVER:
        DirectX::Mouse::ProcessMessage(message, wParam, lParam);
        break;
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
        DirectX::Keyboard::ProcessMessage(message, wParam, lParam);
        break;
    case WM_SYSKEYDOWN:
        DirectX::Keyboard::ProcessMessage(message, wParam, lParam);
    // Alt+Enter pressed
        if (wParam == VK_RETURN && lParam & 1 << 29)
        {
            ToggleFullscreen();
        }
        break;
    case WM_MOUSEACTIVATE:
        // When you click activate the window, we want Mouse to ignore it.
        return MA_ACTIVATEANDEAT;
    case WM_PAINT: // CALLED BY OTHER APPS WHEN NOT IN FOCUS
        //	Update();
        //	Render();
        break;
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        resizeWidth = static_cast<uint32_t>(LOWORD(lParam)); // Queue resize after all messages are handled
        resizeHeight = static_cast<uint32_t>(HIWORD(lParam));
        return 0;
    case WM_CLOSE:
#ifdef EDITOR
        editor->OnExit();
#endif
        shouldExit = true;
        break;
    default:
        break;
    }

    return DesktopWindow::MessageHandler(message, wParam, lParam);
}

uint32_t Window::GetWidth() const
{
    return width;
}

uint32_t Window::GetHeight() const
{
    return height;
}

HWND Window::GetInternalWindow() const
{
    return window;
}

#ifdef EDITOR

void Window::BindEditor(Editor* ptr)
{
    editor = ptr;
}

#endif