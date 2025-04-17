module;
#include "Common/Macros.hpp"
export module Sandbox:SpinSystem;
import Scythe;
import Systems;
import SandboxGenerated;

export class SpinSystem : public System 
{   
    SCLASS(SpinSystem, System);
public:
    void Init() override;
    void Update(double delta) override;
};


