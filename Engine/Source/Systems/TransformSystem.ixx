module;
#include "Common/Macros.hpp"
export module Systems:Hierarchy;

import Common;
import :System;

export class TransformSystem final : public System
{
public:
    TransformSystem() = default;
    ~TransformSystem() = default;
    void Init() override;
    void PostUpdate(double delta) override;

    void Reparent(Handle entity, Handle newParent, bool keepWorldTransform = true);
    void SetRelativeFromWorld(Handle entity, const Transform& world);
private:
    // Reparents the entity to the root of the scene by removing its ties to its previous parent
    // and removing it's ParentComponent
    void ReparentToRoot(Handle entity);

    // This links this entity to the newParent. This does not cleanup if entity was previously parented.
    // You should call ReparentToRoot before calling this
    void AttachToEntity(Handle entity, Handle newParent);

    // Checks the parents of lookupEntity until it finds fixedEntity. If it's found, there is a cycle in the graph
    bool HasCyclicDependency(Handle fixedEntity, Handle lookupEntity);
    
    void UpdateGlobalTransformsRecursive(Handle entity, EntityComponentSystem& ecs);
    void UpdateGlobalTransform(Handle entity, EntityComponentSystem& ecs);
};
