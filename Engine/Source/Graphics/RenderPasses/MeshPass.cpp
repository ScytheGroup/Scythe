module;
#include "../Utils/EventScopeMacros.h"
module Graphics:RenderPass;

import :RenderPass;

import :RasterizerState;
import :Scene;

import Graphics;
import Common;
import Components;
import Core;

void MeshPass::Render(GraphicsScene& scene, EntityComponentSystem& ecs)
{
    SC_SCOPED_GPU_EVENT("Meshes");

    scene.device.SetFrameBuffer(scene.gBuffer);
    scene.device.SetRasterizerState(RasterizerStates::Get<RasterizerPresets::BackfaceCulling>(scene.device));
    
    Array<std::tuple<GlobalTransform*, MeshComponent*>> meshInstances = ecs.GetArchetype<GlobalTransform, MeshComponent>();
    for (auto [transform, meshInstance] : meshInstances)
    {
        Graphics::DrawMesh(scene.device, *transform, *meshInstance);
    }
}
