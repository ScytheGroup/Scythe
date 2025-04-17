module;

#include "Serialization/SerializationMacros.h"

module Components:Camera;
import Graphics;
import :Camera;
import Core;

CameraProjection::CameraProjection(ProjectionType projType, float widthOrFov, float heightOrAspectRatio, float nearPlane, float farPlane)
    : width{ widthOrFov }
    , height{ heightOrAspectRatio }
    , nearPlane{ nearPlane }
    , farPlane{ farPlane }
    , type{ projType }
{
}

void CameraProjection::SetAspectRatio(float width, float height)
{
    if (type == ProjectionType::PERSPECTIVE)
        aspectRatio = width / height;
}

CameraProjection CameraProjection::CreateDefaultProjection()
{
    return CameraProjection(ProjectionType::PERSPECTIVE, 60.0f, 16.0f / 9.0f, 1.0f, 900'000);
}

#if IMGUI_ENABLED
import ImGui;

bool CameraProjection::ImGuiDraw()
{
    bool result = false;

    result |= ScytheGui::ComboEnum("Projection Type", type);
    result |= ImGui::DragFloat("Near Plane", &nearPlane, 0.1f, 0.01f, std::numeric_limits<float>::max(), "%.3f", ImGuiSliderFlags_AlwaysClamp);
    result |= ImGui::DragFloat("Far Plane", &farPlane, 0.1f, 0.01f, std::numeric_limits<float>::max(), "%.3f", ImGuiSliderFlags_AlwaysClamp);

    if (type == ProjectionType::PERSPECTIVE)
    {
        result |= ImGui::DragFloat("Field Of View", &FOV, 0.1f, 10, 170, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    }
    else
    {
        result |= ImGui::DragFloat("Width", &width, 0.1f, 0.01f, std::numeric_limits<float>::max(), "%.3f", ImGuiSliderFlags_AlwaysClamp);
        result |= ImGui::DragFloat("Height", &height, 0.1f, 0.01f, std::numeric_limits<float>::max(), "%.3f", ImGuiSliderFlags_AlwaysClamp);
    }

    return result;
}

#endif

CameraComponent::CameraComponent(const CameraProjection& projection, bool isActiveCamera)
    : projection{projection}
    , isActiveCamera{isActiveCamera}
{
}

ActiveCameraComponent::ActiveCameraComponent() {}

DEFINE_SERIALIZATION_AND_DESERIALIZATION(CameraProjection, FOV, type, nearPlane, farPlane, aspectRatio);
DEFINE_SERIALIZATION_AND_DESERIALIZATION(CameraComponent, projection, isActiveCamera)
