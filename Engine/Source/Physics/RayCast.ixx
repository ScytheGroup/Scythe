module;
#include "PhysicsSystem-Headers.hpp"

#include "Jolt/Physics/Collision/RayCast.h"
#include "Jolt/Physics/Collision/CastResult.h"
export module Systems.Physics:RayCast;

import :Conversion;

import Common;

export
{
    /**
     * @brief RayCast object used with physics and bodyManipulation interface to execute a raycast
     */
    class RayCast : public JPH::RayCast
    {
        using JPH::RayCast::mDirection;
        using JPH::RayCast::mOrigin;
    public:

        /**
        * @param origin The origin of the ray
        * @param direction The direction and length of the ray
        */
        RayCast(Vector3 origin, Vector3 direction)
            : JPH::RayCast{Convert(origin), Convert(direction)}
        {}

        /**
        * @param origin The origin of the ray
        * @param direction The direction of the ray
        * @param length The length of the ray
        */
        RayCast(Vector3 origin, Vector3 direction, float length)
            : JPH::RayCast{Convert(origin), Convert(direction) * length}
        {}

        Vector3 GetOrigin() {return Convert(mOrigin);}
        Vector3 GetDirection() {return Convert(mDirection);}

        void SetOrigin(const Vector3& newOrigin)
        {
            mOrigin = Convert(newOrigin);
        }
        
        void SetDirection(const Vector3& newDirection)
        {
            mDirection = Convert(newDirection);
        }

        operator const JPH::RRayCast&() const
        {
            return JPH::RRayCast(*this);
        }
    };

    using JPH::RayCastSettings;
    using BackFaceMode = JPH::EBackFaceMode;

    struct CastResult
    {
        Handle hitEntity;
        Vector3 collisionPoint;
        Vector3 surfaceNormal;
        Handle collisionShapeEntity;
        const bool hit = false;
        
        // todo subshape and hit fraction
    };
}