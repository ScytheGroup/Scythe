module;

#include "Serialization/SerializationMacros.h"

module Sandbox:SpinComponent;

import :SpinComponent;

SpinComponent::SpinComponent()
{
}

DEFINE_SERIALIZATION_AND_DESERIALIZATION(SpinComponent, rotationSpeed);