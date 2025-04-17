module;
#include <Common/Macros.hpp>
export module Sandbox:MainCharacterSystem;

import Scythe;
import SandboxGenerated;

export class MainCharacterSystem : public System
{
    SCLASS(MainCharacterSystem, System);

public:
    void Init() override;
    void Update(double delta) override;
};