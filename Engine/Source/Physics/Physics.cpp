module;
#include "Common/Macros.hpp"
#include "PhysicsSystem-Headers.hpp"
// Jolt includes
#include <cstdarg>
#include <fstream>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include "Jolt/Core/StreamWrapper.h"
#include "Jolt/Physics/Collision/CastResult.h"
#include "Jolt/Physics/Collision/RayCast.h"
#include "Jolt/Physics/Collision/Shape/StaticCompoundShape.h"
#include "Serialization/SerializationMacros.h"

module Systems.Physics;

import Generated;
import :RigidBody;
import Core;

import :Misc;
import :ContactListener;

// Jolt includes
// STL includes
import <iostream>;
import <thread>;

using JPH::Body;
using JPH::BodyID;
using JPH::Quat;
using JPH::RVec3;
using JPH::Vec3;

// Callback for traces
static void TraceImpl(const char* inFMT, ...)
{
    // Format the message
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);

    DebugPrint("{}", buffer);
}

constexpr uint32_t allocatorSize = 10u * 1024u * 1024u;

namespace Physics_Private
{
static void FetchShapesRecursive(EntityComponentSystem& ecs, ::Array<std::pair<Transform, ::Shape*>>& outShapes, Handle currentEntity, const Transform& relativeTransform)
{
    Transform relative = relativeTransform;
    if (auto* transformComponent = ecs.GetComponent<TransformComponent>(currentEntity))
    {
        relative = transformComponent->transform * relative;
    }

    if (CollisionShapeComponent* shape = ecs.GetComponent<CollisionShapeComponent>(currentEntity))
    {
        if (shape->shapeSettings)
        {
            shape->shapeSettings->storedHandle = shape->GetOwningEntity();
            outShapes.push_back({ relative, shape->shapeSettings.get() });
        }
    }

    if (ChildrenComponent* childComponent = ecs.GetComponent<ChildrenComponent>(currentEntity))
    {
        for (auto childEntity : childComponent->children)
        {
            FetchShapesRecursive(ecs, outShapes, childEntity, relative);
        }
    }
}

Array<std::tuple<IPhysicsBodyComponent*, GlobalTransform*>> GetAllPhysicsComponentsAndTransforms(EntityComponentSystem& ecs)
{
    Array<std::tuple<IPhysicsBodyComponent*, GlobalTransform*>> entities;

    auto rigidBodies = ecs.GetArchetype<RigidBodyComponent, GlobalTransform>();
    auto triggers = ecs.GetArchetype<CollisionTriggerComponent, GlobalTransform>();

    entities.reserve(rigidBodies.size() + triggers.size());

    std::ranges::transform(triggers, std::back_inserter(entities), [](auto pair) { return pair; });
    std::ranges::transform(rigidBodies, std::back_inserter(entities), [](auto pair) { return pair; });

    return entities;
}

void RemoveAndDestroyBody(JPH::BodyInterface* bodyInterface, IPhysicsBodyComponent& rb)
{
    if (rb.GetBodyId() != BodyID())
    {
        bodyInterface->RemoveBody(rb.GetBodyId());
        bodyInterface->DestroyBody(rb.GetBodyId());
        rb.SetBodyId(BodyID{});
    }
}

// Looks through the parent chain for the first RigidBody (which is the one this shape affects)
RigidBodyComponent* GetParentRigidBody(EntityComponentSystem& ecs, Handle entity)
{
    if (auto rigidBody = ecs.GetComponent<RigidBodyComponent>(entity))
        return rigidBody;

    if (auto current = ecs.GetComponent<ParentComponent>(entity))
        return GetParentRigidBody(ecs, current->parent);

    return nullptr;
}

inline auto& TryGetShape(const JPH::ShapeSettings::ShapeResult& result)
{
    if (!result.IsValid())
        Assert<std::string>(false, "Physics component : shapes creation failed ({})", result.GetError().c_str());

    return result.Get();
}

JPH::ShapeRefC GetScaledBodyShape(IPhysicsBodyComponent& rb, const Transform& transform)
{
    // initialize the bodies
    if (JPH::ShapeRefC originalShape = rb.GetPhysicsShape())
    {
        JPH::ShapeRefC endShape = originalShape;
        JPH::Shape::ShapeResult shapeResult;
        if (transform.scale != Vector3::One)
        {
            return TryGetShape(originalShape->ScaleShape(Convert(transform.scale)));
        }

        return originalShape;
    }

    return nullptr;
}

void InitializeComponent(EntityComponentSystem& ecs, RigidBodyComponent& rb)
{
    Handle entityId = rb.GetOwningEntity();
    Array<std::pair<Transform, Shape*>> ret;

    TransformComponent* transformComponent = ecs.GetComponent<TransformComponent>(entityId);

    FetchShapesRecursive(ecs, ret, entityId, transformComponent->transform.Inverse());

    JPH::StaticCompoundShapeSettings settings;

    for (auto& [relativeTransform, shape] : ret)
    {
        if (shape && shape->GetSettings())
        {
            if (shape->CanCreateShape())
            {
                JPH::Ref<JPH::Shape> shapeRef = TryGetShape(shape->Create());
                JPH::Ref<JPH::Shape> scaledShape = TryGetShape(shapeRef->ScaleShape(Convert(relativeTransform.scale)));
                // needed to find entity handle back when attempting to find a subshape.
                settings.AddShape(Convert(relativeTransform.position), Convert(relativeTransform.rotation), scaledShape, shape->storedHandle);
            }
            else
            {
                DebugPrint("Warning : Shape could not be created it will not be considered");
            }
        }
    }

    rb.SetPhysicsShape(!settings.mSubShapes.empty() ? TryGetShape(settings.Create()) : nullptr);
}

void InitializeComponent(EntityComponentSystem& ecs, CollisionTriggerComponent& rb)
{
    if (!rb.shapeSettings)
        return;
    JPH::Shape::ShapeResult shape = rb.shapeSettings->Create();
    if (shape.IsValid())
        shape.Get()->SetUserData(rb.GetOwningEntity());
    rb.SetPhysicsShape(shape.IsValid() ? shape.Get() : nullptr);
}

template<class ComponentType> requires IsPhysicsBodyComponent<ComponentType>
void ConfigureNewEntities(JPH::BodyInterface* bodyInterface, EntityComponentSystem& ecs, Array<Handle>& handles)
{
    // Set up the bodies in the scene
    Array<std::tuple<ComponentType*, GlobalTransform*>> entities;
    entities.reserve(handles.size());

    for (Handle handle : handles)
    {
        std::optional<std::tuple<ComponentType*, GlobalTransform*>> tuple = ecs.GetComponents<ComponentType, GlobalTransform>(handle);
        if (tuple.has_value())
        {
            entities.push_back(*tuple);
        }
        else
        {
            DebugPrint("RigidBody for handle {} found which has no transform. Forcibly removing the component", handle);
            ecs.RemoveComponent<ComponentType>(handle);
        }
    }
    handles.clear();
        
    Array<JPH::BodyID> bodiesToAdd;
    bodiesToAdd.reserve(entities.size());

    Array<JobHandle> jobs;
    jobs.reserve(entities.size());
    spinlock addLock;
    Barrier barrier;

    JobScheduler& scheduler = JobScheduler::Get();
    for (auto [body, transform] : entities)
    {
        jobs.emplace_back(scheduler.CreateJob([&, body, transform] {
            InitializeComponent(ecs, *body);

            if (JPH::ShapeRefC finalShape = GetScaledBodyShape(*body, transform->transform))
            {
                // Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
                JPH::BodyCreationSettings bodySettings(finalShape, Convert(transform->position), Convert(transform->rotation), JPH::EMotionType{ std::to_underlying(body->GetMotionType()) },
                                                       JPH::ObjectLayer{ std::to_underlying(body->GetLayer()) });

                bodySettings.mUserData = body->GetOwningEntity();    // Create the actual rigid body
                    if (body->IsTrigger())
                    {
                        bodySettings.mIsSensor = true;
                        bodySettings.mCollideKinematicVsNonDynamic = true;    
                    }
// Note: Jolt also uses sequence numbers for their id's so this method is risky but works for now
                const PhysicsBodySettings& customSettings = body->GetCreationSettings();
                    bodySettings.mAllowedDOFs = static_cast<JPH::EAllowedDOFs>(customSettings.degreesOfFreedom);
                    bodySettings.mMotionQuality = customSettings.motionQuality;
                    bodySettings.mFriction = customSettings.friction;
                    bodySettings.mRestitution = customSettings.restitution;
                    if (customSettings.mass != 0)
                    {
                        bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
                        bodySettings.mMassPropertiesOverride.mMass = customSettings.mass;    
                    }
                    
                    DebugPrint("Body created with id {}", body->GetOwningEntity());
                    JPH::Body* physicsBody = bodyInterface->CreateBody(bodySettings);
                    //bodyInterface->CreateBodyWithID(JPH::BodyID(body->GetOwningEntity()), bodySettings);
                    Assert(physicsBody, "Body creation failed");
                    body->SetBodyId(physicsBody->GetID());
                    std::unique_lock uniqueLock{ addLock };
                    bodiesToAdd.push_back(body->GetBodyId());
                }
            }));
        }

    barrier.AddJobs(jobs);
    barrier.Wait();

    if (bodiesToAdd.size())
    {
        int bodiesToAddCount = static_cast<int>(bodiesToAdd.size());
        auto addState = bodyInterface->AddBodiesPrepare(bodiesToAdd.data(), bodiesToAddCount);
        bodyInterface->AddBodiesFinalize(bodiesToAdd.data(), bodiesToAddCount, addState, JPH::EActivation::Activate);
    }
}

template <IsPhysicsBodyComponent ComponentType>
void RemoveBodiesOfType(EntityComponentSystem& ecs, JPH::BodyInterface* bodyInterface)
{
    auto entities = ecs.GetArchetype<ComponentType, GlobalTransform>();
    if (entities.empty())
        return;

    auto bodiesRange = entities
        | std::views::transform([](const std::tuple<ComponentType*, GlobalTransform*>& pair) { return std::get<0>(pair)->GetBodyId(); })
        | std::views::filter([](const JPH::BodyID& bodyId) { return !bodyId.IsInvalid(); });

    Array<JPH::BodyID> bodiesToRemove{ bodiesRange.begin(), bodiesRange.end() };

    if (bodiesToRemove.size())
    {
        bodyInterface->RemoveBodies(bodiesToRemove.data(), bodiesToRemove.size());
        bodyInterface->DestroyBodies(bodiesToRemove.data(), bodiesToRemove.size());
    }
}
} // namespace Physics_Private

