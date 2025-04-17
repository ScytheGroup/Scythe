module;
#include "Common/Macros.hpp"
#include "Serialization/SerializationMacros.h"
#include "Physics/PhysicsSystem-Headers.hpp"

#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Body/BodyID.h>

export module Systems.Physics:RigidBody;

import Common;
import Components;
import Generated;
import :PhysicsBodyInterface;
import :BodySettings;

import :Misc;

export class RigidBodyComponent : public Component, public IPhysicsBodyComponent
{
    SCLASS(RigidBodyComponent, Component);
    REQUIRES(TransformComponent);
    MANUALLY_DEFINED_EDITOR();
    friend class Physics;
    JPH::BodyID bodyId;
    JPH::ShapeRefC physicsShape;
public:
    PhysicsBodySettings bodySettings{};
    MotionType motionType = MotionType::STATIC;
    Layers layer = Layers::NON_MOVING;
    bool showDebug = false;

    RigidBodyComponent();
    RigidBodyComponent(const RigidBodyComponent& other);
    ~RigidBodyComponent() override;

    JPH::ShapeRefC GetPhysicsShape() override { return physicsShape; }
    void SetPhysicsShape(const JPH::ShapeRefC& newShape) override { physicsShape = newShape; }
    const JPH::BodyID& GetBodyId() const override { return bodyId; }
    void SetBodyId(const JPH::BodyID& newBodyId) override { bodyId = newBodyId; };
    
    MotionType GetMotionType() const override { return motionType; }
    Layers GetLayer() const override { return layer; }
    bool IsTrigger() const override { return false; }
    bool ShouldDisplayDebug() const override { return showDebug; }
    Handle GetEntityHandle() const override { return GetOwningEntity(); }
    const PhysicsBodySettings& GetCreationSettings() override { return bodySettings; }
};

DECLARE_SERIALIZABLE(RigidBodyComponent)