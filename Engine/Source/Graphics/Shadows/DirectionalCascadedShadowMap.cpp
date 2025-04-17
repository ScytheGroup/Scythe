module;

#include "Graphics/Utils/EventScopeMacros.h"

module Graphics:DirectionalCascadedShadowMap;
import :DirectionalCascadedShadowMap;

import Core;
import Components;
import Graphics;
import Systems;

import :Lights;
import :Scene;

std::array<Vector3, 8> GetFrustumCornersWorldSpace(const Matrix& viewProj)
{
    const auto inv = viewProj.Invert();

    int currentIndex = 0;
    std::array<Vector3, 8> frustumCorners;
    for (int x = 0; x < 2; ++x)
    {
        for (int y = 0; y < 2; ++y)
        {
            for (int z = 0; z < 2; ++z)
            {
                const Vector3 v{
                    2.0f * x - 1.0f,
                    2.0f * y - 1.0f,
                    2.0f * z - 1.0f
                };
                frustumCorners[currentIndex++] = Vector3::Transform(v, inv);
            }
        }
    }

    return frustumCorners;
}

Matrix GetLightViewMatrix(const Vector3& viewCenter, const Vector3& lightDirection)
{
    Vector3 cross = lightDirection;
    cross.x -= 1;
    cross.z += 1;

    Vector3 result;
    lightDirection.Cross(cross, result);
    result.Normalize();

    return Matrix::CreateLookAt(viewCenter, viewCenter - lightDirection, result);
}

void DirectionalCascadedShadowMap::Init(Device& device)
{
    size = 2048;
    ShadowMap::Init(device);
    shadowMap = { device, size, size, CASCADE_COUNT };
}

void DrawBoundingBox(const std::array<Vector3, 8>& boundingBox, const Color& boundingColor)
{
    DebugShapes::AddDebugLine(boundingBox[0], boundingBox[1], boundingColor);
    DebugShapes::AddDebugLine(boundingBox[1], boundingBox[2], boundingColor);
    DebugShapes::AddDebugLine(boundingBox[2], boundingBox[3], boundingColor);
    DebugShapes::AddDebugLine(boundingBox[3], boundingBox[0], boundingColor);

    DebugShapes::AddDebugLine(boundingBox[4], boundingBox[5], boundingColor);
    DebugShapes::AddDebugLine(boundingBox[5], boundingBox[6], boundingColor);
    DebugShapes::AddDebugLine(boundingBox[6], boundingBox[7], boundingColor);
    DebugShapes::AddDebugLine(boundingBox[7], boundingBox[4], boundingColor);

    DebugShapes::AddDebugLine(boundingBox[0], boundingBox[4], boundingColor);
    DebugShapes::AddDebugLine(boundingBox[1], boundingBox[5], boundingColor);
    DebugShapes::AddDebugLine(boundingBox[2], boundingBox[6], boundingColor);
    DebugShapes::AddDebugLine(boundingBox[3], boundingBox[7], boundingColor);
}

std::pair<Matrix, DirectX::BoundingOrientedBox> DirectionalCascadedShadowMap::GetLightSpaceMatrix(const Transform& cameraTransform, const CameraComponent& camera, const DirectionalLightData& light, const int cascadeId)
{
    auto& [n, f, bias] = cascades[cascadeId];

    if (n == f)
        return { Matrix::Identity, DirectX::BoundingOrientedBox() };

    const auto cameraView = CameraSystem::GetViewMatrix(cameraTransform);
    const auto projectionMatrix = CameraSystem::GetProjectionMatrix({ CameraProjection::ProjectionType::PERSPECTIVE, camera.projection.FOV, camera.projection.aspectRatio, n, f });

    const std::array frustrumPoints = GetFrustumCornersWorldSpace(cameraView * projectionMatrix);

    Vector3 center = cameraTransform.position + cameraTransform.GetForwardVector() * (f - (f - n) / 2);

    Matrix viewMatrix = GetLightViewMatrix(center, -light.direction);

    std::array<Vector3, 8> worldSpaceFrustumCorners;
    std::ranges::transform(frustrumPoints, worldSpaceFrustumCorners.begin(), [&](const Vector3& v) {
        return Vector3::Transform(v, viewMatrix);
    });
    
    DirectX::BoundingBox worldSpaceBB;
    DirectX::BoundingBox::CreateFromPoints(worldSpaceBB, 8, worldSpaceFrustumCorners.data(), sizeof(Vector3));

    DirectX::BoundingOrientedBox worldSpaceOBB;
    DirectX::BoundingOrientedBox::CreateFromBoundingBox(worldSpaceOBB, worldSpaceBB);

    auto inv = viewMatrix.Invert();

    DirectX::BoundingOrientedBox lightViewOBB;
    worldSpaceOBB.Transform(lightViewOBB, inv);

    Matrix finalViewMat = GetLightViewMatrix(lightViewOBB.Center, -light.direction);
    
    Vector3 OBBExtents = lightViewOBB.Extents;

    const Matrix projection = Matrix::CreateOrthographicOffCenter(
        -OBBExtents.x, OBBExtents.x,
        -OBBExtents.y, OBBExtents.y,
        -OBBExtents.z, OBBExtents.z
    );

    if (debugBounds)
    {
        Matrix inv = finalViewMat.Invert();
        
        std::array<Vector3, 8> corners;
        lightViewOBB.GetCorners(corners.data());
        DrawBoundingBox(corners, Colors::Blue);

        constexpr Color red{1,0,0};
        DebugShapes::AddDebugLine(Vector3::Transform({ 1, 0, 0 }, inv), Vector3::Transform({ -1, 0, 0 }, inv), red);
        DebugShapes::AddDebugLine(Vector3::Transform({ 0, 1, 0 }, inv), Vector3::Transform({ 0, -1, 0 }, inv), red);
        DebugShapes::AddDebugLine(Vector3::Transform({ 0, 0, 1 }, inv), Vector3::Transform({ 0, 0, -1 }, inv), red);

        Vector3 OBBcenter = lightViewOBB.Center;
        DebugShapes::AddDebugLine(OBBcenter + Vector3{ 1, 0, 0 }, OBBcenter + Vector3{ -1, 0, 0 }, Colors::Purple);
        DebugShapes::AddDebugLine(OBBcenter + Vector3{ 0, 1, 0 }, OBBcenter + Vector3{ 0, -1, 0 }, Colors::Purple);
        DebugShapes::AddDebugLine(OBBcenter + Vector3{ 0, 0, 1 }, OBBcenter + Vector3{ 0, 0, -1 }, Colors::Purple);

        DebugShapes::AddDebugFrustumPerspective((cameraView * projectionMatrix).Invert(), Colors::Red);
        DebugShapes::AddDebugFrustumPerspective((finalViewMat * projection).Invert(), Colors::Yellow);     
    }

    return { finalViewMat * projection, lightViewOBB };
}