void Physics::ConfigureNewBodiesIfAny(EntityComponentSystem& ecs)
{
    if (newRigidbodies.size())
        Physics_Private::ConfigureNewEntities<RigidBodyComponent>(bodyInterface, ecs, newRigidbodies);
    if (newTriggers.size())
        Physics_Private::ConfigureNewEntities<CollisionTriggerComponent>(bodyInterface, ecs, newTriggers);
}

void Physics::RecreateBody(JPH::BodyInterface* bodyInterface, EntityComponentSystem& ecs, IPhysicsBodyComponent& bodyComponent)
{
    Physics_Private::RemoveAndDestroyBody(bodyInterface, bodyComponent);

    if (bodyComponent.IsTrigger())
    {
        newTriggers.push_back_unique(bodyComponent.GetEntityHandle());
        Physics_Private::ConfigureNewEntities<CollisionTriggerComponent>(bodyInterface, ecs, newTriggers);
    }
    else
    {
        newRigidbodies.push_back_unique(bodyComponent.GetEntityHandle());
        Physics_Private::ConfigureNewEntities<RigidBodyComponent>(bodyInterface, ecs, newRigidbodies);
    }
}

std::unique_ptr<JPH::TempAllocator> Physics::CreateTempAllocator()
{
    return std::make_unique<JPH::TempAllocatorImpl>(allocatorSize);
}

