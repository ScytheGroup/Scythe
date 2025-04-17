module;
#include "PhysicsSystem-Headers.hpp"

#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/ContactListener.h>
module Systems.Physics;
import Core;

std::map<Handle, IPhysicsBodyComponent*> GetPhysicsBodyComponentMap(EntityComponentSystem& ecs)
{
    std::map<Handle, IPhysicsBodyComponent*> lookup;
    
    Array<RigidBodyComponent*> rigibodiesComp = ecs.GetComponentsOfType<RigidBodyComponent>();
    for (auto& comp : rigibodiesComp)
        lookup.insert({comp->GetOwningEntity(), comp});

    Array<CollisionTriggerComponent*> triggerComp = ecs.GetComponentsOfType<CollisionTriggerComponent>();
    for (auto& comp : triggerComp)
        lookup.insert({comp->GetOwningEntity(), comp});

    return lookup;
}

JPH::ValidateResult ContactListener::OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult)
{
    // Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
    return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
}

void ContactListener::OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& inSettings)
{
    std::lock_guard _{addedLock};
    addedContacts.push_back(ContactAddedData{ inBody1.GetID(), inBody2.GetID(), inManifold, inSettings });
}

void ContactListener::OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& inSettings)
{
    std::lock_guard _{persistedLock};
    persistedContacts.push_back(ContactPersistedData{ inBody1.GetID(), inBody2.GetID(), inManifold, inSettings });
}

void ContactListener::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair)
{
    std::lock_guard _{removedLock};
    removedContacts.push_back(ContactRemovedData{ inSubShapePair.GetBody1ID(), inSubShapePair.GetBody2ID() });
}

void ContactListener::EmitCallbacks(EntityComponentSystem& ecs)
{
    auto lookup = GetPhysicsBodyComponentMap(ecs);

    Physics& p = ecs.GetSystem<Physics>();
    
    {
        std::lock_guard _{addedLock};
        for (const auto& contact : addedContacts)
        {
            auto h1 = p.GetBodyManipulationInterface().GetEntityHandle(contact.Body1);
            auto h2 = p.GetBodyManipulationInterface().GetEntityHandle(contact.Body2);
            OnContactAddedDelegate.Execute(ecs, h1, h2, contact.Manifold, contact.Settings);
        }
        addedContacts.clear();
    }

    {
        std::lock_guard _{persistedLock};
        for (const auto& contact : persistedContacts)
        {
            auto h1 = p.GetBodyManipulationInterface().GetEntityHandle(contact.Body1);
            auto h2 = p.GetBodyManipulationInterface().GetEntityHandle(contact.Body2);
            OnContactPersistedDelegate.Execute(ecs, h1, h2, contact.Manifold, contact.Settings);
        }
        persistedContacts.clear();
    }

    {
        std::lock_guard _{removedLock};
        for (const auto& contact : removedContacts)
        {
            auto h1 = p.GetBodyManipulationInterface().GetEntityHandle(contact.Body1);
            auto h2 = p.GetBodyManipulationInterface().GetEntityHandle(contact.Body2);
            OnContactRemovedDelegate.Execute(ecs, h1, h2);
        }
        removedContacts.clear();
    }
}

void ActivationListener::OnBodyActivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData)
{
    std::lock_guard _{activatedLock};
    activations.push_back(BodyActivationData{ inBodyID, inBodyUserData });
}

void ActivationListener::OnBodyDeactivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData)
{
    std::lock_guard _{deactivatedLock};
    deactivations.push_back(BodyActivationData{ inBodyID, inBodyUserData });
}

void ActivationListener::EmitCallbacks(EntityComponentSystem& ecs)
{
    auto lookup = GetPhysicsBodyComponentMap(ecs);
    Physics& p = ecs.GetSystem<Physics>();

    {
        std::lock_guard _{activatedLock};
        for (const auto& contact : activations)
        {
            OnContactActivatedDelegate.Execute(*lookup[p.GetBodyManipulationInterface().GetEntityHandle(contact.bodyId)], contact.userData);
        }
        activations.clear();
    }

    {
        std::lock_guard _{deactivatedLock};
        for (const auto& contact : deactivations)
        {
            OnContactDeactivatedDelegate.Execute(*lookup[p.GetBodyManipulationInterface().GetEntityHandle(contact.bodyId)], contact.userData);
        }
        deactivations.clear();
    }
}