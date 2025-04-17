module;

#include "Common/Macros.hpp"
#include "Serialization/SerializationMacros.h"

module Core:EntityComponentSystem;
import :EntityComponentSystem;
import :Window;
import Components;
import Common;
import :SceneSettings;
import :Scene;
import :SceneSystem;

namespace ECS_Internal
{
    static Array<Type>& GetRegisteredComponents()
    {
        static Array<Type> registeredComponents;
        return registeredComponents;
    }

    static std::unordered_map<Type, std::function<Ref<Component>()>>& GetComponentConstructors()
    {
        static std::unordered_map<Type, std::function<Ref<Component>()>> registeredComponents;
        return registeredComponents;
    }

    static std::unordered_map<Type, std::vector<Type>>& GetRequirements()
    {
        static std::unordered_map<Type, std::vector<Type>> requiredComponents;
        return requiredComponents;
    }

    static std::unordered_map<Type, std::function<void(nlohmann::json&, Component*)>>& GetComponentSerializers()
    {
        static std::unordered_map<Type, std::function<void(nlohmann::json&, Component*)>> registeredSerializers;
        return registeredSerializers;
    }

    static std::unordered_map<Type, std::function<simdjson::error_code(Scene&, uint32_t, simdjson::simdjson_result<simdjson::ondemand::field>&)>>& GetComponentDeserializers()
    {
        static std::unordered_map<Type, std::function<simdjson::error_code(Scene&, uint32_t, simdjson::simdjson_result<simdjson::ondemand::field>&)>> registeredDeserializers;
        return registeredDeserializers;
    }

    static Array<std::unique_ptr<System>>& GetRegisteredSystems()
    {
        static Array<std::unique_ptr<System>> registeredSystems;
        return registeredSystems;
    }
}

System* EntityComponentSystem::FindSystem(const Type& t)
{
    return FindSystemInternal(registeredSystems, t);
}

System* EntityComponentSystem::FindSystemInternal(const Array<std::unique_ptr<System>>& systems, const Type& t)
{
    auto it = std::ranges::find_if(systems, [&](auto&& systemPtr)
    {
        return systemPtr->GetType() == t;
    });

    if (it == systems.end())
        return nullptr;

    return it->get();
}

System* EntityComponentSystem::FindRegisteredSystem(const Type& t)
{
    return FindSystemInternal(ECS_Internal::GetRegisteredSystems(), t);
}

EntityComponentSystem::EntityComponentSystem(Window& window)
#if !EDITOR
    : isSimulationActive{true}
#else
    : isSimulationActive{false}
#endif
{
    DebugPrint("Starting modules initialisation.");
    cameraSystem = std::make_unique<CameraSystem>();

#ifdef RENDERDOC
    renderDoc = std::make_unique<RenderDoc>(window.GetInternalWindow());
#endif
    graphics = std::make_unique<Graphics>(window.GetInternalWindow(), window.GetWidth(), window.GetHeight());
    DebugPrint("Graphics module correctly initialised.");

    inputs.GetMouse().SetWindow(window.GetInternalWindow());
    DebugPrint("Input modules correctly initialised.");

    sceneSystem = std::make_unique<SceneSystem>();
    DebugPrint("SceneSystem correctly initialised.");

    DebugPrint("Modules initialisation completed.");

#ifdef IMGUI_ENABLED
    ImGuiManager::RegisterWindow("Scene settings", WindowCategories::SCENE, BindMethod(&SceneSettings::ImGuiDraw, &sceneSettings), true);
#endif
    
    RegisterComponentDelegates();

    // Not needed anymore?
    //RegisterComponent<CameraComponent>();
    //RegisterComponent<TransformComponent>();
    //RegisterComponent<RigidBodyComponent>();
    //RegisterComponent<MeshComponent>();
    //RegisterComponent<PunctualLightComponent>();
    //RegisterComponent<ParentComponent>();
    //RegisterComponent<ChildrenComponent>();
    //RegisterComponent<CollisionTriggerComponent>();
    //RegisterComponent<CollisionShapeComponent>();
    
    registeredSystems = std::exchange(ECS_Internal::GetRegisteredSystems(), {});
}

