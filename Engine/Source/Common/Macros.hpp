#pragma once

#include "Reflection/TypeBase.h"
#include "Components/ComponentsUtils.hpp"
#include "Serialization/SerializationMacros.h"

#define DEFINE_SCYTHE_PROJECT(_Game) \
int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) \
{ \
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2); \
    Engine engine; \
    engine.CreateGame<_Game>(); \
    engine.Run(); \
    return 0; \
} \

// OPTIMIZATION
#ifdef SC_DEV_VERSION
#define SC_DEOPTIMIZE __pragma(message("Optimization disabled at file " __FILE__ " line " __LINE__)); \
    __pragma(optimize("", off));
#define SC_OPTIMIZE __pragma(message("Optimization enabled at file " __FILE__ " line " __LINE__)); \
    __pragma(optimize("", on));
#else
#define SC_DEOPTIMIZE
#define SC_OPTIMIZE
#endif


// Imgui macros

#ifdef IMGUI_ENABLED
#define IMGUI_REGISTER_WINDOW(name, category, method) ImGuiManager::RegisterWindow(name, WindowCategories::category, BindMethod(&method, this))
#define IMGUI_REGISTER_WINDOW_OPEN(name, category, method) ImGuiManager::RegisterWindow(name, WindowCategories::category, BindMethod(&method, this), true)
#define IMGUI_REGISTER_WINDOW_LAMBDA(name, category, lambda) ImGuiManager::RegisterWindow(name, WindowCategories::category, lambda)
#define IMGUI_REGISTER_COMMAND(name, category, method) ImGuiManager::RegisterCommand(name, WindowCategories::category, BindMethod(&method, this))
#define IMGUI_REGISTER_COMMAND_LAMBDA(name, category, lambda) ImGuiManager::RegisterCommand(name, WindowCategories::category, lambda)
#else
#define IMGUI_REGISTER_WINDOW(name, category, method) 0
#define IMGUI_REGISTER_WINDOW_OPEN(name, category, method) 0
#define IMGUI_REGISTER_WINDOW_LAMBDA(name, category, lambda) 0
#define IMGUI_REGISTER_COMMAND(name, category, method) 0
#define IMGUI_REGISTER_COMMAND_LAMBDA(name, category, lambda) 0
#endif

#define DECLARE_COMPONENT(_name) \
    namespace \
    { \
       struct _name##_Register { \
           _name##_Register() { EntityComponentSystem::RegisterComponent<_name>(); };  \
       } _name##_register; \
    }

// #define ECS_REGISTER(_name) \
//     namespace \
//     { \
//         struct _name##_Register { \
//             _name##_Register() { \
//                 if constexpr (std::is_base_of_v<Component, _name>) \
//                     EntityComponentSystem::RegisterComponent<_name>(); \
//                 else if constexpr (std::is_base_of_v<System, _name>)  \
//                     EntityComponentSystem::RegisterSystem<_name>(); \
//             };  \
//         } _name##_register; \
//     }

#define ECS_REGISTER(_name) \
namespace \
    { \
    struct _name##_ECSRegister { \
        _name##_ECSRegister() { \
            EntityComponentSystem::RegisterClass<_name>(); \
        }; \
    } _name##_ecsregister; \
}