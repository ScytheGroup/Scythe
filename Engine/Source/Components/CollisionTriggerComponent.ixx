module;
#include "Common/Macros.hpp"
#include "Serialization/SerializationMacros.h"
#include "Physics/PhysicsSystem-Headers.hpp"

#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Body/BodyID.h>

export module Systems.Physics:CollisionTriggerComponent;

import Common;
import Components;
import Generated;
import :PhysicsBodyInterface;

import :Misc;

export class CollisionTriggerComponent : public Component, public IPhysicsBodyComponent
{
    SCLASS(CollisionTriggerComponent, Component);
    REQUIRES(TransformComponent);
    MANUALLY_DEFINED_EDITOR();
    friend class Physics;
    JPH::BodyID bodyId;
    JPH::ShapeRefC physicsShape;
public:

    CollisionTriggerComponent();
    CollisionTriggerComponent(const CollisionTriggerComponent& other);
    ~CollisionTriggerComponent() override;

    JPH::ShapeRefC GetPhysicsShape() override { return physicsShape; }
    void SetPhysicsShape(const JPH::ShapeRefC& newShape) override { physicsShape = newShape; }
    const JPH::BodyID& GetBodyId() const override { return bodyId; }
    void SetBodyId(const JPH::BodyID& newBodyId) override { bodyId = newBodyId; };

    MotionType GetMotionType() const override { return MotionType::KINEMATIC; };
    Layers GetLayer() const override { return Layers::MOVING; };
    bool IsTrigger() const override { return true; }
    bool ShouldDisplayDebug() const override { return showDebug; }
    Handle GetEntityHandle() const override { return GetOwningEntity(); }
    const PhysicsBodySettings& GetCreationSettings() override
    {
        static PhysicsBodySettings defaultSettings{};
        return defaultSettings;
    }

    bool showDebug = false;
    std::unique_ptr<Shape> shapeSettings;
};

DECLARE_SERIALIZABLE(CollisionTriggerComponent)