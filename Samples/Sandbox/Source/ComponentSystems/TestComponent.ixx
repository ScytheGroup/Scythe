module;

#include "Common/Macros.hpp"
#include "Serialization/SerializationMacros.h"

export module Sandbox:TestComponent;
import Components;
import Scythe;
import SandboxGenerated;

export class TestComponent : public Component
{
    SCLASS(TestComponent, Component);
public:
    TestComponent(const TestComponent&) = default;
    TestComponent();
    
    uint32_t uint32Test = 8;
    float floatTest = 10.0f;

    AssetRef<Texture> textureTest;
    AssetRef<Mesh> meshTest;
    AssetRef<Material> materialTest;

    Vector3 vector3Test;
    Color colorTest;
};

DECLARE_SERIALIZABLE(TestComponent)
