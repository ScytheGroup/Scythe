#ifdef EDITOR
module;
#include <Common/Macros.hpp>

export module Components:Editor;


import :Component;
import :Camera;

export class EditorCameraComponent : public CameraComponent
{
    SCLASS(EditorCameraComponent, CameraComponent);
    REQUIRES(TransformComponent);
    EDITOR_ONLY();
public:
    EditorCameraComponent(const CameraProjection& projection = CameraProjection::CreateDefaultProjection(), bool isActiveCamera = true)
        : Super(projection, isActiveCamera)
    {
    };
    float cameraSpeed = 50.0f;
};
#else
export module Components:Editor;
#endif
