module;

#include "Graphics/Utils/EventScopeMacros.h"

module Graphics:SpotShadowMap;

import :SpotShadowMap;
import :Lights;
import :Scene;

import Core;
import Components;
import Graphics;

class PunctualLightComponent;

void SpotShadowMaps::Render(GraphicsScene& scene, int index, const std::tuple<GlobalTransform*, PunctualLightComponent*>& entity, const Array<std::tuple<GlobalTransform*, MeshComponent*>>& meshes)
{
    auto [transformComp, punctualLightComp] = entity;

    if (punctualLightComp->type != PunctualLightData::SPOT)
        return;

    SC_SCOPED_GPU_EVENT("Spot Shadow Map");
    
    const Matrix view = Matrix::CreateLookAt(transformComp->transform.position, transformComp->transform.position - transformComp->transform.GetForwardVector(), transformComp->transform.GetUpVector());
    const Matrix proj = Matrix::CreatePerspectiveFieldOfView(punctualLightComp->outerAngle * 2, 1.0f, 0.1f, punctualLightComp->attenuationRadius);

    auto dsv = shadowMap->GetDSV(index);
    scene.device.SetRenderTargetsAndDepthStencilView({}, dsv);
    scene.device.ClearDepthStencilView(dsv);
    scene.device.SetViewports({ viewport });

    viewProjBuffer.Update(scene.device, view * proj);
    scene.device.SetConstantBuffers(vsShader->GetConstantBufferSlot("TransformMatrixes"), ViewSpan{ &viewProjBuffer} , VERTEX_SHADER);
            
    scene.device.SetVertexShader(*vsShader);
    scene.device.SetPixelShader(*psShader);
    scene.device.SetRasterizerState(RasterizerStates::Get<RasterizerPresets::NoCulling>(scene.device));
    
    for (auto [transform, meshInstance] : meshes)
    {
        if (meshInstance->mesh)
            Graphics::DrawGeometry(scene.device, *transform, *meshInstance->mesh->geometry);
    }

    spotLightMatrices[index] = view * proj;
}

void SpotShadowMaps::Init(Device& device)
{
    size = 1024;
    ShadowMap::Init(device);
}

void SpotShadowMaps::Render(GraphicsScene& scene, EntityComponentSystem& ecs)
{
    Array<std::tuple<GlobalTransform*, MeshComponent*>> meshInstances = ecs.GetArchetype<GlobalTransform, MeshComponent>();
    Array<std::tuple<GlobalTransform*, PunctualLightComponent*>> punctualLights = ecs.GetArchetype<GlobalTransform, PunctualLightComponent>();
    auto partitionPoint = std::stable_partition(punctualLights.begin(), punctualLights.end(), [](const auto& entity) { return std::get<1>(entity)->type == PunctualLightData::SPOT; });

    uint32_t nSpotLights = static_cast<uint32_t>(std::distance(punctualLights.begin(), partitionPoint));

    if (nSpotLights == 0)
        return;

    uint32_t prevSize = shadowMap ? shadowMap->GetArraySize() : 0;
    if (prevSize != nSpotLights)
    {
        numMaps = nSpotLights;
        shadowMap = { scene.device, size, size, nSpotLights };
        spotLightMatrices.resize(nSpotLights);
    }
    
    for (uint32_t i = 0; i < nSpotLights; ++i)
        Render(scene, i, punctualLights[i], meshInstances);

    spotLightMatricesBuffer.Update(scene.device, spotLightMatrices);
}
