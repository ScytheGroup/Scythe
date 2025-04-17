module;
#include "Common/Macros.hpp"
export module Systems:Input;
import :System;
import Common;

export namespace Inputs
{
    using Keyboard = DirectX::Keyboard;
    using Mouse = DirectX::Mouse;
    using KeyboardTracker = DirectX::Keyboard::KeyboardStateTracker;
    using MouseTracker = DirectX::Mouse::ButtonStateTracker;
    using MouseState = DirectX::Mouse::State;
    using KeyboardState = DirectX::Keyboard::State;
    using MouseMode = DirectX::Mouse::Mode;
    using Keys = DirectX::Keyboard::Keys;
    using MouseButtonsState = DirectX::Mouse::ButtonStateTracker::ButtonState;

    enum class MouseButton : uint8_t
    {
        Left = 0,
        Right = 1,
        Middle = 2,
        Thumb1 = 3,
        Thumb2 = 4
    };
}

export class InputSystem : public InstancedSingleton<InputSystem>, public System
{
    Inputs::Keyboard keyboard;
    Inputs::Mouse mouse;
    Inputs::Keyboard::KeyboardStateTracker keyboardTracker;
    Inputs::Mouse::ButtonStateTracker mouseTracker;
    Inputs::Mouse::State mouseState;
    Inputs::Keyboard::State keyboardState;
    Inputs::Mouse::Mode currentMode;
public:
    InputSystem();
    ~InputSystem() override;
    
    void Init() override;
    void Update(double delta) override;


    static Inputs::Keyboard& GetKeyboard();
    static Inputs::Mouse& GetMouse();
    static const Inputs::KeyboardTracker& GetKeyboardTracker();
    static const Inputs::MouseTracker& GetMouseTracker();
    static const Inputs::MouseState& GetMouseState();
    static const Inputs::KeyboardState& GetKeyboardState();

    static void SetMouseMode(DirectX::Mouse::Mode mode);
    static Inputs::Mouse::Mode GetMouseMode();
    static void SetMouseModeRelative();
    static void SetMouseModeAbsolute();

    static bool IsKeyPressed(Inputs::Keys key);
    static bool IsKeyReleased(Inputs::Keys key);
    static bool IsKeyHeld(Inputs::Keys key);

    static bool IsMouseButtonPressed(Inputs::MouseButton button);
    static bool IsMouseButtonReleased(Inputs::MouseButton button);
    static bool IsMouseButtonHeld(Inputs::MouseButton button);
};
