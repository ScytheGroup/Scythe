module;

module Sandbox;

import :RocketSystem;
import :RocketComponent;
import Scythe;

import Graphics;

static constexpr RayCastSettings castSettings{BackFaceMode::IgnoreBackFaces, BackFaceMode::CollideWithBackFaces};


void RocketSystem::Update(double delta)
{
    static auto& ecs = GetEcs();
    auto rocketComps = ecs.GetArchetype<TransformComponent, RigidBodyComponent, CollisionShapeComponent, RocketComponent>();

    auto& physics = ecs.GetSystem<Physics>();
    BodyManipulationInterface& bodyManipulationInterface = physics.GetBodyManipulationInterface();
    for (auto [t, r, c, rocket] : rocketComps)
    {
        Vector3 front = t->transform.GetForwardVector();

        // Add steering using wasd
        InputSystem& inputSystem = ecs.GetSystem<InputSystem>();

        static constexpr float BaseYawPitchInput = 1.f;
        static constexpr float BaseRollInput = 5.f;
        float yawPitch = BaseYawPitchInput * delta;
        float roll = BaseRollInput * delta;
        
        if (inputSystem.IsKeyHeld(Inputs::Keys::S))
        {
            t->transform.Rotate(Quaternion::CreateFromAxisAngle(t->transform.GetRightVector(), yawPitch));
        }
        if (inputSystem.IsKeyHeld(Inputs::Keys::W))
        {
            t->transform.Rotate(Quaternion::CreateFromAxisAngle(t->transform.GetRightVector(), -yawPitch));
        }
        if (inputSystem.IsKeyHeld(Inputs::Keys::A))
        {
            t->transform.Rotate(Quaternion::CreateFromAxisAngle(t->transform.GetUpVector(), yawPitch));
        }
        if (inputSystem.IsKeyHeld(Inputs::Keys::D))
        {
            t->transform.Rotate(Quaternion::CreateFromAxisAngle(t->transform.GetUpVector(), -yawPitch));
        }
        if (inputSystem.IsKeyHeld(Inputs::Keys::E))
        {
            t->transform.Rotate(Quaternion::CreateFromAxisAngle(t->transform.GetForwardVector(), roll));
        }
        if (inputSystem.IsKeyHeld(Inputs::Keys::Q))
        {
            t->transform.Rotate(Quaternion::CreateFromAxisAngle(t->transform.GetForwardVector(), -roll));
        }
        
        if (inputSystem.IsKeyHeld(Inputs::Keys::Space))
            bodyManipulationInterface.AddForce(*r, front * rocket->desiredSpeed);


        // Adjust the velocity towards the front
        Vector3 velocity = bodyManipulationInterface.GetLinearVelocity(*r);
        Vector3 frontVelocity = front * velocity.Dot(front);
        Vector3 sideVelocity = velocity - frontVelocity;
        bodyManipulationInterface.SetLinearVelocity(*r, frontVelocity + sideVelocity * 0.99f);

        // do the gun
        // displace the origin of the ray a little bit to the front
        Vector3 rayOrigin = t->transform.position;
        RayCast ray{rayOrigin, front * 100};

        static bool fmj = false;

        if (InputSystem::IsKeyPressed(Inputs::Keys::OemPeriod))
            fmj = !fmj;
        
        DebugShapes::AddDebugArrow(rayOrigin, rayOrigin + front * 10, Colors::Green);
        if (inputSystem.IsMouseButtonPressed(Inputs::MouseButton::Left))
        {
            IgnoreBodiesFilter bodyFilter{r->GetOwningEntity()};
            // Todo add filters;
            // CastResult res = physics.CastRay(ray, bodyFilter);
            if (fmj)
            {
                Array resArray = physics.CastRayUninterrupted(ray, bodyFilter);
                for (auto res : resArray)
                    if (res.hit)
                    {
                        ecs.RemoveEntity(res.hitEntity);
                    }
            }
            else
            {
                CastResult res = physics.CastRay(ray, bodyFilter);
                if (res.hit)
                {
                    ecs.RemoveEntity(res.hitEntity);
                }
            }
        }
    }
}
