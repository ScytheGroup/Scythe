module;
#include "Common/Macros.hpp"
#include "Physics/PhysicsSystem-Headers.hpp"

#include "Jolt/Physics/Character/CharacterVirtual.h"
#include "Jolt/Physics/Collision/ContactListener.h"
export module Systems:CharacterContactListener;

import Serialization;

export class CharacterContactListener : public JPH::CharacterContactListener
{
public:
    // Delegates for all of them
    
    ~CharacterContactListener() override = default;
};
