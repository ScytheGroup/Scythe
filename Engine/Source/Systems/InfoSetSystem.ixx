module;
#include "Common/Macros.hpp"
export module Systems:InfoSet;

import :System;

export class InfoSetSystem : public System
{
    SCLASS(InfoSetSystem, System);
public:
    void Init() override;
    void Deinit() override;
    void PostUpdate(double delta) override;

    // Recreates the parent child components hierarchy
    void CleanupEntityHierarchy();

    void OnComponentRemoval(Component& component) override;
};
