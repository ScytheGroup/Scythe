module;

#include "Serialization/SerializationMacros.h"

export module Core:Scene;
import Common;

import Generated;
import Components;

import Systems.Physics;

import Serialization;
import :SceneSettings;
import :EntityInfo;

export {
    
class Scene
{
public:
    Array<std::pair<EntityInfo, Array<std::unique_ptr<Component>>>> entities;
    SceneSettings sceneSettings;
    
    Scene() = default;
    Scene(const Scene& other);
    Scene(Scene&& other) = default;
    Scene& operator=(const Scene& other);
    Scene& operator=(Scene&& other) = default;

    Handle AddEntity();
    Handle AddEntity(const EntityInfo& info);

    template <class T, class... TArgs>
    T* AddComponent(Handle entityId, TArgs&&...);

    template <class T>
    void RemoveComponent(Handle entityId);
};

template <class T, class... TArgs>
T* Scene::AddComponent(Handle entityId, TArgs&&... args)
{
    auto& ent = entities[entityId].second;
    auto typeName = Type::GetType<T>();
    for (std::unique_ptr<Component>& comp : ent)
    {
        if (comp->GetType() == typeName)
            return comp->Cast<T>();
    }

    ent.push_back(std::make_unique<T>(std::forward<TArgs>(args)...));
    return ent.back()->Cast<T>();
}

template <class T>
void Scene::RemoveComponent(Handle entityId)
{
    if (entityId >= entities.size())
        return;

    for (Array<std::unique_ptr<Component>>::iterator it = entities[entityId].second.begin(); it != entities[entityId].second.end(); ++it)
    {
        if (Type::GetType<T>() == it->get()->GetType())
        {
            entities[entityId].second.fast_remove(it);
            return;
        }
    }
}

}
