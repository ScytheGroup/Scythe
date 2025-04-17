module;

#include "Common/Macros.hpp"
#include "Serialization/SerializationMacros.h"

module Sandbox;
import :TestComponent;
import SandboxGenerated;

TestComponent::TestComponent()
{
}

DEFINE_SERIALIZATION_AND_DESERIALIZATION(TestComponent, uint32Test, floatTest, vector3Test, colorTest, meshTest, textureTest, materialTest);