module;
#include <Common/Macros.hpp>
module [project_name];

import Scythe;
import Systems;

[project_name]Game::[project_name]Game()
{
    IMGUI_REGISTER_WINDOW("[project_name] Debug", SCENE, [project_name]Game::DrawDebug);
}

void [project_name]Game::Init()
{

}

void [project_name]Game::Update(double dt)
{
    
}

#ifdef IMGUI_ENABLED

void [project_name]Game::DrawDebug(bool* opened)
{
    
}

#endif
