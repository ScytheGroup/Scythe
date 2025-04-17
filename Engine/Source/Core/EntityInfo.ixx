module;
#include "Serialization/SerializationMacros.h"
export module Core:EntityInfo;

import std;
import Common;
import Serialization;

// EntityInfo is non component metadata owned by the ECS.
export struct EntityInfo
{
    // The unique name of the entity in the scene.
    std::string name = "Entity";
    Guid guid;
};

export struct EntityId
{
    
};

DECLARE_SERIALIZABLE(EntityInfo)