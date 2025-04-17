module;
#include "Common/Macros.hpp"
export module Sandbox:RocketComponent;
import Scythe;
import SandboxGenerated;

export class RocketComponent : public Component
{
    SCLASS(RocketComponent, Component);
public:    
    float desiredSpeed = 100;
    bool controlRocket = false;
};

DECLARE_SERIALIZABLE(RocketComponent);
DEFINE_SERIALIZATION_AND_DESERIALIZATION(RocketComponent, desiredSpeed, controlRocket)