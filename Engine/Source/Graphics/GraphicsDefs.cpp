module Graphics:GraphicsDefs;

import :GraphicsDefs;
import Graphics;
import Systems;

void FrameData::UpdateView(const Transform& cameraTransform, const CameraProjection& cameraProjection)
{
    view = CameraSystem::GetViewMatrix(cameraTransform);
    projection = CameraSystem::GetProjectionMatrix(cameraProjection);
    viewProjection = view * projection;
    
    invView = view.Invert();
    invProjection = projection.Invert();
    invViewProjection = viewProjection.Invert();

    cameraPosition = Vector3::Transform(Vector3(0,0,0), invView);
}