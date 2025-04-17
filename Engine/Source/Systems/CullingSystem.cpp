module Systems;

import Core;
import Systems.Physics;
import Common;
import Components;
import :Camera;
import :CullingSystem;

void CullingSystem::Update(double delta)
{
    System::Update(delta);

    CameraSystem& cameraSystem = GetEcs().GetSystem<CameraSystem>();
    
    CameraComponent* activeCamera = cameraSystem.GetActiveCamera();
    if (!activeCamera)
        return;

    GlobalTransform* transformComponent = GetEcs().GetComponent<GlobalTransform>(activeCamera->GetOwningEntity());
    if (!transformComponent)
        return;

    auto boundingFrustum = cameraSystem.GetFrustum(activeCamera, transformComponent->transform);
    auto camPosition = Vector3::Transform(Vector3::Zero, transformComponent->transform.ToMatrixWithScale());

    Array meshes = GetEcs().GetArchetype<GlobalTransform, MeshComponent>();
    for (auto [transformComp, meshComp] : meshes)
    {
        if (!meshComp->isFrustumCulled || !meshComp->mesh.HasAsset() || !meshComp->mesh->geometry || !meshComp->mesh->geometry->IsLoaded())
            continue;
        
        DirectX::BoundingBox box = meshComp->mesh->geometry->box;

        auto globalTransform = transformComp->transform.ToMatrixWithScale();
        box.Transform(box, globalTransform);

        bool isVisible = true;

        auto meshPosition = Vector3::Transform(Vector3::Zero, transformComp->transform.ToMatrixWithScale());
        isVisible &= Vector3::Distance(camPosition, meshPosition) <= 500;
        isVisible &= boundingFrustum.Intersects(box);
        meshComp->isVisible = isVisible;
    }

    Physics& physics = GetEcs().GetSystem<Physics>();
    auto& bmi = physics.GetBodyManipulationInterface();

    Array bodies = GetEcs().GetArchetype<GlobalTransform, RigidBodyComponent>();
    for (auto [globalTransform, rigidBodyComp] : bodies)
    {
        auto body = Vector3::Transform(Vector3::Zero, globalTransform->transform.ToMatrixWithScale());

        if (Vector3::Distance(camPosition, body) >= 1000)
            bmi.DeactivateBody(*rigidBodyComp);
        else
            bmi.ActivateBody(*rigidBodyComp);
    }
}
