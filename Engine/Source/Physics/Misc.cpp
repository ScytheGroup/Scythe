module;
#include "PhysicsSystem-Headers.hpp"

module Systems.Physics;

using namespace JPH;

bool ObjectLayerPairFilterImpl::ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const
{
    switch (inObject1)
    {
    case std::to_underlying(Layers::NON_MOVING):
        return inObject2 == std::to_underlying(Layers::MOVING); // Non moving only collides with moving
    case std::to_underlying(Layers::MOVING):
        return true; // Moving collides with everything
    default:
        Assert(false);
        return false;
    }
}

// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
namespace BroadPhaseLayers
{
static constexpr BroadPhaseLayer NON_MOVING(0);
static constexpr BroadPhaseLayer MOVING(1);
static constexpr uint32_t NUM_LAYERS(2);
};

BPLayerInterfaceImpl::BPLayerInterfaceImpl()
{
    // Create a mapping table from object to broad phase layer
    mObjectToBroadPhase[static_cast<uint32_t>(Layers::NON_MOVING)] = BroadPhaseLayers::NON_MOVING;
    mObjectToBroadPhase[static_cast<uint32_t>(Layers::MOVING)] = BroadPhaseLayers::MOVING;
}

uint BPLayerInterfaceImpl::GetNumBroadPhaseLayers() const
{
    return BroadPhaseLayers::NUM_LAYERS;
}

BroadPhaseLayer BPLayerInterfaceImpl::GetBroadPhaseLayer(ObjectLayer inLayer) const
{
    Assert(inLayer < std::to_underlying(Layers::NUM_LAYERS));
    return mObjectToBroadPhase[inLayer];
}

bool ObjectVsBroadPhaseLayerFilterImpl::ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const
{
    switch (inLayer1)
    {
    case std::to_underlying(Layers::NON_MOVING):
        return inLayer2 == BroadPhaseLayers::MOVING;
    case std::to_underlying(Layers::MOVING):
        return true;
    default:
        Assert(false);
        return false;
    }
}