void EntityComponentSystem::EmplaceComponentInternal(Type type, Handle handle, Ref<Component> component)
{
    component->LateInitialize(handle, static_cast<uint32_t>(componentPool[type].size()));
    entities[handle][type] = component;
    componentPool[type].push_back(component);
    componentAddedDelegate.Execute(*component);
}

void EntityComponentSystem::RegisterComponentDelegates()
{
    ForEachSystem([this](System& system)
    {
        componentRemovalDelegate.Subscribe(BindMethod(&System::OnComponentRemoval, &system));
        componentChangedDelegate.Subscribe(BindMethod(&System::OnComponentChanged, &system));
        componentAddedDelegate.Subscribe(BindMethod(&System::OnComponentAdded, &system));
    });
}

std::unordered_map<Type, std::function<simdjson::error_code(Scene&, uint32_t, simdjson::simdjson_result<simdjson::ondemand::field>&)>>& EntityComponentSystem::GetRegisteredDeserializers()
{
    return ECS_Internal::GetComponentDeserializers();
}

std::unordered_map<Type, std::function<void(nlohmann::json&, Component*)>>& EntityComponentSystem::GetRegisteredSerializers()
{
    return ECS_Internal::GetComponentSerializers();
}

void EntityComponentSystem::RegisterSystem(std::unique_ptr<System>&& system)
{
    ECS_Internal::GetRegisteredSystems().push_back(std::move(system));
}

template <class... TRequired>
void EntityComponentSystem::CheckRequiredComponents(Handle handle)
{
    auto entity = GetEntity(handle);
    // for each type, if entity does not have one, add it
    for (Type type : { TRequired::GetType()... })
    {
        if (entity.contains(type))
        {
            AddComponent<TRequired>(handle);
        }
    }
}

Handle EntityComponentSystem::AddEntity(const EntityInfo& info)
{
    auto newHandle = Handle{entities.Emplace()};
    auto newName = info.name.empty() ? std::format("Entity {}", newHandle.id) : info.name;
    auto newGuid = !info.guid.IsValid() ? Guid::Create() : info.guid;
    infos.Emplace(newName, newGuid);
    return newHandle;
}

void EntityComponentSystem::PreInit()
{
    ForEachSystem([this](System& system) { system.PreInit(*this); });
}

void EntityComponentSystem::Init()
{
    PreInit();

    infoSet.Init();
    transformSystem.Init();
    physics.Init();
    inputs.Init();
    cameraSystem->Init();
    sceneSystem->Init();
    graphics->Init();
    cullingSystem.Init();

    for (auto& system : registeredSystems)
    {
        system->Init();
    }

    postInit = true;
}

void EntityComponentSystem::Deinit()
{
    // nothing to deinit if it never was
    if (!postInit)
        return;
    ForEachSystem([this](System& system) { system.Deinit(); });
    postInit = false;
}

EntityInfo& EntityComponentSystem::GetEntityInfo(Handle handle)
{
    return infos[handle];
}

Handle EntityComponentSystem::FindEntity(const Guid& guid)
{
    auto it = std::find_if(infos.begin(), infos.end(), [&guid] (const EntityInfo& info)
    {
        return info.guid == guid;
    });

    return it == infos.end() ? Handle{} : Handle{it.AbsoluteIndex()};
}

SceneSettings& EntityComponentSystem::GetSceneSettings()
{
    return sceneSettings;
}

void EntityComponentSystem::Reset()
{
    Deinit();
    entities.Clear();
    infos.Clear();
    componentPool.clear();
    sceneSettings = {};
}

Array<Component*> EntityComponentSystem::GetComponentsOfType(Type type)
{
    if (!componentPool.contains(type))
        return {};

    Array<Ref<Component>>& comps = componentPool[type];
    Array<Component*> ret;
    ret.reserve(comps.size());
    for (size_t i = 0; i < comps.size(); ++i)
    {
        if (auto comp = comps[i]; comp)
        {
            ret.push_back(comp.Get());
        }
        else
        {
            RemoveComponentFromPool(type, i);
        }
    }
    return ret;
}

void EntityComponentSystem::ForEachSystem(std::function<void(System&)> func)
{
    func(transformSystem);
    func(infoSet);
    func(physics);
    func(inputs);
    func(cullingSystem);
    func(*cameraSystem);
    func(*sceneSystem);
    func(*graphics);

    for (auto& system : registeredSystems)
    {
        func(*system);
    }
}

