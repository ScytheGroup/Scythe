module;
#include "Common/Macros.hpp"
export module Sandbox:RocketSystem;

import SandboxGenerated;
import Scythe;

export class RocketSystem : public System
{
    SCLASS(RocketSystem, System);
public:

    void Update(double delta) override;
};
