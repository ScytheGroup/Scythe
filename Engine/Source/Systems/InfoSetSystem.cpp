module Systems;

import :InfoSet;

import Core;
import Components;

void InfoSetSystem::Init()
{
    System::Init();
    CleanupEntityHierarchy();
}

void InfoSetSystem::CleanupEntityHierarchy()
{
    auto& ecs = GetEcs();

    auto children = ecs.GetArchetype<ChildrenComponent>();
    for (auto [child] : children)
        child->children.clear();
    
    auto parentComps = ecs.GetArchetype<ParentComponent>();
    for (auto [parent] : parentComps)
    {       
        auto handle = ecs.FindEntity(parent->parentId);
        if (handle.IsValid())
        {
            parent->parent = handle;
            auto* childrenComp = ecs.GetOrCreateComponent<ChildrenComponent>(parent->parent);
            childrenComp->children.push_back(parent->GetOwningEntity());
        }
    }
}

void InfoSetSystem::Deinit()
{
    System::Deinit();
    auto& ecs = GetEcs();

    auto childrenComps = ecs.GetArchetype<ChildrenComponent>();
    for (auto [children] : childrenComps)
    {
        for (auto& child : children->children)
        {
            auto* parentComp = ecs.GetComponent<ParentComponent>(child);
            if (parentComp)
            {
                parentComp->parentId = ecs.GetEntityInfo(children->GetOwningEntity()).guid;
            }
        }
    }
}

void InfoSetSystem::PostUpdate(double delta)
{
    System::PostUpdate(delta);
    auto& ecs = GetEcs();

    // check that everybody has it's handle and that the guid is correct
    auto parentComps = ecs.GetArchetype<ParentComponent>();
    for (auto [parent] : parentComps)
    {
        if (parent->parent == Handle{})
        {
            if (!parent->parentId.IsValid())
                continue;

            auto handle = ecs.FindEntity(parent->parentId);
            if (handle != Handle{})
            {
                parent->parent = handle;
                auto* childrenComp = ecs.GetComponent<ChildrenComponent>(parent->parent);
                childrenComp->children.push_back(parent->GetOwningEntity());
            }
        }
        else
        {
            auto parentInfo = ecs.GetEntityInfo(parent->parent);
            if (parent->parentId == parentInfo.guid)
                continue;
            
            parent->parentId = parentInfo.guid;
        }
    }
    
}

void InfoSetSystem::OnComponentRemoval(Component& component)
{
    System::OnComponentRemoval(component);
    //TODO: Note once systems get global access to ecs, should probably take care of stuff when an entity loses children, parent.
}
