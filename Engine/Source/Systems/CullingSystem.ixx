export module Systems:CullingSystem;

import Common;
import :System;

export class CullingSystem : public System
{
public:
    void Update(double delta) override;
};
