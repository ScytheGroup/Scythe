module;
#include "PhysicsSystem-Headers.hpp"
#include "Jolt/Physics/Collision/CastResult.h"
#include "Jolt/Physics/Collision/Shape/CompoundShape.h"
module Systems.Physics;
import Core;

// Wrapping on Jolt's Body manipulation functions
// Velocity manipulation
void BodyManipulationInterface::MoveKinematic(const IPhysicsBodyComponent& body, const Vector3& targetPosition, const Quaternion& targetRotation, float deltaTime) const
{
    physics.bodyInterface->MoveKinematic(body.GetBodyId(), Convert(targetPosition), Convert(targetRotation), deltaTime);
}

void BodyManipulationInterface::SetLinearAndAngularVelocity(const IPhysicsBodyComponent& body, const Vector3& linearVelocity, const Vector3& angularVelocity) const
{
    physics.bodyInterface->SetLinearAndAngularVelocity(body.GetBodyId(), Convert(linearVelocity), Convert(angularVelocity));
}

void BodyManipulationInterface::GetLinearAndAngularVelocity(const IPhysicsBodyComponent& body, Vector3& outLinearVelocity, Vector3& outAngularVelocity) const
{
    JPH::Vec3 outLinearVel;
    JPH::Vec3 outAngularVel;
    physics.bodyInterface->GetLinearAndAngularVelocity(body.GetBodyId(), outLinearVel, outAngularVel);
    outLinearVelocity = Convert(outLinearVel);
    outAngularVelocity = Convert(outAngularVel);
}

void BodyManipulationInterface::SetLinearVelocity(const IPhysicsBodyComponent& body, const Vector3& linearVelocity) const
{
    physics.bodyInterface->SetLinearVelocity(body.GetBodyId(), Convert(linearVelocity));
}

Vector3 BodyManipulationInterface::GetLinearVelocity(const IPhysicsBodyComponent& body) const
{
    return Convert(physics.bodyInterface->GetLinearVelocity(body.GetBodyId()));
}

void BodyManipulationInterface::AddLinearVelocity(const IPhysicsBodyComponent& body, const Vector3& linearVelocity) const
{
    physics.bodyInterface->AddLinearVelocity(body.GetBodyId(), Convert(linearVelocity));
}

void BodyManipulationInterface::AddLinearAndAngularVelocity(const IPhysicsBodyComponent& body, const Vector3& linearVelocity, const Vector3& angularVelocity) const
{
    physics.bodyInterface->AddLinearAndAngularVelocity(body.GetBodyId(), Convert(linearVelocity), Convert(angularVelocity));
}

void BodyManipulationInterface::SetAngularVelocity(const IPhysicsBodyComponent& body, const Vector3& angularVelocity) const
{
    physics.bodyInterface->SetAngularVelocity(body.GetBodyId(), Convert(angularVelocity));
}

Vector3 BodyManipulationInterface::GetAngularVelocity(const IPhysicsBodyComponent& body) const
{
    return Convert(physics.bodyInterface->GetAngularVelocity(body.GetBodyId()));
}

Vector3 BodyManipulationInterface::GetPointVelocity(const IPhysicsBodyComponent& body, const Vector3& point) const
{
    return Convert(physics.bodyInterface->GetPointVelocity(body.GetBodyId(), Convert(point)));
}

// Force application
void BodyManipulationInterface::AddForce(const IPhysicsBodyComponent& body, const Vector3& force, JPH::EActivation activation) const
{
    physics.bodyInterface->AddForce(body.GetBodyId(), Convert(force), activation);
}

void BodyManipulationInterface::AddForce(const IPhysicsBodyComponent& body, const Vector3& force, const Vector3& point, JPH::EActivation activation) const
{
    physics.bodyInterface->AddForce(body.GetBodyId(), Convert(force), Convert(point), activation);
}

void BodyManipulationInterface::AddTorque(const IPhysicsBodyComponent& body, const Vector3& torque, JPH::EActivation activation) const
{
    physics.bodyInterface->AddTorque(body.GetBodyId(), Convert(torque), activation);
}

void BodyManipulationInterface::AddForceAndTorque(const IPhysicsBodyComponent& body, const Vector3& force, const Vector3& torque, JPH::EActivation activation) const
{
    physics.bodyInterface->AddForceAndTorque(body.GetBodyId(), Convert(force), Convert(torque), activation);
}

