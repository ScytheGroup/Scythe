export module Core:Game;

import Common;
import :EntityComponentSystem;

export class Game
{
    friend Engine;
    EntityComponentSystem* entityComponentSystem;
public:
    SceneIdentifier initialLoadedScene = std::filesystem::path{"Resources/Scenes/Default.json"};

    Game() = default;
    virtual ~Game() = default;

    virtual void Init();
    virtual void Update(double dt) = 0;

    EntityComponentSystem& GetECS() { return *entityComponentSystem; }
private:
    void BindECS(EntityComponentSystem* ecs) { entityComponentSystem = ecs; }
};

void Game::Init()
{
}
