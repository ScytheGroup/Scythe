module;

#include "Common/Macros.hpp"
#include "Serialization/SerializationMacros.h"

export module Sandbox:SpinComponent;
import Components;
import Scythe;
import SandboxGenerated;

export class SpinComponent : public Component
{
    SCLASS(SpinComponent, Component);
public:
    SpinComponent(const SpinComponent&) = default;
    SpinComponent();
    float rotationSpeed = 1.0f;
};

DECLARE_SERIALIZABLE(SpinComponent)