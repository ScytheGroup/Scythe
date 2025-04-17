module;
#include "Common/Macros.hpp"
#include "PhysicsSystem-Headers.hpp"

#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/PhysicsSystem.h>

#ifdef SC_DEV_VERSION
#include "Jolt/Renderer/DebugRendererSimple.h"

#include "Jolt/Renderer/DebugRendererRecorder.h"
#endif
export module Systems.Physics;

import Systems;
import :JobSystemWrapper;
import :ContactListener;

export import :Conversion;
export import :Misc;
export import :Shapes;
import :ContactListener;
export import :RigidBody;
export import :ShapeComponent;
export import :CollisionTriggerComponent;
export import :PhysicsBodyInterface;
export import :BodyManipulationInterface;
export import :RayCast;
export import :IgnoreBodiesFilter;
export import :BodySettings;

export namespace JPH
{
using JPH::AABox;
using JPH::Body;
using JPH::BodyFilter;
using JPH::BodyID;
using JPH::BroadPhaseLayerFilter;
using JPH::Color;
using JPH::ColorArg;
using JPH::ContactManifold;
using JPH::ContactSettings;
using JPH::EActivation;
using JPH::Mat44;
using JPH::ObjectLayerFilter;
using JPH::Real;
using JPH::Real3;
using JPH::Ref;
using JPH::RefTargetVirtual;
using JPH::RMat44;
using JPH::RMat44Arg;
using JPH::RVec3;
using JPH::RVec3Arg;
using JPH::SubShapeIDPair;
using JPH::Triangle;
using JPH::Vec3;
using JPH::Vec4;
#ifdef SC_DEV_VERSION
using JPH::DebugRenderer;
using JPH::DebugRendererSimple;
#endif
// Define real to float
} // namespace JPH

export 
{
    using JPH::BodyID;
    using JPH::BodyInterface;
    using JPH::EActivation;
    using JPH::BroadPhaseLayerFilter;
    using JPH::BodyFilter;
    using JPH::ObjectLayerFilter;
    using JPH::BroadPhaseLayer;
    using JPH::ContactSettings;
    using JPH::ContactManifold;
}

export template <class T>
concept IsPhysicsBodyComponent = std::is_base_of_v<Component, T> and std::is_base_of_v<IPhysicsBodyComponent, T>;


export class Physics final : public System
{
    friend BodyManipulationInterface;
    friend class CharacterControllerSystem;
    
    JobSystemWrapper jobSystemWrapper;
    JPH::BodyInterface* bodyInterface = nullptr;

    // We need a temp allocator for temporary allocations during the physics update. We're
    // pre-allocating 10 MB to avoid having to do allocations during the physics update.
    // B.t.w. 10 MB is way too much for this example but it is a typical value you can use.
    // If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
    // malloc / free.
    std::unique_ptr<JPH::TempAllocator> tempAllocator = nullptr;
    JPH::PhysicsSystem physicsSystem;

    // Create mapping table from object layer to broadphase layer
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    BPLayerInterfaceImpl broadPhaseLayerInterface;

    // Create class that filters object vs broadphase layers
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    ObjectVsBroadPhaseLayerFilterImpl objectVsBroadphaseLayerFilter;

    // Create class that filters object vs object layers
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    ObjectLayerPairFilterImpl objectVsObjectLayerFilter;

    BodyManipulationInterface bodyManipulationInterface;

public:
    ActivationListener bodyActivationListener;
    ContactListener contactListener;

    // This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
    // Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
    static constexpr uint32_t maxBodies = 65536;

    // This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
    static constexpr uint32_t numBodyMutexes = 0;

    // This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
    // body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
    // too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
    // Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
    static constexpr uint32_t maxBodyPairs = 65536;

    // This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
    // number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
    // Note: This value is low because this is a simple test. For a real project use something in the order of 10240.
    static constexpr uint32_t maxContactConstraints = 10240;

    /**
    * @brief Casts a ray and will return the closest hit, if found. Faster than cast ray generic but only evaluates first hit
    * @param cast The ray to cast
    * @param bodyFilter Filter to apply to the bodies
    * @param broadPhaseLayerFilter Optional filter to apply to the broadphase layers
    * @param objectLayerFilter Optional filter to apply to the object layers
    */
    CastResult CastRay(const RayCast& cast, const BodyFilter& bodyFilter = {}, const BroadPhaseLayerFilter& broadPhaseLayerFilter = {}, const ObjectLayerFilter& objectLayerFilter = {});
    /**
     * @brief Cast ray and accumulates all bodies collided with.
     * @param cast The ray to cast
     * @param bodyFilter a body filter that you can join to filter which body is collided with
     * @param broadPhaseLayerFilter Optional filter to apply to the broadphase layers
     * @param objectLayerFilter Optional filter to apply to the object layers
     * @return The array of cast results obtained 
     */
    Array<CastResult> CastRayUninterrupted(const RayCast& cast, const BodyFilter& bodyFilter = {}, const BroadPhaseLayerFilter& broadPhaseLayerFilter = {}, const ObjectLayerFilter& objectLayerFilter = {});
    class MutlipleHitCastRayCollector;
private:
    Array<Handle> newRigidbodies;
    Array<Handle> newTriggers;
    // Creates and configures any rigidbody and triggers that have been queued up in the system
    void ConfigureNewBodiesIfAny(EntityComponentSystem& ecs);
    void RecreateBody(JPH::BodyInterface* bodyInterface, EntityComponentSystem& ecs, IPhysicsBodyComponent& bodyComponent);

    CastResult GetCastResult(const JPH::RayCastResult& result, const RayCast& cast) const;

public:
    Physics();
    ~Physics() override;

    void Init() override;
    void Deinit() override;
    void FixedUpdate(double delta) override;
    void OnComponentAdded(Component& component) override;
    void OnComponentChanged(Component& component) override;
    void OnComponentRemoval(Component& component) override;
    BodyManipulationInterface& GetBodyManipulationInterface() { return bodyManipulationInterface; }

    static std::unique_ptr<JPH::TempAllocator> CreateTempAllocator();
    
public:
#if EDITOR
    void DrawDebug(EntityComponentSystem& ecs, JPH::DebugRenderer* renderer, bool drawAll = false);
#endif
};