void EntityComponentSystem::RegisterComponent(Type type, std::function<Ref<Component>()> constructor)
{
    // if can find do nothing
    if (std::ranges::find(ECS_Internal::GetRegisteredComponents(), type) != ECS_Internal::GetRegisteredComponents().end())
        return;
    
    ECS_Internal::GetRegisteredComponents().push_back(type);
    ECS_Internal::GetComponentConstructors().emplace(type, constructor);
}

void EntityComponentSystem::RegisterRequirements(Type type, std::vector<Type> requirements)
{
    ECS_Internal::GetRequirements().insert({ type, requirements });
}

void EntityComponentSystem::RegisterComponentSerializer(Type type, std::function<void(nlohmann::json&, Component*)> serializer)
{
    // if can find do nothing
    if (ECS_Internal::GetComponentSerializers().contains(type))
        return;
    
    ECS_Internal::GetComponentSerializers().insert({ type, serializer });
}

void EntityComponentSystem::RegisterComponentDeserializer(Type type, std::function<simdjson::error_code(Scene&, uint32_t, simdjson::simdjson_result<simdjson::ondemand::field>&)> deserializer)
{
    // if can find do nothing
    if (ECS_Internal::GetComponentDeserializers().contains(type))
        return;
    
    ECS_Internal::GetComponentDeserializers().insert({ type, deserializer });
}

void EntityComponentSystem::AddComponent(Type type, Handle handle)
{
    if (entities.HasValue(handle) && entities[handle].contains(type))
    {
        return;
    }

    auto& constructors = ECS_Internal::GetComponentConstructors();
    if (constructors.contains(type))
    {
        EmplaceComponentInternal(type, handle, constructors[type]());
    }
}

void EntityComponentSystem::EmplaceComponent(Type type, Handle handle, Ref<Component> component)
{
    if (entities.HasValue(handle) && entities[handle].contains(type))
    {
        return;
    }
    
    EmplaceComponentInternal(type, handle, component);
}

void EntityComponentSystem::AddComponentAndRequirements(Type type, Handle handle)
{
    if (auto it = ECS_Internal::GetRequirements().find(type); it != ECS_Internal::GetRequirements().end())
    {
        for (Type requiredType : it->second)
        {
            AddComponent(requiredType, handle);
        }
    }
    AddComponent(type, handle);
}

const Array<Type>& EntityComponentSystem::GetRegisteredComponentList() const 
{
    return ECS_Internal::GetRegisteredComponents();
}

void EntityComponentSystem::Update(double delta)
{
    sceneSystem->Update(delta);
    physics.Update(delta);
    transformSystem.Update(delta);
    inputs.Update(delta);
    cameraSystem->Update(delta);
    cullingSystem.Update(delta);

    if (!isSimulationActive)
        return;
    
    for (auto& system : registeredSystems)
    {
        system->Update(delta);
    }
}

void EntityComponentSystem::FixedUpdate(double delta)
{
    if (isSimulationActive)
    {
        physics.FixedUpdate(delta);
        transformSystem.FixedUpdate(delta);

        for (auto& system : registeredSystems)
        {
            system->FixedUpdate(delta);
        }
    }

    inputs.FixedUpdate(delta);
    cameraSystem->FixedUpdate(delta);
    cullingSystem.FixedUpdate(delta);
}

void EntityComponentSystem::UpdateGraphics(double delta)
{
    graphics->Update(delta);
}

void EntityComponentSystem::PostUpdate(double delta)
{
    ForEachSystem([this, delta](System& system) { system.PostUpdate(delta); });
}

void EntityComponentSystem::RemoveComponentFromPool(Type componentType, uint32_t componentId)
{
    auto& componentsOfType = componentPool[componentType];
    if (componentsOfType.size() <= 0)
        return;
    auto oldComp = componentsOfType[componentId];
    componentRemovalDelegate.Execute(*oldComp);
    uint32_t movedComp = componentsOfType.fast_remove(componentId);
    if (auto components = componentsOfType; components.size() > movedComp)
        componentsOfType[movedComp]->componentId = movedComp;
}

void EntityComponentSystem::NotifyComponentChanged(Component& component)
{
    componentChangedDelegate.Execute(component);
}

