module;
#include "Common/Macros.hpp"
#include "Graphics/Utils/EventScopeMacros.h"
module Systems;
import :Camera;
import Common;
import Core;
import Components;

#ifdef EDITOR
import Editor;
template void CameraSystem::SetFirstActiveCameraAsMain<EditorCameraComponent>();
#endif


namespace
{
    CameraComponent* GetCameraComponentFromEntity(Handle entity, EntityComponentSystem& ecs)
    {
        CameraComponent* cam = ecs.GetComponent<CameraComponent>(entity);
#ifdef EDITOR
        cam = cam ? cam : ecs.GetComponent<EditorCameraComponent>(entity);
#endif
        return cam;
    }
}

template void CameraSystem::SetFirstActiveCameraAsMain<CameraComponent>();

template <class CameraComp>
void CameraSystem::SetFirstActiveCameraAsMain()
{
    EntityComponentSystem& ecs = GetEcs();
    Array<std::tuple<CameraComp*>> camComps = ecs.GetArchetype<CameraComp>();
    for (auto [cam] : camComps)
    {
        if (cam->isActiveCamera)
        {
            SetMainCamera(cam);
            // activeCamera = cam->GetOwningEntity();
            break;
        }
    }
}

void CameraSystem::Init()
{
#if !EDITOR
    SetFirstActiveCameraAsMain<CameraComponent>();
#endif
}

void CameraSystem::Update(double delta)
{
}

void CameraSystem::SetMainCamera(CameraComponent* component)
{
    SetMainCamera(component, GetEcs());
}

void CameraSystem::SetMainCamera(CameraComponent* component, EntityComponentSystem& ecs)
{
    SetMainCameraInternal(component, ecs);
}

void CameraSystem::RotateCamera(TransformComponent* cameraTransform, int dx, int dy)
{
    // right handed rotates counter clockwise (so -dx and -dy)
    const Quaternion rotx = Quaternion::CreateFromAxisAngle(Vector3::UnitY, -dx / 1000.0f);
    const Quaternion roty = Quaternion::CreateFromAxisAngle(cameraTransform->transform.GetRightVector(), -dy / 1000.0f);
    cameraTransform->transform.rotation *= roty * rotx;
}

Matrix CameraSystem::GetViewMatrix(const Transform& transform)
{
    return Matrix::CreateLookAt(transform.position, transform.position + transform.GetForwardVector(), transform.GetUpVector());
}

Matrix CameraSystem::GetProjectionMatrix(const CameraComponent* component)
{
    return CameraSystem::GetProjectionMatrix(component->projection);
}

Matrix CameraSystem::GetProjectionMatrix(const CameraProjection& projection)
{
    if (projection.type == CameraProjection::ProjectionType::PERSPECTIVE)
        return Matrix::CreatePerspectiveFieldOfView(DegToRad(projection.FOV), projection.aspectRatio, projection.nearPlane, projection.farPlane);
    if (projection.type == CameraProjection::ProjectionType::ORTHOGRAPHIC)
        return Matrix::CreateOrthographic(projection.width, projection.height, projection.nearPlane, projection.farPlane);
    return Matrix::Identity;
}

DirectX::BoundingFrustum CameraSystem::GetFrustum(const CameraComponent* component, const Transform& transform)
{
    DirectX::BoundingFrustum frustum = DirectX::BoundingFrustum(GetProjectionMatrix(component), true);
    Matrix invView = GetViewMatrix(transform).Invert();
    frustum.Transform(frustum, invView);
    return frustum;
}

CameraComponent* CameraSystem::GetActiveCamera()
{
    EntityComponentSystem& ecs = GetEcs();

    // use caching for better fetching
    if (ecs.IsValidEntity(mainCamera))
    {
        return GetCameraComponentFromEntity(mainCamera, ecs);
    }

    // if no fetch then check current cameras marked as active 
    auto activeCams = ecs.GetComponentsOfType<ActiveCameraComponent>();
    
    if (!activeCams.size())
    {
        // no marked cameras just get first camera
        auto cameras = ecs.GetComponentsOfType<CameraComponent>();
        if (!cameras.size())
        {
#ifdef EDITOR
            auto c = ecs.GetComponentsOfType<EditorCameraComponent>();
            DebugPrintWindow("Error: game camera was deleted with no fallback left");
            SetMainCameraInternal(c.front(), GetEcs());
            return c.front();
#else
            Assert(false, "Error: game camera was deleted with no fallback left");
#endif
        }
        
        GetEcs().AddComponent<ActiveCameraComponent>(cameras.front()->GetOwningEntity());
        return cameras.front();
    }
    
    std::sort(activeCams.begin(), activeCams.end(), [] (ActiveCameraComponent* component1, ActiveCameraComponent* component2)
    {
        return component1->priority >= component2->priority;
    });

    auto* activeCam = activeCams.front();
    return GetCameraComponentFromEntity(activeCam->GetOwningEntity(), ecs);
}

void CameraSystem::SetMainCameraInternal(CameraComponent* component, EntityComponentSystem& ecs) const
{
    if (!component)
        return;

    if (ecs.IsValidEntity(mainCamera))
        ecs.RemoveComponent<ActiveCameraComponent>(mainCamera);
    
    ecs.AddComponent<ActiveCameraComponent>(component->GetOwningEntity());
    mainCamera = component->GetOwningEntity();
}