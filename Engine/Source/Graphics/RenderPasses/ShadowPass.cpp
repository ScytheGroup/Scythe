module;

#include "../Utils/EventScopeMacros.h"

module Graphics:RenderPass;

import :RenderPass;
import :Scene;

import Core;
import Components;

void ShadowPass::Init(Device& device)
{
    dirShadowMap.Init(device);
    spotShadowMaps.Init(device);
    pointShadowMaps.Init(device);
}

void ShadowPass::Render(GraphicsScene& scene, EntityComponentSystem& ecs)
{
    SC_SCOPED_GPU_EVENT("Shadows");
    
    dirShadowMap.Render(scene, ecs);
    spotShadowMaps.Render(scene, ecs);
    pointShadowMaps.Render(scene, ecs);

    sceneShadowData.directionalShadowMap = &*dirShadowMap.shadowMap;
    sceneShadowData.shadowMapCascades = &dirShadowMap.cascadeShadowBuffer;
    
    if (pointShadowMaps.numMaps > 0)
    {
        sceneShadowData.pointShadowMaps = &*pointShadowMaps.shadowMap;
        sceneShadowData.pointLightData = &pointShadowMaps.pointLightDataBuffer;
    }
    else
    {
        sceneShadowData.pointShadowMaps = nullptr; 
        sceneShadowData.pointLightData = nullptr;
    }
    
    if (spotShadowMaps.numMaps > 0)
    {
        sceneShadowData.spotShadowMaps = &*spotShadowMaps.shadowMap;
        sceneShadowData.spotLightViewMatrices = &spotShadowMaps.spotLightMatricesBuffer;        
    }
    else
    {
        sceneShadowData.spotShadowMaps = nullptr;
        sceneShadowData.spotLightViewMatrices = nullptr;
    }
}

#ifdef IMGUI_ENABLED

import ImGui;

bool ShadowPass::ImGuiDraw()
{
    bool result = false;
    ImGui::SeparatorText("Directional Shadows");
    result |= dirShadowMap.ImGuiDraw(); 
    return result;
}

#endif  