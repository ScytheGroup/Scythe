module;
#include "Common/Macros.hpp"
#include "Serialization/SerializationMacros.h"

export module Components:Camera;
import Common;
import :Component;
import :Transform;

export struct FrameData;

export struct CameraProjection
{
    enum class ProjectionType
    {
        PERSPECTIVE,
        ORTHOGRAPHIC
    };

    union
    {
        float width, FOV;
    };

    union
    {
        float height, aspectRatio;
    };

    float nearPlane, farPlane;
    ProjectionType type;

    CameraProjection(const CameraProjection&) = default;
    CameraProjection& operator=(const CameraProjection&) = default;

    CameraProjection(CameraProjection&&) = default;
    CameraProjection& operator=(CameraProjection&&) = default;

    CameraProjection(ProjectionType projType, float widthOrFov, float heightOrAspectRatio, float nearPlane, float farPlane);

    void SetAspectRatio(float width, float height);

    static CameraProjection CreateDefaultProjection();

#if IMGUI_ENABLED
    bool ImGuiDraw();
#endif
};

export class CameraComponent : public Component
{
    SCLASS(CameraComponent, Component);
    REQUIRES(TransformComponent);
    
    friend class CameraSystem;
    friend struct CubemapHelpers;

public:
    CameraComponent(const CameraProjection& projection = CameraProjection::CreateDefaultProjection(), bool isActiveCamera = false);

    CameraProjection projection;
    bool isActiveCamera;
};

export class ActiveCameraComponent : public Component
{
    SCLASS(ActiveCameraComponent, Component);
    REQUIRES(CameraComponent);
public:
    ActiveCameraComponent();
    uint32_t priority = 0;
};

DECLARE_SERIALIZABLE(CameraComponent)