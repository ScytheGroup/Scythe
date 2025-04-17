module;
#include "PhysicsSystem-Headers.hpp"

#include "Jolt/Physics/Body/Body.h"
#include "Jolt/Physics/Body/BodyFilter.h"
#include "Jolt/Physics/Body/BodyLockInterface.h"
export module Systems.Physics:IgnoreBodiesFilter;
import Common;
import std;

export class IgnoreBodiesFilter final : public JPH::BodyFilter
{
public:
    explicit IgnoreBodiesFilter(const Handle id) : handlesToIgnore({id}) {}
    explicit IgnoreBodiesFilter(const std::span<Handle> handlesToIgnore) : handlesToIgnore(handlesToIgnore.begin(), handlesToIgnore.end()) {}
    explicit IgnoreBodiesFilter(const auto&... handlesToIgnore) : handlesToIgnore({handlesToIgnore...}) {}
    inline bool ShouldCollide(const JPH::BodyID& inBodyID) const override
    {
        return !handlesToIgnore.contains(inBodyID.GetIndexAndSequenceNumber());
    }

    bool ShouldCollideLocked(const JPH::Body& body) const override
    {
        Handle handle = body.GetUserData();
        return !handlesToIgnore.contains(handle);
    }
    
    std::set<Handle> handlesToIgnore;
};