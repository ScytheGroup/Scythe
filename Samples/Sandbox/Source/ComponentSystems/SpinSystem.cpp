module Sandbox:SpinSystem;

import Scythe;
import :SpinComponent;
import :SpinSystem;
import SandboxGenerated;

void SpinSystem::Init()
{
    System::Init();
}

void SpinSystem::Update(double delta)
{
    System::Update(delta);
    auto& ecs = GetEcs();
    auto archetype = ecs.GetArchetype<TransformComponent, SpinComponent>();
    for (auto& [transformComp, spinComponent] : archetype)
    {
        // Prevents div by 0
        if (spinComponent->rotationSpeed == 0.0f)
            continue;
        
        Quaternion mat = Quaternion::CreateFromAxisAngle(Vector3::Up, delta * spinComponent->rotationSpeed);
        transformComp->rotation *= mat;
    }
}
