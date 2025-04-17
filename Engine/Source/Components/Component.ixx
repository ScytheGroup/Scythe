module;

#include "Common/Macros.hpp"

export module Components:Component;
import Common;
export import Generated;

export struct ComponentInitializer
{
    const Handle entityHandle;
    uint32_t componentId;
};

export template <class... TArgs>
TypeHolder<TArgs...> RequireComponents();

export template <class... TArgs, class... TArgs2>
TypeHolder<TArgs..., TArgs2...> RequireComponents(TypeHolder<TArgs2...>);


template <class... TArgs>
const std::array<Type, sizeof...(TArgs)> GetRequiredTypes()
{
    return { Type::GetType<TArgs>()... };
}

export class Component : public Base
{
    SCLASS(Component, Base);
    friend class EntityComponentSystem;
    friend class Scene;
    Handle entityHandle = 0;
    uint32_t componentId = 0;
protected:
    bool isInitialized = false;
    // public constructor is needed for internal systems but should never be used.
    Component() = default;
    Component(const Component&);
    void LateInitialize(Handle newHandle, uint32_t newComponentId);

public:
    using required_components = decltype(RequireComponents<>());

    Handle GetOwningEntity() const { return entityHandle; }
    uint32_t GetId() const { return componentId; };
};