Physics::Physics()
    : bodyManipulationInterface(*this)
{
    // Jolt uses malloc. It can be changed if needed.
    JPH::RegisterDefaultAllocator();

    JPH::Trace = TraceImpl;
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;)

    // Create a factory, this class is responsible for creating instances of classes based on their name or hash and is mainly used for deserialization of saved data.
    // It is not directly used in this example but still required.
    JPH::Factory::sInstance = new JPH::Factory();

    // Register all physics types with the factory and install their collision handlers with the CollisionDispatch class.
    // If you have your own custom shape types you probably need to register their handlers with the CollisionDispatch before calling this function.
    // If you implement your own default material (PhysicsMaterial::sDefault) make sure to initialize it before this function or else this function will create one for you.
    JPH::RegisterTypes();

    tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(allocatorSize);

    // We need a job system that will execute physics jobs on multiple threads. Typically
    // you would implement the JobSystem interface yourself and let Jolt Physics run on top
    // of your own job scheduler. JobSystemThreadPool is an example implementation.
    // JobSystemThreadPool job_system(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);

    // Now we can create the actual physics system.
    physicsSystem.Init(maxBodies, numBodyMutexes, maxBodyPairs, maxContactConstraints, broadPhaseLayerInterface, objectVsBroadphaseLayerFilter, objectVsObjectLayerFilter);

    // A body activation listener gets notified when bodies activate and go to sleep
    // Note that this is called from a job so whatever you do here needs to be thread safe.
    // Registering one is entirely optional.
    physicsSystem.SetBodyActivationListener(&bodyActivationListener);

    // A contact listener gets notified when bodies (are about to) collide, and when they separate again.
    // Note that this is called from a job so whatever you do here needs to be thread safe.
    // Registering one is entirely optional.
    physicsSystem.SetContactListener(&contactListener);

    // The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
    // variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
    bodyInterface = &physicsSystem.GetBodyInterface();

    // Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance.
    // You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
    // Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
    physicsSystem.OptimizeBroadPhase();
}

