module;
#include "Common/Macros.hpp"
#include "Serialization/SerializationMacros.h"
#include <atomic>
export module Core:EntityComponentSystem;

import :SceneSystem;
import Common;
import Graphics;
import Generated;
import Components;
import Systems;
import Tools;
import :SceneSettings;
import Serialization;
import Systems.Physics;
export import :EntityInfo;

export class Scene;

export using Entity = std::unordered_map<Type, Ref<Component>>;

export
class EntityComponentSystem final
{
    friend class Engine;
    friend Component;

    JobScheduler jobScheduler;
    // The level settings for the current scene
    SceneSettings sceneSettings;
    // Systems
    InputSystem inputs;
    std::unique_ptr<CameraSystem> cameraSystem;
    std::unique_ptr<Graphics> graphics;
    std::unique_ptr<SceneSystem> sceneSystem;
    

    Array<std::unique_ptr<System>> registeredSystems;

    Physics physics;
    TransformSystem transformSystem;
    InfoSetSystem infoSet;
    CullingSystem cullingSystem;

    // Entity Components
    std::unordered_map<Type, Array<Ref<Component>>> componentPool;
    StaticIndexedArray<Entity> entities;
    StaticIndexedArray<EntityInfo> infos;
    
    Delegate<Component&> componentRemovalDelegate;
    Delegate<Component&> componentChangedDelegate;
    Delegate<Component&> componentAddedDelegate;
    
    bool isSimulationActive;

    bool postInit = false;
public:
    EntityComponentSystem(class Window& window);
    ~EntityComponentSystem() = default;
    
    Handle AddEntity(const EntityInfo& info = {"", Guid{}});

    void ToggleSimulationActive() { isSimulationActive = !isSimulationActive; }
    void SetSimulationActive(bool newState) { isSimulationActive = newState; }
    bool IsSimulationActive() const noexcept { return isSimulationActive; }
    bool IsPostInit() const { return postInit; }

    void NotifyComponentChanged(Component& component);

    void RemoveComponent(Ref<Component> component);
    void RemoveEntity(Handle entityHandle);

    // Templated remove component
    template <class T>
    void RemoveComponent(Handle entityHandle)
    {
        if (!IsValidEntity(entityHandle))
            return;
        
        Type type = Type::GetType<T>();
        Entity& entity = GetEntity(entityHandle);
        auto it = entity.find(type);
        if (it != entity.end())
        {
            RemoveComponentFromPool(type, it->second->GetId());
            entity.erase(it);
        }
    }
    
    template <class T, class... TArgs> requires std::is_base_of_v<Component, T>
    constexpr T* AddComponent(Handle entityHandle, TArgs... args)
    {
        Type componentType = Type::GetType<T>();
        auto& entity = GetEntity(entityHandle);

        if (entity.contains(componentType))
        {
            DebugPrint("Warning: Component was already added to that entity");
            return reinterpret_cast<T*>(entity.at(componentType).Get());
        }

        // validate that every type in T::required_components was added to entity
        std::array types = typename T::required_components{}.types;
        for (Type type : types)
        {
            if (!entity.contains(type))
                DebugPrint("Warning: Component {} requires {} which was not added", Type::GetTypeName<T>().data(), type.GetName().data());
        }

        Ref<T> comp = MakeRef<T>(args...);
        comp->LateInitialize(entityHandle, static_cast<uint32_t>(componentPool[componentType].size()));
        entities[entityHandle][componentType] = comp;
        componentPool[componentType].push_back(comp);

        componentAddedDelegate.Execute(*comp);

        return comp.Get();
    }

    template <class T> requires std::is_base_of_v<Component, T>
    constexpr void AddComponentAndRequirements(Handle entityHandle)
    {
        Type componentType = Type::GetType<T>();
        const auto& entity = GetEntity(entityHandle);

        // validate that every type in T::required_components was added to entity
        std::array types = typename T::required_components{}.types;
        for (Type type : types)
        {
            if (!entity.contains(type))
            {
                AddComponent(type, entityHandle);
            }
        }

        if (!entity.contains(componentType))
        {
            Ref<T> comp = MakeRef<T>();
            comp->LateInitialize(entityHandle, static_cast<uint32_t>(componentPool[componentType].size()));
            entities[entityHandle][componentType] = comp;
            componentPool[componentType].push_back(comp);
            componentAddedDelegate.Execute(*comp);
        }
    }

