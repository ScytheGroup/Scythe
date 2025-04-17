module Systems:Input;
import Systems;
import Core;

static InputSystem* instance = nullptr;

InputSystem::InputSystem()
    : mouseState()
    , keyboardState()
    , currentMode()
{
    instance = GetPtr();
}

InputSystem::~InputSystem()
{
}

void InputSystem::Init()
{
    System::Init();
}

void InputSystem::Update(double delta)
{
    System::Update(delta);
    mouseState = mouse.GetState();
    keyboardState = keyboard.GetState();
    mouseTracker.Update(mouseState);
    keyboardTracker.Update(keyboardState);
}

Inputs::Keyboard& InputSystem::GetKeyboard()
{
    return instance->keyboard;
}

Inputs::Mouse& InputSystem::GetMouse()
{
    return instance->mouse;
}

const Inputs::KeyboardTracker& InputSystem::GetKeyboardTracker()
{
    return instance->keyboardTracker;
}

const Inputs::MouseTracker& InputSystem::GetMouseTracker()
{
    return instance->mouseTracker;
}

const Inputs::MouseState& InputSystem::GetMouseState()
{
    return instance->mouseState;
}

const Inputs::KeyboardState& InputSystem::GetKeyboardState()
{
    return instance->keyboardState;
}

Inputs::Mouse::Mode InputSystem::GetMouseMode()
{
    return instance->currentMode;
}

void InputSystem::SetMouseModeRelative()
{
    SetMouseMode(DirectX::Mouse::MODE_RELATIVE);
}

void InputSystem::SetMouseModeAbsolute()
{
    SetMouseMode(DirectX::Mouse::MODE_ABSOLUTE);
}

// Key pressed should only be true once
bool InputSystem::IsKeyPressed(Inputs::Keys key)
{
    return instance->keyboardTracker.IsKeyPressed(key);
}

bool InputSystem::IsKeyReleased(Inputs::Keys key)
{
    return instance->keyboardTracker.IsKeyReleased(key);
}

bool InputSystem::IsKeyHeld(Inputs::Keys key)
{
    return instance->GetKeyboardState().IsKeyDown(key);
}

bool InputSystem::IsMouseButtonPressed(Inputs::MouseButton button)
{
    switch (button)
    {
    case Inputs::MouseButton::Left:
        return instance->mouseTracker.leftButton == Inputs::MouseButtonsState::PRESSED;
    case Inputs::MouseButton::Right:
        return instance->mouseTracker.rightButton == Inputs::MouseButtonsState::PRESSED;
    case Inputs::MouseButton::Middle:
        return instance->mouseTracker.middleButton == Inputs::MouseButtonsState::PRESSED;
    case Inputs::MouseButton::Thumb1:
        return instance->mouseTracker.xButton1 == Inputs::MouseButtonsState::PRESSED;
    case Inputs::MouseButton::Thumb2:
        return instance->mouseTracker.xButton2 == Inputs::MouseButtonsState::PRESSED;
    default:
        std::unreachable();
    }
}

bool InputSystem::IsMouseButtonReleased(Inputs::MouseButton button)
{
    switch (button)
    {
    case Inputs::MouseButton::Left:
        return instance->mouseTracker.leftButton == Inputs::MouseButtonsState::RELEASED;
    case Inputs::MouseButton::Right:
        return instance->mouseTracker.rightButton == Inputs::MouseButtonsState::RELEASED;
    case Inputs::MouseButton::Middle:
        return instance->mouseTracker.middleButton == Inputs::MouseButtonsState::RELEASED;
    case Inputs::MouseButton::Thumb1:
        return instance->mouseTracker.xButton1 == Inputs::MouseButtonsState::RELEASED;
    case Inputs::MouseButton::Thumb2:
        return instance->mouseTracker.xButton2 == Inputs::MouseButtonsState::RELEASED;
    default:
        std::unreachable();
    }
}

bool InputSystem::IsMouseButtonHeld(Inputs::MouseButton button)
{
    switch (button)
    {
    case Inputs::MouseButton::Left:
        return instance->mouseState.leftButton;
    case Inputs::MouseButton::Right:
        return instance->mouseState.rightButton;
    case Inputs::MouseButton::Middle:
        return instance->mouseState.middleButton;
    case Inputs::MouseButton::Thumb1:
        return instance->mouseState.xButton1;
    case Inputs::MouseButton::Thumb2:
        return instance->mouseState.xButton2;
    default:
        std::unreachable();
    }
}

void InputSystem::SetMouseMode(DirectX::Mouse::Mode mode)
{
    instance->currentMode = mode;
    instance->mouse.SetMode(mode);
}