Physics::~Physics()
{
    // Unregisters all types with the factory and cleans up the default material
    JPH::UnregisterTypes();

    // Destroy the factory
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
}

void Physics::Init()
{
    auto& ecs = GetEcs();

    // Set up the world settings like the gravity
    SceneSettings& sceneSettings = ecs.GetSceneSettings();
    physicsSystem.SetGravity(Convert(sceneSettings.physics.gravity));
    ConfigureNewBodiesIfAny(ecs);
}

void Physics::Deinit()
{
    auto& ecs = GetEcs();

    Physics_Private::RemoveBodiesOfType<RigidBodyComponent>(ecs, bodyInterface);
    Physics_Private::RemoveBodiesOfType<CollisionTriggerComponent>(ecs, bodyInterface);

    contactListener.OnContactAddedDelegate.Clear();
    contactListener.OnContactRemovedDelegate.Clear();
    contactListener.OnContactPersistedDelegate.Clear();
}

void Physics::FixedUpdate(double delta)
{
    auto& ecs = GetEcs();

    ConfigureNewBodiesIfAny(ecs);

    auto entities = Physics_Private::GetAllPhysicsComponentsAndTransforms(ecs);

    for (auto [body, transform] : entities)
    {
        // Update the position with the new one if it has changed
        bodyInterface->SetPositionAndRotationWhenChanged(body->GetBodyId(), Convert(transform->position), Convert(transform->rotation), JPH::EActivation::Activate);
    }

    // If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
    const int cCollisionSteps = 1; // int(ceil(delta * 60.0));

    // Step the world
    physicsSystem.Update(static_cast<float>(delta), cCollisionSteps, tempAllocator.get(), &jobSystemWrapper);

    // Now update our positions back with the new calculated ones
    for (auto [body, transform] : entities)
    {
        Transform newTransform = transform->transform;
        RVec3 position = bodyInterface->GetPosition(body->GetBodyId());
        newTransform.position = Convert(position);

        Quat rotation = bodyInterface->GetRotation(body->GetBodyId());
        newTransform.rotation = Convert(rotation);
        ecs.GetSystem<TransformSystem>().SetRelativeFromWorld(transform->GetOwningEntity(), newTransform);
    }

    contactListener.EmitCallbacks(ecs);
    bodyActivationListener.EmitCallbacks(ecs);
}

#ifdef EDITOR

class CustomBodyDrawFilter : public JPH::BodyDrawFilter
{
    std::unordered_set<JPH::BodyID> bodyToDraw;

public:
    CustomBodyDrawFilter(EntityComponentSystem& ecs)
    {
        const auto& rigidBodies = ecs.GetComponentsOfType<RigidBodyComponent>();
        const auto& triggers = ecs.GetComponentsOfType<CollisionTriggerComponent>();

        bodyToDraw.insert_range(triggers | std::views::filter([](auto comp) { return comp->ShouldDisplayDebug(); }) | std::views::transform([](auto comp) { return comp->GetBodyId(); }));
        bodyToDraw.insert_range(rigidBodies | std::views::filter([](auto comp) { return comp->ShouldDisplayDebug(); }) | std::views::transform([](auto comp) { return comp->GetBodyId(); }));
    }

    bool ShouldDraw(const Body& inBody) const override { return bodyToDraw.contains(inBody.GetID()); };
};

void Physics::DrawDebug(EntityComponentSystem& ecs, JPH::DebugRenderer* renderer, bool showAll)
{
    if (!ecs.IsSimulationActive())
    {
        auto entities = Physics_Private::GetAllPhysicsComponentsAndTransforms(ecs);

        for (auto [body, transform] : entities)
        {
            // Update the position with the new one if it has changed
            bodyInterface->SetPositionAndRotationWhenChanged(body->GetBodyId(), Convert(transform->position), Convert(transform->rotation), JPH::EActivation::Activate);
        }
    }

    JPH::BodyManager::DrawSettings settings;
    settings.mDrawVelocity = true;
    settings.mDrawMassAndInertia = true;
    settings.mDrawShape = true;

    if (showAll)
    {
        physicsSystem.DrawBodies(settings, renderer);
    }
    else
    {
        CustomBodyDrawFilter filter(ecs);
        physicsSystem.DrawBodies(settings, renderer, &filter);
    }
}
#endif

