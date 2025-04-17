module;
#include "PhysicsSystem-Headers.hpp"

#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/ContactListener.h>
export module Systems.Physics:ContactListener;

import Common;

import Systems;
import :PhysicsBodyInterface;

// An example contact listener
export class ContactListener : public JPH::ContactListener
{
    struct ContactAddedData
    {
        JPH::BodyID Body1, Body2;
        JPH::ContactManifold Manifold;
        JPH::ContactSettings Settings;
    };
    
    struct ContactPersistedData
    {
        JPH::BodyID Body1, Body2;
        JPH::ContactManifold Manifold;
        JPH::ContactSettings Settings;
    };

    struct ContactRemovedData
    {
        JPH::BodyID Body1, Body2;
    };

    spinlock addedLock; 
    Array<ContactAddedData> addedContacts;
    
    spinlock persistedLock;
    Array<ContactPersistedData> persistedContacts;
    
    spinlock removedLock;
    Array<ContactRemovedData> removedContacts;
public:
    Delegate<EntityComponentSystem&, Handle, Handle, const JPH::ContactManifold&, const JPH::ContactSettings&> OnContactAddedDelegate;
    Delegate<EntityComponentSystem&, Handle, Handle, const JPH::ContactManifold&, const JPH::ContactSettings&> OnContactPersistedDelegate;
    Delegate<EntityComponentSystem&, Handle, Handle> OnContactRemovedDelegate;

    void EmitCallbacks(EntityComponentSystem& ecs);
    
protected:
    // See: ContactListener
    JPH::ValidateResult OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override;

    void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
    void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
    void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override;

};

// An example activation listener
export class ActivationListener : public JPH::BodyActivationListener
{
    struct BodyActivationData
    {
        JPH::BodyID bodyId;
        uint64_t userData;
    };

    spinlock activatedLock;
    Array<BodyActivationData> activations;

    spinlock deactivatedLock;
    Array<BodyActivationData> deactivations;
public:
    Delegate<IPhysicsBodyComponent&, uint64_t> OnContactActivatedDelegate;
    Delegate<IPhysicsBodyComponent&, uint64_t> OnContactDeactivatedDelegate;
    void EmitCallbacks(EntityComponentSystem& ecs);
protected:
    void OnBodyActivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override;
    void OnBodyDeactivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override;
};
