module;
#include "Common/Macros.hpp"
export module Systems:Camera;
import :System;
import Common;

export struct CameraProjection;

export class CameraSystem : public System
{
    mutable Handle mainCamera;
public:
    void Init() override;
    void Update(double delta) override;

    void SetMainCamera(CameraComponent* component);
    void SetMainCamera(CameraComponent* component, EntityComponentSystem& ecs);
    
    template<class CameraComp>
    void SetFirstActiveCameraAsMain();
    
    void RotateCamera(TransformComponent* cameraTransform, int dx, int dy);
    
    static Matrix GetViewMatrix(const Transform& transform);
    static Matrix GetProjectionMatrix(const CameraComponent* component);
    static Matrix GetProjectionMatrix(const CameraProjection& projection);
    static DirectX::BoundingFrustum GetFrustum(const CameraComponent* component, const Transform& transform);

    CameraComponent* GetActiveCamera();
private:
    void SetMainCameraInternal(CameraComponent* component, EntityComponentSystem& ecs) const;
};
