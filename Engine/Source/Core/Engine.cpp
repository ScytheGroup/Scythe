module;
#include "Windows.h"
#include "Graphics/Utils/EventScopeMacros.h"
// This needs to be here even though we dont use it. I figure its some wierd module issue
// Having it here make it work every where. 
#include <stacktrace>
module Core:Engine;

import :Engine;

import Common;
import Graphics;

import Components;

import :Game;

static Window* globalWindow = nullptr;
#ifdef EDITOR
static bool stopPIE = false;
#endif

Engine::Engine()
    : window(L"ScytheEngine", Window::WindowMode::WINDOWED, BindMethod(&Engine::OnResizedWindow, this))
    , ecs(window)
#ifdef EDITOR
    ,editor(ecs)
#endif
{
    InitModules();
#ifdef PROFILING
    // Allows us to profile the initial engine startup time
    SC_INIT_END();
#endif

    globalWindow = &window;
}

Engine::~Engine()
{
}

void Engine::InitModules()
{
#ifdef IMGUI_ENABLED
#ifdef EDITOR
    ecs.graphics->SetEditor(editor);
    window.BindEditor(&editor);
#endif
#endif
    Reflection::InitRefletion(); // Necessary to initialize runtime reflection info when engine is a lib
}

void Engine::Run()
{
    Assert(game.get(), "No game project created");
    using Clock = std::chrono::high_resolution_clock;

    std::chrono::time_point<std::chrono::steady_clock> currentTime = Clock::now();

    double accumulator = 0.0;

    ecs.PreInit();
    game->Init();
    ecs.GetSystem<SceneSystem>().LoadSceneImmediate(game->initialLoadedScene, ecs);
    
    while (!window.shouldExit)
    {
        SC_SCOPED_FRAME();

        auto newTime = Clock::now();
        // Convert time difference from nanoseconds to seconds
        double frameTime = std::chrono::duration_cast<std::chrono::duration<double>>(newTime - currentTime).count();
        currentTime = newTime;
        
        // First manage all windows events
        window.Run();

        // Simulate physics via adaptive timestepping
        accumulator += frameTime;
        while (accumulator >= SimulationTimestep)
        {
            accumulator -= SimulationTimestep;
            simulationTime += SimulationTimestep;
            ecs.FixedUpdate(SimulationTimestep);
        }
#ifdef EDITOR
        editor.Update(frameTime);
#endif
        
        // Update engine logic
        ecs.Update(frameTime);
        if (ecs.IsSimulationActive()) {
            game->Update(frameTime);
        }

        ecs.PostUpdate(frameTime);

        // Render
        ecs.UpdateGraphics(frameTime);

        ResourceManager::Get().CleanupAssetLoaders();

#ifdef EDITOR
        if (stopPIE)
        {
            stopPIE = false;
            editor.StopPlayInEditor();
        }
#endif
    }
    Shutdown();
}

void Engine::OnResizedWindow(uint32_t width, uint32_t height)
{
    ecs.graphics->OnResize(width, height);
}

void Engine::Shutdown()
{
    DebugPrint("Engine shutting down.");
}

void RequestGameExit()
{
#ifdef EDITOR
    stopPIE = true;
#else
    if (globalWindow)
        globalWindow->RequestExit();
#endif
}