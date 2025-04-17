export module Core:Engine;

import :Window;
import :EntityComponentSystem;
import :Game;

#ifdef IMGUI_ENABLED
import ImGui;

#ifdef EDITOR
import Editor;
#endif // EDITOR

#endif // IMGUI_ENABLED


export class Engine
{
    std::unique_ptr<Game> game = nullptr;
    Engine(const Engine&) = delete;
public:
    Engine();
    ~Engine() ;

	void InitModules();
	void Run() ;
	void Shutdown();

    void OnResizedWindow(uint32_t width, uint32_t height);

    template <class T>
    constexpr void CreateGame();
private:
#ifdef IMGUI_ENABLED
    ImGuiManager imgui;
#endif // IMGUI_ENABLED
    
    static constexpr double SimulationTimestep = 1.f/60;
    double simulationTime = 0.0;
    Window window;
    EntityComponentSystem ecs;

#ifdef EDITOR
    Editor editor;
#endif // IMGUI_ENABLED

};

template <class T>
constexpr void Engine::CreateGame()
{
    Assert(!game, "A game was already created.");
    game = std::make_unique<T>();
    game->BindECS(&ecs);
}

export void RequestGameExit();