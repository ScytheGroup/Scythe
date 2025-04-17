module Systems;

import Core;
import Components;
import :Hierarchy;

// Global note : most of this code assumes that a parent will always have a transform component 

void TransformSystem::Init()
{
    System::Init();

    // For backwards compatibility
    PostUpdate(0.0f);
}

void TransformSystem::PostUpdate(double delta)
{
    // Update all transforms
    // uses the relative transforms (TransformComponent) to write the global transform
    auto& ecs = GetEcs();

    auto archetype = ecs.GetArchetype<TransformComponent>();
    for (auto& [transform] : archetype)
    {
        // if the owning entity doesn't have a global transform, create one
        if (!ecs.GetComponent<GlobalTransform>(transform->GetOwningEntity()))
        {
            ecs.AddComponent<GlobalTransform>(transform->GetOwningEntity());
        }

        // if not a root parent continue, else update all children
        if (auto parent = ecs.GetComponent<ParentComponent>(transform->GetOwningEntity());
            parent && parent->parent != Handle{})
        {
            continue;
        }

        UpdateGlobalTransformsRecursive(transform->GetOwningEntity(), ecs);
    }
}

Transform GetParentGlobalTransform(Handle entity, EntityComponentSystem& ecs)
{
    // get first parent with global transform
    auto parent = ecs.GetComponent<ParentComponent>(entity);
    while (parent && parent->parent != Handle{})
    {
        auto globalTransform = ecs.GetComponent<GlobalTransform>(parent->parent);
        if (globalTransform)
        {
            return globalTransform->transform;
        }
        parent = ecs.GetComponent<ParentComponent>(parent->parent);
    }

    return Transform{};
}

// Change the parenting and update the relative transform to be the same global transform as it used to have
void TransformSystem::Reparent(const Handle entity, const Handle newParent, bool keepWorldTransform)
{
    auto& ecs = GetEcs();

    ReparentToRoot(entity);
    
    Transform previousGlobalTransform{};
    bool hasTransform = false;
    if (GlobalTransform* transformComp = ecs.GetComponent<GlobalTransform>(entity))
    {
        previousGlobalTransform = transformComp->transform;
        hasTransform = true;
    } 
    
    if (newParent.IsValid())
    {
        if (!HasCyclicDependency(entity, newParent)) {
            AttachToEntity(entity, newParent);
        }
        else
        {
            DebugPrint("Warning : Cyclic dependency detected, aborting reparenting.");
        }
    }

    if (hasTransform)
    {
        SetRelativeFromWorld(entity, previousGlobalTransform);
    }
    UpdateGlobalTransformsRecursive(entity, ecs);
}

void TransformSystem::SetRelativeFromWorld(Handle entity, const Transform& world)
{
    auto& ecs = GetEcs();
    
    auto transform = ecs.GetComponent<TransformComponent>(entity);
    if (!transform) return;

    transform->transform = world;

    // Try get the parent
    auto parent = ecs.GetComponent<ParentComponent>(entity);
    while (parent && parent->parent != Handle{})
    {
        auto parentGlobalTransform = ecs.GetComponent<GlobalTransform>(parent->parent);
        if (!parentGlobalTransform)
        {
            parent = ecs.GetComponent<ParentComponent>(parent->parent);
            continue;
        }

        transform->transform = world.GetRelativeTransform(parentGlobalTransform->transform);
           
        return;
    }
    UpdateGlobalTransformsRecursive(entity, ecs);
}

void TransformSystem::ReparentToRoot(Handle entity)
{
    auto& ecs = GetEcs();
    if (ParentComponent* parentComponent = ecs.GetComponent<ParentComponent>(entity))
    {
        if (ChildrenComponent* childrenComponent = ecs.GetComponent<ChildrenComponent>(parentComponent->parent))
        {
            auto foundIt = std::ranges::find(childrenComponent->children, entity);
            if (foundIt != childrenComponent->children.end())
                childrenComponent->children.erase(foundIt);
            
            if (childrenComponent->children.empty())
            {
                ecs.RemoveComponent<ChildrenComponent>(parentComponent->parent);
            }                
        }
        ecs.RemoveComponent<ParentComponent>(entity);
    }
}

void TransformSystem::AttachToEntity(Handle entity, Handle newParent)
{
    auto& ecs = GetEcs();
    
    ParentComponent* parentComponent = ecs.GetOrCreateComponent<ParentComponent>(entity);
    auto entityInfo = ecs.GetEntityInfo(newParent);
    
    parentComponent->parent = newParent;
    parentComponent->parentId = entityInfo.guid;

    ChildrenComponent* childrenComponent = ecs.GetOrCreateComponent<ChildrenComponent>(newParent);
    childrenComponent->children.push_back(entity);
}

bool TransformSystem::HasCyclicDependency(Handle fixedEntity, Handle lookupEntity)
{
    auto& ecs = GetEcs();
    // Check for cyclic dependencies and return if found
    auto newParentComponent = ecs.GetComponent<ParentComponent>(lookupEntity);
    while (newParentComponent && newParentComponent->parent.IsValid())
    {
        if (newParentComponent->parent == fixedEntity)
        {
            return true;
        }
        newParentComponent = ecs.GetComponent<ParentComponent>(newParentComponent->parent);
    }
    return false;
}

void TransformSystem::UpdateGlobalTransformsRecursive(Handle entity, EntityComponentSystem& ecs)
{
    if (auto transform = ecs.GetComponent<TransformComponent>(entity))
    {
        transform->transform.rotation.Normalize();
        auto globalTransform = ecs.GetComponent<GlobalTransform>(entity);
        if (!globalTransform)
        {
            globalTransform = ecs.AddComponent<GlobalTransform>(entity);
        }
        
        auto parent = ecs.GetComponent<ParentComponent>(entity);
        if (parent && parent->parent != Handle{})
        {
            GlobalTransform* parentGlobalTransform;
            do
            {
                parentGlobalTransform = ecs.GetComponent<GlobalTransform>(parent->parent);
            } while (!parentGlobalTransform && ((parent = ecs.GetComponent<ParentComponent>(parent->parent))));
            Transform parentT = parentGlobalTransform ? parentGlobalTransform->transform : Transform{};
            
            *const_cast<Transform*>(&globalTransform->transform) = transform->transform * parentT;
        }
        else
        {
            *const_cast<Transform*>(&globalTransform->transform) = transform->transform;
        }
        const_cast<Transform*>(&globalTransform->transform)->rotation.Normalize();
        
        //UpdateGlobalTransform(entity, ecs);
    }
    
    auto children = ecs.GetComponent<ChildrenComponent>(entity);
    if (!children)
        return;
    
    for (auto& child : children->children)
    {
        UpdateGlobalTransformsRecursive(child, ecs);
    }
}

void TransformSystem::UpdateGlobalTransform(Handle entity, EntityComponentSystem& ecs)
{
    auto* transformComp = ecs.GetComponent<TransformComponent>(entity);
    auto* worldTransform = ecs.GetComponent<GlobalTransform>(entity);

    if (!transformComp)
        return;
    
    ParentComponent* parent = ecs.GetComponent<ParentComponent>(entity);
    auto resultTransform = transformComp->transform;
    
    while (parent && parent->parent.IsValid())
    {
        resultTransform *= ecs.GetComponent<TransformComponent>(parent->parent)->transform;
        parent = ecs.GetComponent<ParentComponent>(parent->parent);
    }
    
    *const_cast<Transform*>(&worldTransform->transform) = resultTransform;    
}