void Physics::OnComponentAdded(Component& component)
{
    System::OnComponentAdded(component);
    if (auto rb = component.Cast<RigidBodyComponent>())
    {
        newRigidbodies.push_back_unique(rb->GetOwningEntity());
    }
    if (auto rb = component.Cast<CollisionTriggerComponent>())
    {
        newTriggers.push_back_unique(rb->GetOwningEntity());
    }
}

void Physics::OnComponentChanged(Component& component)
{
    auto& ecs = GetEcs();

    System::OnComponentChanged(component);

    if (component.GetType() == Type::GetType<CollisionShapeComponent>() || component.GetType() == Type::GetType<RigidBodyComponent>()
        || component.GetType() == Type::GetType<TransformComponent>() || component.GetType() == Type::GetType<GlobalTransform>())
    {
        if (RigidBodyComponent* rb = Physics_Private::GetParentRigidBody(ecs, component.GetOwningEntity()))
        {
            RecreateBody(bodyInterface, ecs, *rb);
        }
    }

    if (CollisionTriggerComponent* trigger = component.Cast<CollisionTriggerComponent>())
    {
        RecreateBody(bodyInterface, ecs, *trigger);
    }
}

void Physics::OnComponentRemoval(Component& component)
{
    System::OnComponentRemoval(component);
    if (auto rb = component.Cast<RigidBodyComponent>())
    {
        Physics_Private::RemoveAndDestroyBody(bodyInterface, *rb);
    }

    if (auto trigger = component.Cast<CollisionTriggerComponent>())
    {
        Physics_Private::RemoveAndDestroyBody(bodyInterface, *trigger);
    }
}

CastResult Physics::GetCastResult(const JPH::RayCastResult& result, const RayCast& cast) const
{
    Vector3 contactPoint{Convert(cast.GetPointOnRay(result.mFraction))};

    JPH::BodyLockRead lock{ physicsSystem.GetBodyLockInterface(), result.mBodyID };
    if (lock.Succeeded())
    {
        const JPH::Body& body = lock.GetBody();
        Vector3 contactNormal = Convert(body.GetWorldSpaceSurfaceNormal(result.mSubShapeID2, Convert(contactPoint)));
        
        return CastResult{ body.GetUserData(), contactPoint, contactNormal, body.GetShape()->GetSubShapeUserData(result.mSubShapeID2), true };
    }
    return {};
}

CastResult Physics::CastRay(const RayCast& cast, const BodyFilter& bodyFilter, const BroadPhaseLayerFilter& broadPhaseLayerFilter, const ObjectLayerFilter& objectLayerFilter)
{
    JPH::RayCastResult castResult;
    JPH::RRayCast jCast = cast;
    bool hit = physicsSystem.GetNarrowPhaseQuery().CastRay(jCast, castResult, broadPhaseLayerFilter, objectLayerFilter, bodyFilter);

    if (!hit)
        return {};

    return GetCastResult(castResult, cast);
}

class Physics::MutlipleHitCastRayCollector : public JPH::CastRayCollector
{
    const Physics& p;
    const RayCast& cast;
    Array<CastResult> results;
public:
    MutlipleHitCastRayCollector(const Physics& p, const RayCast& cast)
        : p{ p }, cast{ cast }
    {}
    
    void AddHit(const JPH::RayCastResult& inResult) override
    {
        results.push_back(p.GetCastResult(inResult, cast));
    }

    Array<CastResult> GetResults() const
    {
        return results;
    }
};

Array<CastResult> Physics::CastRayUninterrupted(const RayCast& cast, const BodyFilter& bodyFilter, const BroadPhaseLayerFilter& broadPhaseLayerFilter, const ObjectLayerFilter& objectLayerFilter)
{
    MutlipleHitCastRayCollector collector{ *this, cast };
    JPH::RRayCast jCast = cast;
    physicsSystem.GetNarrowPhaseQuery().CastRay(jCast, {},collector, broadPhaseLayerFilter, objectLayerFilter, bodyFilter);
    return collector.GetResults();
}

BEGIN_SERIALIZATION(JPH::Vec3)
    SerializeObject(json, Convert(SER_VAL));
END_SERIALIZATION()

BEGIN_DESERIALIZATION(JPH::Vec3)
std::vector<float> vec;
PARSE_VARIABLE(vec);
PARSED_VAL = JPH::Vec3{ vec[0], vec[1], vec[2] };
END_DESERIALIZATION()