    // reparents the entity to the new parent
    void Reparent(Handle entity, Handle newParent, bool keepWorldTransform = true);

    template <class T>
        requires std::is_base_of_v<Component, T>
    T* GetComponent(Handle entity)
    {
        return IsValidEntity(entity) ? GetComponent<T>(GetEntity(entity)) : nullptr;
    }

    template <class T>
        requires std::is_base_of_v<Component, T>
    T* GetOrCreateComponent(Handle entity)
    {
        if (auto comp = GetComponent<T>(entity); comp)
            return comp;
        return AddComponent<T>(entity);
    }
    
    template <class T>
        requires std::is_base_of_v<Component, T>
    T* GetComponent(Entity& entity)
    {
        Type type = Type::GetType<T>();
        if (auto it = entity.find(type); it != entity.end())
        {
            return it->second->Cast<T>();
        }
        return nullptr;
    }

    //  Returns a tuple if all components are found. Could be optimised
    template <class... TArgs>
    requires (std::is_base_of_v<Component, TArgs> && ...)
    std::optional<std::tuple<TArgs*...>> GetComponents(Handle entity)
    {
        if (!IsValidEntity(entity))
            return {};

        Entity& entityRef = GetEntity(entity);
        if ((entityRef.contains(Type::GetType<TArgs>()) && ...))
        {
            return std::make_tuple(entityRef.at(Type::GetType<TArgs>())->Cast<TArgs>()...);
        }
        return {};
    }

    [[nodiscard]]
    StaticIndexedArray<Entity>& GetEntities()
    {
        return entities;
    }

    /**
    * Returns all Entities that have T and TNeeded components. If one of them is null, it won't be returned.
     *
     * @param T The lookup component you're using. The function will use this component type as base to find the others.
     * @param TNeeded The other components required that will be found by the method.
     */
    template <class T, class... TNeeded>
    Array<std::tuple<T*, TNeeded*...>> GetArchetype()
    {
        Array<std::tuple<T*, TNeeded*...>> result;
        Type type = Type::GetType<T>();
        Array<Ref<Component>>& components = componentPool[type];

        for (auto& componentRef : components)
        {
            Ref<Component> component = componentRef;

#ifdef SC_DEV_VERSION
            Assert(component, "Archetype got a null component");
            Assert<std::string, size_t>(entities.HasValue(component->GetOwningEntity()), "Component of type {} found for removed entity {}", type.GetName().data(), component->GetOwningEntity());
#endif // SC_DEV_VERSION
            Entity& entity = GetEntity(component->GetOwningEntity());
            if (entity.contains(type))
            {
                bool hasAllComponents = true;
                if constexpr (sizeof...(TNeeded) > 0)
                {
                    hasAllComponents = (entity.contains(Type::GetType<TNeeded>()) && ...);
                }

                if (hasAllComponents)
                {
                    result.push_back(std::make_tuple(entity.at(type)->Cast<T>(), entity.at(Type::GetType<TNeeded>())->Cast<TNeeded>()...));
                }
            }
        }

        return result;
    }

    EntityInfo& GetEntityInfo(Handle handle);
    
    Entity& GetEntity(Handle handle)
    {
        return entities[handle];
    }

    template <class T>
        requires std::is_arithmetic_v<T>
    Entity& GetEntity(T handle)
    {
        return entities[static_cast<Handle>(handle)];
    }

    Handle FindEntity(const Guid& guid);

    Array<Component*> GetComponentsOfType(Type type);
    
    template <class T> requires std::is_base_of_v<Component, T>
    Array<T*> GetComponentsOfType()
    {
        Array<Component*> components = GetComponentsOfType(Type::GetType<T>());
        Array<T*> result;
        result.reserve(components.size());
        std::ranges::transform(components, std::back_inserter(result), [](Component* comp) { return comp->Cast<T>(); });
        return result;
    }

    SceneSettings& GetSceneSettings();

    void Reset();
    void LoadScene(const Scene& scene);
    [[nodiscard]] Scene SaveToScene();

    void PreInit();
    void Init();
    void Deinit();

    Delegate<std::string> sceneLoadedDelegate;


    bool IsValidEntity(Handle handle) const
    {
        return handle.IsValid() && entities.HasValue(handle);
    }

