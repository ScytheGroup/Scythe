module;
#include "Common/Macros.hpp"
module Graphics:RenderPass;

import :RenderPass;
import :ResourceManager;
import :Scene;
import :PhysicsDebugRenderer;

import Systems.Physics;
import Components;
import Systems;

import Core;

#ifdef EDITOR

import ImGui;

DebugPass::DebugPass()
{
    IMGUI_REGISTER_WINDOW_LAMBDA("Physics", DEBUG, [this] (bool* p_open) {
        if (ImGui::Begin("Debug Lines", p_open))
        {
            bool result = false;
            
            result |= ImGui::Checkbox("Enable shape debug", &enableDebugDisplay);
        
            ImGui::BeginDisabled(!enableDebugDisplay);
            result |= ImGui::Checkbox("Display All", &showAll);
            ImGui::EndDisabled();
        }
        ImGui::End();
    });
}

void DebugPass::DrawDebugLines(GraphicsScene& scene)
{
    if (debugLines.empty())
        return;
    
    scene.device.SetVertexShader(*debugLinesVS);
    scene.device.SetInputLayout(inputLayout);
    scene.device.SetPixelShader(*debugLinesPS);

    scene.device.SetFrameBuffer(scene.sceneColorBuffer, scene.gBuffer.depthStencil);

    scene.device.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    std::vector<DebugLinesVertex> vertices;
    vertices.resize(debugLines.size() * 2);

    uint32_t i = 0;
    for (const DebugLine& line : debugLines)
    {
        DebugLinesVertex v1;
        v1.position = line.start;
        v1.color = line.color;
        vertices[i++] = std::move(v1);
        
        DebugLinesVertex v2;
        v2.position = line.end;
        v2.color = line.color;
        vertices[i++] = std::move(v2);
    }

    VertexBuffer debugLinesBuffer{ scene.device, vertices };
    scene.device.SetVertexBuffer(0, ViewSpan{ &debugLinesBuffer }, 0);

    scene.device.Draw(static_cast<uint32_t>(vertices.size()), 0);

    debugLines.clear();
}

void DebugPass::Init(Device& device)
{
    ResourceManager& rm = ResourceManager::Get();

    ShaderKey key;
    key.filepath = "DebugLines.vs.hlsl";
    key.createInputLayout = false;
    debugLinesVS = rm.shaderLoader.Load<VertexShader>(std::move(key), device);

    ShaderKey psKey;
    psKey.filepath = "DebugLines.ps.hlsl";
    debugLinesPS = rm.shaderLoader.Load<PixelShader>(std::move(psKey), device);

    inputLayout = device.CreateInputLayout(*debugLinesVS, InputLayouts::DebugLines);

    if (renderer)
        delete renderer;
    
    renderer = new PhysicsDebugRenderer();
}

void DebugPass::Render(GraphicsScene& scene, EntityComponentSystem& ecs)
{
    CameraSystem& cameraSystem = ecs.GetSystem<CameraSystem>();
    CameraComponent* camera = cameraSystem.GetActiveCamera();
    GlobalTransform* cameraTransform = ecs.GetComponent<GlobalTransform>(camera->GetOwningEntity());

    SceneSettings& ws = ecs.GetSceneSettings();
    if (enableDebugDisplay)
    {
        renderer->SetCameraPos(Convert(cameraTransform->position));
        ecs.GetSystem<Physics>().DrawDebug(ecs, renderer, showAll);
    }
    
    DrawDebugLines(scene);
}

DebugPass::~DebugPass()
{
    delete renderer;
}

#endif
