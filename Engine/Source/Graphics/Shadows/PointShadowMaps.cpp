module;

#include "Graphics/Utils/EventScopeMacros.h"

module Graphics:PointShadowMaps;

import :PointShadowMaps;
import :ResourceManager;
import :CubemapHelpers;
import :Scene;

import Core;
import Components;

void PointShadowMaps::Init(Device& device)
{
    size = 1024;
    linearDepth = true;
    ShadowMap::Init(device);
}

void PointShadowMaps::Render(GraphicsScene& scene, EntityComponentSystem& ecs)
{
    Array<std::tuple<GlobalTransform*, MeshComponent*>> meshInstances = ecs.GetArchetype<GlobalTransform, MeshComponent>();
    Array<std::tuple<GlobalTransform*, PunctualLightComponent*>> punctualLights = ecs.GetArchetype<GlobalTransform, PunctualLightComponent>();
    auto partitionPoint = std::stable_partition(punctualLights.begin(), punctualLights.end(), [](const auto& entity) { return std::get<1>(entity)->type == PunctualLightData::POINT; });

    uint32_t nPointLights = static_cast<uint32_t>(std::distance(punctualLights.begin(), partitionPoint));

    if (nPointLights == 0)
        return;

    uint32_t prevSize = shadowMap ? shadowMap->GetArraySize() : 0;
    if (prevSize != nPointLights)
    {
        numMaps = nPointLights;
        shadowMap = { scene.device, size, size, nPointLights * 6, true };
        pointLightData.resize(nPointLights);
    }
    
    for (uint32_t i = 0; i < nPointLights; ++i)
        Render(scene, i, punctualLights[i], meshInstances);

    pointLightDataBuffer.Update(scene.device, pointLightData);
}

void PointShadowMaps::Render(GraphicsScene& scene, uint32_t index, const std::tuple<GlobalTransform*, PunctualLightComponent*>& entity, const Array<std::tuple<GlobalTransform*, MeshComponent*>>& meshes)
{
    auto [transformComp, punctualLightComp] = entity;

    if (punctualLightComp->type != PunctualLightData::POINT)
        return;

    SC_SCOPED_GPU_EVENT("Point Shadow Map");

    const Matrix proj = Matrix::CreatePerspectiveFieldOfView(DegToRad(90), 1, 0.1f, punctualLightComp->attenuationRadius); 

    scene.device.SetVertexShader(*vsShader);
    scene.device.SetPixelShader(*psShader);
    scene.device.SetRasterizerState(RasterizerStates::Get<RasterizerPresets::NoCulling>(scene.device));

    PointLightData data {
        transformComp->position,
        punctualLightComp->attenuationRadius
    };
    pointLightBuffer.Update(scene.device, data);
    scene.device.SetConstantBuffers(psShader->GetConstantBufferSlot("PointLightData"), ViewSpan{ &pointLightBuffer }, PIXEL_SHADER | VERTEX_SHADER);

    pointLightData[index] = data;

    for (int i = 0; i < 6; ++i)
    {
        auto dsv = shadowMap->GetDSV(index * 6 + i);
        scene.device.SetRenderTargetsAndDepthStencilView({}, dsv);
        scene.device.ClearDepthStencilView(dsv);
        scene.device.SetViewports({ viewport });

        Matrix view = CubemapHelpers::GetViewMatrixForFace(i, transformComp->position);
        
        viewProjBuffer.Update(scene.device, view * proj);
        scene.device.SetConstantBuffers(vsShader->GetConstantBufferSlot("TransformMatrixes"), ViewSpan{ &viewProjBuffer} , VERTEX_SHADER);
        
        for (auto [transform, meshInstance] : meshes)
        {
            if (meshInstance->mesh)
                Graphics::DrawGeometry(scene.device, *transform, *meshInstance->mesh->geometry);
        }
    }
}