void EntityComponentSystem::RemoveComponent(Ref<Component> component)
{
    Type type = component->GetType();
    Entity& entity = GetEntity(component->GetOwningEntity());
    auto componentId = component->GetId();
    if (entity.contains(type))
    {
        RemoveComponentFromPool(type, componentId);
        entity.erase(type);
    }
}

void EntityComponentSystem::RemoveEntity(Handle entityHandle)
{
    if (!IsValidEntity(entityHandle))
    {
        DebugPrint("Tried removing {} more than once", entityHandle);
        return;
    }
    
    {
        // removed all of the children recursively
        Entity& entityToRemove = GetEntity(entityHandle);
        if (entityToRemove.contains(Type::GetType<ChildrenComponent>()))
        {
            ChildrenComponent* childrenComponent = entityToRemove[Type::GetType<ChildrenComponent>()]->Cast<ChildrenComponent>();
            auto copy = childrenComponent->children;
            for (Handle child : copy)
            {
                RemoveEntity(child);
            }
        }
    }

    {
        // Need to re-get entity here because removing child might have moved the pointer data and create a dangling ref
        Entity& entityToRemove = GetEntity(entityHandle);
        if (entityToRemove.contains(Type::GetType<ParentComponent>()))
        {
            if (ParentComponent* parentComponent = entityToRemove[Type::GetType<ParentComponent>()]->Cast<ParentComponent>())
            {
                ChildrenComponent* parentChildren = GetComponent<ChildrenComponent>(parentComponent->parent);
                if (const auto it = std::ranges::find(parentChildren->children, entityHandle); it != parentChildren->children.end())
                    parentChildren->children.erase(it);
            }
        }
    
        for (auto& [type, compPtr] : entityToRemove)
        {
            RemoveComponentFromPool(type, compPtr->GetId());
        }

        entityToRemove.clear();
        entities.Remove(entityHandle);
        infos.Remove(entityHandle);
    }
}

void EntityComponentSystem::Reparent(Handle entity, Handle newParent, bool keepWorldTransform)
{
    transformSystem.Reparent(entity, newParent, keepWorldTransform);
}

// Loads a scene as current.
void EntityComponentSystem::LoadScene(const Scene& scene)
{
    Reset();

    sceneSettings = scene.sceneSettings;

    // Actual loading of the scene
    {
        infos.Reserve(scene.entities.size());
        entities.Reserve(scene.entities.size());
        for (size_t i = 0; i < scene.entities.size(); ++i)
        {
            auto handle = AddEntity({scene.entities.at(i).first});
            for (const std::unique_ptr<Component>& component : scene.entities.at(i).second)
            {
                Type componentType = component->GetType();
                Ref<Component> comp = (entities[handle][componentType] = component->Copy<Component>().release());
                comp->LateInitialize(handle, componentPool[componentType].size());
                componentPool[componentType].push_back(comp);
                componentAddedDelegate.Execute(*comp);
            }
        }
    }

    sceneLoadedDelegate.Execute(scene.sceneSettings.worldName);
    // Todo : We should make sure wether systems should be reinitialized after loading a scene.
    // Maybe just a targetted reset function simulating the first frame or smth of the sort
    Init();
}

Scene EntityComponentSystem::SaveToScene()
{
    Scene scene;
    scene.sceneSettings = sceneSettings;

    Array<std::pair<EntityInfo, Array<std::unique_ptr<Component>>>>& entitiesList = scene.entities;
    
    entitiesList.reserve(entities.Size());

    auto i = 0;
    for (auto it = entities.begin(); it != entities.end(); ++it)
    {
        auto& entity = *it;
#ifdef EDITOR
        if (entity.contains(Type::GetType<EditorCameraComponent>()))
            continue;
#endif
        
        auto& newEntity = entitiesList.emplace_back();
        newEntity.first = infos[it.AbsoluteIndex()];
        for (auto& [type,component] : entity)
        {
            newEntity.second.push_back(component->Copy<Component>());
        }
        
        ++i;
    }

    return scene;
}

#ifdef EDITOR
const std::unordered_map<Type, std::vector<Type>>& EntityComponentSystem::GetRequirements()
{
    return ECS_Internal::GetRequirements();
}
#endif
