module;

#include "Graphics/Utils/EventScopeMacros.h"

module Graphics:DirectionalProjectedShadowMap;

import :DirectionalProjectedShadowMap;

import Core;
import Systems;
import Components;
import Graphics;
import :Scene;

Matrix DirectionalProjectedShadowMap::GetViewMatrix(const Vector3& lightDirection)
{
    return Matrix::CreateLookAt(boundsCenter, boundsCenter + lightDirection, Vector3::Up);
}

Matrix DirectionalProjectedShadowMap::GetProjectionMatrix(const Matrix& viewMatrix, const Vector3& lightDirection)
{
    Vector3 corners[8] = {
        boundsCenter + boundsExtents / 2.0f * Vector3(1, 1, 1),
        boundsCenter + boundsExtents / 2.0f * Vector3(-1, 1, 1),
        boundsCenter + boundsExtents / 2.0f * Vector3(-1, -1, 1),
        boundsCenter + boundsExtents / 2.0f * Vector3(-1, -1, -1),
        boundsCenter + boundsExtents / 2.0f * Vector3(1, -1, 1),
        boundsCenter + boundsExtents / 2.0f * Vector3(-1, 1, -1),
        boundsCenter + boundsExtents / 2.0f * Vector3(1, -1, -1),
        boundsCenter + boundsExtents / 2.0f * Vector3(1, 1, -1),
    };

    float minX = std::numeric_limits<float>::max(),
        maxX = std::numeric_limits<float>::lowest(),
        minY = std::numeric_limits<float>::max(),
        maxY = std::numeric_limits<float>::lowest(),
        minZ = std::numeric_limits<float>::max(),
        maxZ = std::numeric_limits<float>::lowest();
    
    for (int i = 0; i < 8; ++i)
    {
        Vector3 viewSpacePos = Vector3::Transform(corners[i], viewMatrix);
        minX = std::min(minX, viewSpacePos.x);
        maxX = std::max(maxX, viewSpacePos.x);
        
        minY = std::min(minY, viewSpacePos.y);
        maxY = std::max(maxY, viewSpacePos.y);
        
        minZ = std::min(minZ, viewSpacePos.z);
        maxZ = std::max(maxZ, viewSpacePos.z);
    }

    return Matrix::CreateOrthographic(maxX - minX, maxY - minY, minZ, maxZ);
}

void DirectionalProjectedShadowMap::Init(Device& device)
{
    size = 2048;
    ShadowMap::Init(device);
    shadowMap = { device, size, size };
}

void DirectionalProjectedShadowMap::Render(GraphicsScene& scene, EntityComponentSystem& ecs)
{
    SC_SCOPED_GPU_EVENT("Directional Projected Shadow Map");

    SceneSettings& sceneSettings = ecs.GetSceneSettings();

    Vector3 lightDirection = sceneSettings.graphics.directionalLight.direction;
    
    const Matrix view = GetViewMatrix(lightDirection);
    const Matrix proj = GetProjectionMatrix(view, lightDirection);

    auto dsv = shadowMap->GetDSV();
    scene.device.SetRenderTargetsAndDepthStencilView({}, dsv);
    scene.device.ClearDepthStencilView(dsv);
    scene.device.SetViewports({ viewport });

    viewProjBuffer.Update(scene.device, view * proj);
    scene.device.SetConstantBuffers(vsShader->GetConstantBufferSlot("TransformMatrixes"), ViewSpan{ &viewProjBuffer} , VERTEX_SHADER);
            
    scene.device.SetVertexShader(*vsShader);
    scene.device.SetPixelShader(*psShader);
    scene.device.SetRasterizerState(RasterizerStates::Get<RasterizerPresets::NoCulling>(scene.device));

    Array<std::tuple<GlobalTransform*, MeshComponent*>> meshInstances = ecs.GetArchetype<GlobalTransform, MeshComponent>();
    for (auto [transform, meshInstance] : meshInstances)
    {
        Graphics::DrawGeometry(scene.device, *transform, *meshInstance->mesh->geometry);
    }

    viewProjMatrixBuffer.Update(scene.device, view * proj);
}

void DirectionalProjectedShadowMap::SetBounds(const Vector3& center, const Vector3& extents)
{
    boundsCenter = center;
    boundsExtents = extents;
}