// Impulse application
void BodyManipulationInterface::AddImpulse(const IPhysicsBodyComponent& body, const Vector3& impulse) const
{
    physics.bodyInterface->AddImpulse(body.GetBodyId(), Convert(impulse));
}

void BodyManipulationInterface::AddImpulse(const IPhysicsBodyComponent& body, const Vector3& impulse, const Vector3& point) const
{
    physics.bodyInterface->AddImpulse(body.GetBodyId(), Convert(impulse), Convert(point));
}

void BodyManipulationInterface::AddAngularImpulse(const IPhysicsBodyComponent& body, const Vector3& angularImpulse) const
{
    physics.bodyInterface->AddAngularImpulse(body.GetBodyId(), Convert(angularImpulse));
}

Matrix BodyManipulationInterface::GetInverseInertia(const IPhysicsBodyComponent& body) const
{
    return Convert(physics.bodyInterface->GetInverseInertia(body.GetBodyId()));
}

void BodyManipulationInterface::SetRestitution(const IPhysicsBodyComponent& body, float restitution) const
{
    physics.bodyInterface->SetRestitution(body.GetBodyId(), restitution);
}

float BodyManipulationInterface::GetRestitution(const IPhysicsBodyComponent& body) const
{
    return physics.bodyInterface->GetRestitution(body.GetBodyId());
}

void BodyManipulationInterface::SetFriction(const IPhysicsBodyComponent& body, float friction) const
{
    physics.bodyInterface->SetFriction(body.GetBodyId(), friction);
}

float BodyManipulationInterface::GetFriction(const IPhysicsBodyComponent& body) const
{
    return physics.bodyInterface->GetFriction(body.GetBodyId());
}

void BodyManipulationInterface::SetGravityFactor(const IPhysicsBodyComponent& body, float gravityFactor) const
{
    physics.bodyInterface->SetGravityFactor(body.GetBodyId(), gravityFactor);
}

float BodyManipulationInterface::GetGravityFactor(const IPhysicsBodyComponent& body) const
{
    return physics.bodyInterface->GetGravityFactor(body.GetBodyId());
}

CastResult BodyManipulationInterface::TestRayCastAgainst(IPhysicsBodyComponent& component, const RayCast& ray)
{
    JPH::RayCastResult result;
    bool hit = physics.bodyInterface->GetTransformedShape(component.GetBodyId()).CastRay(ray, result);
    if (!hit)
        return {};
    return physics.GetCastResult(result, ray);
}

Handle BodyManipulationInterface::GetEntityHandle(const JPH::BodyID& id)
{
    return physics.bodyInterface->GetUserData(id);
}

Handle BodyManipulationInterface::GetSubShapeEntityHandle(const JPH::BodyID& id, const JPH::SubShapeID& subShapeID)
{
    auto shape = physics.bodyInterface->GetTransformedShape(id);
    return shape.GetSubShapeUserData(subShapeID);
}

void BodyManipulationInterface::DeactivateBody(const IPhysicsBodyComponent& body)
{
    physics.bodyInterface->DeactivateBody(body.GetBodyId());
}

void BodyManipulationInterface::ActivateBody(const IPhysicsBodyComponent& body)
{
    physics.bodyInterface->ActivateBody(body.GetBodyId());
}

#if 0
bool BodyManipulationInterface::ApplyBuoyancyImpulse(const IPhysicsBodyComponent& body, const Vector3& surfacePosition, const Vector3& surfaceNormal, float buoyancy, float linearDrag, float angularDrag,
                          const Vector3& fluidVelocity, const Vector3& gravity, float deltaTime)
{
    return physics.bodyInterface->ApplyBuoyancyImpulse(body.GetBodyId(), Convert(surfacePosition), Convert(surfaceNormal), buoyancy, linearDrag, angularDrag, Convert(fluidVelocity), Convert(gravity), deltaTime);
}

void BodyManipulationInterface::SetUseManifoldReduction(const IPhysicsBodyComponent& body, bool useReduction)
{
    physics.bodyInterface->SetUseManifoldReduction(body.GetBodyId(), useReduction);
}

bool BodyManipulationInterface::GetUseManifoldReduction(const IPhysicsBodyComponent& body) const
{
    return physics.bodyInterface->GetUseManifoldReduction(body.GetBodyId());
}
#endif