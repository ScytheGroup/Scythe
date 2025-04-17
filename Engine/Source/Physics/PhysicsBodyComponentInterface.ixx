module;

#include "Physics/PhysicsSystem-Headers.hpp"
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Body/BodyID.h>

export module Systems.Physics:PhysicsBodyInterface;

import :Misc;
import :BodySettings;

export struct IPhysicsBodyComponent
{
    virtual Handle GetEntityHandle() const = 0;
    virtual JPH::ShapeRefC GetPhysicsShape() = 0;
    virtual void SetPhysicsShape(const JPH::ShapeRefC&) = 0;
    
    virtual const JPH::BodyID& GetBodyId() const = 0;
    virtual void SetBodyId(const JPH::BodyID&) = 0;
    
    virtual MotionType GetMotionType() const = 0;
    virtual Layers GetLayer() const = 0;
    virtual bool IsTrigger() const = 0;
    virtual bool ShouldDisplayDebug() const = 0;
    virtual const PhysicsBodySettings& GetCreationSettings() = 0;
    virtual ~IPhysicsBodyComponent() = default;
};
