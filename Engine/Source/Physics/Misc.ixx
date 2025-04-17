module;
#include "PhysicsSystem-Headers.hpp"
#include "Jolt/Physics/Collision/ObjectLayer.h"
#include "Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h"
export module Systems.Physics:Misc;
import Common;

// Todo refactor all of this to use arrays and custom layers when we have settings 

// Layer that objects can be in, determines which other objects it can collide with
// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
// but only if you do collision testing).
export enum struct Layers : JPH::ObjectLayer
{
    NON_MOVING,
    MOVING,
    NUM_LAYERS
};

export enum class MotionType : uint8_t
{
    STATIC = 0,
    KINEMATIC,
    DYNAMIC,
};

// Class that determines if two object layers can collide
export class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override;
};


// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
export class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
    BPLayerInterfaceImpl();

    virtual JPH::uint GetNumBroadPhaseLayers() const override;

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override;

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
    {
        switch ((BroadPhaseLayer::Type)inLayer)
        {
        case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:
            return "NON_MOVING";
        case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:
            return "MOVING";
        default: JPH_ASSERT(false);
            return "INVALID";
        }
    }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[static_cast<uint8_t>(Layers::NUM_LAYERS)];
};

// Class that determines if an object layer can collide with a broadphase layer
export class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override;
};

#ifdef JPH_ENABLE_ASSERTS

// Callback for asserts, connect this to your own assert handler if you have one
export inline bool AssertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, JPH::uint inLine)
{
    // Print to the TTY
    DebugPrint("Assertion failed: {}:{}: ({}): {}", inFile, inLine, inExpression, inMessage ? inMessage : "");

    // Breakpoint
    return true;
};

#endif // JPH_ENABLE_ASSERTS