void DirectionalCascadedShadowMap::UpdateCascadeBuffer(Device& device)
{
    CascadesData data;

    for (int i = 0; i < CASCADE_COUNT; ++i)
    {
        data[i].cascadeBound = cascades[i];
        data[i].matrix = cascadeInfo[i].first;
    }

    cascadeShadowBuffer.Update(device, data);
}

void DirectionalCascadedShadowMap::Render(GraphicsScene& scene, EntityComponentSystem& ecs)
{
    if (!shadowMap.has_value())
        return;
    
    SC_SCOPED_GPU_EVENT("Directional Cascaded Shadow Maps");
    scene.device.UnsetRenderTargets();

    CameraComponent* cameraComp = ecs.GetSystem<CameraSystem>().GetActiveCamera();
    
    GlobalTransform* cameraTransform = ecs.GetComponent<GlobalTransform>(cameraComp->GetOwningEntity());
    
    for (int i = 0; i < CASCADE_COUNT; ++i)
    {
        SC_SCOPED_GPU_EVENT("Cascade");
        
        auto dsv = shadowMap->GetDSV(i);
        scene.device.SetRenderTargetsAndDepthStencilView({}, dsv);
        scene.device.ClearDepthStencilView(dsv);
        scene.device.SetVertexShader(*vsShader);
        scene.device.SetPixelShader(*psShader);
        scene.device.SetRasterizerState(RasterizerStates::Get<RasterizerPresets::NoCullingDirectionalShadows>(scene.device));
        scene.device.SetViewports({ viewport });
        
        cascadeInfo[i] = GetLightSpaceMatrix(cameraTransform->transform, *cameraComp, scene.frameData.directionalLight, i);

        viewProjBuffer.Update(scene.device, cascadeInfo[i].first);
        scene.device.SetConstantBuffers(vsShader->GetConstantBufferSlot("TransformMatrixes"), ViewSpan{ &viewProjBuffer} , VERTEX_SHADER);
        
        Array<std::tuple<GlobalTransform*, MeshComponent*>> meshInstances = ecs.GetArchetype<GlobalTransform, MeshComponent>();
        for (auto [transform, meshInstance] : meshInstances)
        {
            if (meshInstance->mesh)
                Graphics::DrawGeometry(scene.device, *transform, *meshInstance->mesh->geometry);
        }
    }

    scene.device.UnsetRenderTargets();
    scene.device.UnsetDepthStencilState();

    UpdateCascadeBuffer(scene.device);
}

#ifdef IMGUI_ENABLED

import ImGui;

bool DirectionalCascadedShadowMap::ImGuiDraw()
{
    bool result = false;
    
    for (int i = 0; i < cascades.size(); ++i)
    {
        result |= ImGui::DragFloat2(std::format("##Upper_bound_{}", i).c_str(), &cascades[i].x, 0.5f, 0.1f, 1000.0f);
        result |= ImGui::DragFloat(std::format("Bias##bias_{}", i).c_str(), &cascades[i].z, 0.00001f, 0.00001f, 0.1f, "%.8f");
    }

    ImGui::Checkbox("Debug Cascades", &debugBounds);

    return result;
}
#endif
