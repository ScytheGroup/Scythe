module;
#include <Common/Macros.hpp>
export module Systems:System;
export import Generated;

import Common;

export class EntityComponentSystem;

export class System : public Base
{
    SCLASS(System, Base);
    friend EntityComponentSystem;
    EntityComponentSystem* ecs = nullptr;
    void PreInit(EntityComponentSystem& inEcs)
    {
        ecs = &inEcs;
    }
protected:
    EntityComponentSystem& GetEcs();
    const EntityComponentSystem& GetEcs() const;
public:
    virtual void Update(double delta) {};
    virtual void FixedUpdate(double delta) {};
    virtual void Init() {}
    virtual void Deinit() {}
    virtual void PostUpdate(double delta) {}
    // Todo maybe theres a better way to do this?
    virtual void OnComponentAdded(Component& component) {};
    virtual void OnComponentChanged(Component& component) {};
    virtual void OnComponentRemoval(Component& component) {};
};

