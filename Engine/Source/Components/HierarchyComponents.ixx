module;
#include "Common/Macros.hpp"
#include "Serialization/SerializationMacros.h"
export module Components:Hierarchy;

import Serialization;
import :Component;

export class ChildrenComponent : public Component
{
    SCLASS(ChildrenComponent, Component);

public:
    Array<Handle> children;
};

export class ParentComponent : public Component
{
    SCLASS(ParentComponent, Component);

public:
    Handle parent;
    Guid parentId;
};


DECLARE_SERIALIZABLE(ParentComponent);
DECLARE_SERIALIZABLE(Handle);