    // Custom ForEachSystem function that simulates foreach with every system
    void ForEachSystem(std::function<void(System&)> func);
private:
    static void RegisterComponent(Type type, std::function<Ref<Component>()> constructor);
    static void RegisterRequirements(Type type, std::vector<Type> requirements);
    static void RegisterComponentDeserializer(Type type, std::function<simdjson::error_code(Scene&, uint32_t, simdjson::simdjson_result<simdjson::ondemand::field>&)> deserializer);
    static void RegisterComponentSerializer(Type type, std::function<void(nlohmann::json&, Component*)> serializer);
public:

    template <class T> requires std::is_base_of_v<Component, T> && Reflection::IsReflected<T>
    static void RegisterComponent()
    {
        Type t = Type::GetType<T>();
        RegisterComponent(t, []() -> Ref<Component> { return MakeRef<T>(); });
        std::array requirements = typename T::required_components{}.types;
        RegisterRequirements(t, std::vector<Type>{requirements.begin(), requirements.end()});
        if constexpr (IsSerializable<T>::value)
        {
            RegisterComponentDeserializer(t, [](Scene& scene, uint32_t entityId, simdjson::simdjson_result<simdjson::ondemand::field>& field) -> simdjson::error_code
            {
                auto val = field.value().value();
                auto component = scene.AddComponent<T>(entityId);
                if (val.is_null()) return simdjson::SUCCESS;
                return DeserializeObject(val, *component);
            });
            RegisterComponentSerializer(t, [](nlohmann::json& json, Component* component)
            {
                SerializeObject(json[Type::GetTypeName<T>().data()], *(component)->Cast<T>());
            });
        }
    }

    static std::unordered_map<Type, std::function<simdjson::error_code(Scene&, uint32_t, simdjson::simdjson_result<simdjson::ondemand::field>&)>>& GetRegisteredDeserializers();
    static std::unordered_map<Type, std::function<void(nlohmann::json&, Component*)>>& GetRegisteredSerializers();

private:
    static void RegisterSystem(std::unique_ptr<System>&&);

public:
    template <class T> requires std::is_base_of_v<System, T>
    static void RegisterSystem();

private:
    static System* FindSystemInternal(const Array<std::unique_ptr<System>>& systems, const Type& t);
    static System* FindRegisteredSystem(const Type& type);
    System* FindSystem(const Type& type);

public:
    template <class T>
    T& GetSystem()
    {
        T* it = FindSystem(Type::GetType<T>())->Cast<T>();

        Assert(it, "System type not recognized");

        return *it;
    }

    template <>
    CameraSystem& GetSystem<CameraSystem>() { return *cameraSystem; }

    template <>
    Graphics& GetSystem<Graphics>() { return *graphics; }

    template <>
    SceneSystem& GetSystem<SceneSystem>() { return *sceneSystem; }

    template <>
    InputSystem& GetSystem<InputSystem>() { return inputs; }

    template <>
    Physics& GetSystem<Physics>() { return physics; }

    template <>
    TransformSystem& GetSystem<TransformSystem>() { return transformSystem; }

    template <class T>
    static void RegisterClass()
    {
        if constexpr (std::is_same_v<Component, T> || std::is_same_v<System, T>)
            return;
        else if constexpr (std::is_base_of_v<Component, T>)
            EntityComponentSystem::RegisterComponent<T>();
        else if constexpr (std::is_base_of_v<System, T>)
            EntityComponentSystem::RegisterSystem<T>();
        else
            return;
    }

    // Add component method that uses component reflection. Using this you need to setup you're parameters manually.
    void AddComponent(Type type, Handle handle);
    void EmplaceComponent(Type type, Handle handle, Ref<Component> component);
    void AddComponentAndRequirements(Type type, Handle handle);

    const Array<Type>& GetRegisteredComponentList() const;
private:
    void Update(double delta);
    void FixedUpdate(double delta);
    void UpdateGraphics(double delta);
    void PostUpdate(double delta);

    void RemoveComponentFromPool(Type componentType, uint32_t componentId);
    template <class... TRequired>
    void CheckRequiredComponents(Handle handle);

    void EmplaceComponentInternal(Type type, Handle handle, Ref<Component> component);
    void RegisterComponentDelegates();

#ifdef EDITOR
private:
    static const std::unordered_map<Type, std::vector<Type>>& GetRequirements();

    friend class Editor;
#endif
};

template <class T> requires std::is_base_of_v<System, T>
void EntityComponentSystem::RegisterSystem()
{
    if (FindRegisteredSystem(Type::GetType<T>()))
        return;

    RegisterSystem(std::make_unique<T>());
}

export using ECS = EntityComponentSystem;
