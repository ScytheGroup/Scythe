module;
#include "PhysicsSystem-Headers.hpp"
#include "Jolt/Physics/EActivation.h"
#include "Jolt/Physics/Collision/Shape/SubShapeID.h"
export module Systems.Physics:BodyManipulationInterface;
import Common;
import Components;
import :PhysicsBodyInterface;
import :RayCast;

export class BodyManipulationInterface final
{
    friend class Physics;
    Physics& physics;

    BodyManipulationInterface(Physics& physics)
        : physics{ physics }
    {}

    using EActivation = JPH::EActivation;
public:
    BodyManipulationInterface() = delete;
    BodyManipulationInterface(BodyManipulationInterface&) = delete;
    BodyManipulationInterface(BodyManipulationInterface&&) = delete;
    BodyManipulationInterface& operator=(BodyManipulationInterface&) = delete;
    BodyManipulationInterface& operator=(BodyManipulationInterface&&) = delete;

    // Wrapping on Jolt's Body manipulation functions
    // Velocity manipulation
    void MoveKinematic(const IPhysicsBodyComponent& body, const Vector3& targetPosition, const Quaternion& targetRotation, float deltaTime) const;

    void SetLinearAndAngularVelocity(const IPhysicsBodyComponent& body, const Vector3& linearVelocity, const Vector3& angularVelocity) const;

    void GetLinearAndAngularVelocity(const IPhysicsBodyComponent& body, Vector3& outLinearVelocity, Vector3& outAngularVelocity) const;

    void SetLinearVelocity(const IPhysicsBodyComponent& body, const Vector3& linearVelocity) const;

    Vector3 GetLinearVelocity(const IPhysicsBodyComponent& body) const;

    void AddLinearVelocity(const IPhysicsBodyComponent& body, const Vector3& linearVelocity) const;

    void AddLinearAndAngularVelocity(const IPhysicsBodyComponent& body, const Vector3& linearVelocity, const Vector3& angularVelocity) const;

    void SetAngularVelocity(const IPhysicsBodyComponent& body, const Vector3& angularVelocity) const;

    Vector3 GetAngularVelocity(const IPhysicsBodyComponent& body) const;

    Vector3 GetPointVelocity(const IPhysicsBodyComponent& body, const Vector3& point) const;
    
    // Force application
    void AddForce(const IPhysicsBodyComponent& body, const Vector3& force, JPH::EActivation activation = EActivation::Activate) const;

    void AddForce(const IPhysicsBodyComponent& body, const Vector3& force, const Vector3& point, JPH::EActivation activation = EActivation::Activate) const;

    void AddTorque(const IPhysicsBodyComponent& body, const Vector3& torque, JPH::EActivation activation = EActivation::Activate) const;

    void AddForceAndTorque(const IPhysicsBodyComponent& body, const Vector3& force, const Vector3& torque, JPH::EActivation activation = EActivation::Activate) const;

    // Impulse application
    void AddImpulse(const IPhysicsBodyComponent& body, const Vector3& impulse) const;

    void AddImpulse(const IPhysicsBodyComponent& body, const Vector3& impulse, const Vector3& point) const;

    void AddAngularImpulse(const IPhysicsBodyComponent& body, const Vector3& angularImpulse) const;

    Matrix GetInverseInertia(const IPhysicsBodyComponent& body) const;

    void SetRestitution(const IPhysicsBodyComponent& body, float restitution) const;

    float GetRestitution(const IPhysicsBodyComponent& body) const;

    void SetFriction(const IPhysicsBodyComponent& body, float friction) const;

    float GetFriction(const IPhysicsBodyComponent& body) const;

    void SetGravityFactor(const IPhysicsBodyComponent& body, float gravityFactor) const;

    float GetGravityFactor(const IPhysicsBodyComponent& body) const;

    /**
     * @brief Effectuates a raycast and tests if it hits the joined body
     * @param component component against which to test for collision
     * @param ray the raycast
     * @return The result of the cast operation
     */
    CastResult TestRayCastAgainst(IPhysicsBodyComponent& component, const RayCast& ray);

    Handle GetEntityHandle(const JPH::BodyID& id);
    Handle GetSubShapeEntityHandle(const JPH::BodyID& id, const JPH::SubShapeID& subShapeID);

    void DeactivateBody(const IPhysicsBodyComponent& body);
    void ActivateBody(const IPhysicsBodyComponent& body);
#if 0
    bool ApplyBuoyancyImpulse(const IPhysicsBodyComponent& body, const Vector3& surfacePosition, const Vector3& surfaceNormal, float buoyancy, float linearDrag, float angularDrag,
                              const Vector3& fluidVelocity, const Vector3& gravity, float deltaTime);
    
    void SetUseManifoldReduction(const IPhysicsBodyComponent& body, bool useReduction);

    bool GetUseManifoldReduction(const IPhysicsBodyComponent& body) const;
#endif
};