module;
#include "Serialization/SerializationMacros.h"

module Core;

import :Scene;
import Serialization;

Scene::Scene(const Scene& other)
{
    entities.clear();
    sceneSettings = other.sceneSettings;
    for (auto& otherEntity : other.entities)
    {
        entities.emplace_back();
        auto& entity = entities.back();
        entity.first = otherEntity.first;
        entity.second.reserve(otherEntity.second.size());

        for (const std::unique_ptr<Component>& component : otherEntity.second)
        {
            std::unique_ptr<Component> componentPtr = component->Copy<Component>();
            Assert(componentPtr != nullptr, "Copy operation returned nullptr");
            entity.second.push_back(std::move(componentPtr));
        }
    }
}

Scene& Scene::operator=(const Scene& other)
{
    entities.clear();
    sceneSettings = other.sceneSettings;
    for (auto& otherEntity : other.entities)
    {
        entities.emplace_back();
        auto& entity = entities.back();
        entity.first = otherEntity.first;
        entity.second.reserve(otherEntity.second.size());

        for (const std::unique_ptr<Component>& component : otherEntity.second)
        {
            std::unique_ptr<Component> componentPtr = component->Copy<Component>();
            Assert(componentPtr != nullptr, "Copy operation returned nullptr");
            entity.second.push_back(std::move(componentPtr));
        }
    }
    return *this;
}

Handle Scene::AddEntity()
{
    std::string newName = std::format("Entity {}", entities.size());
    entities.emplace_back(std::make_pair(EntityInfo{ newName, Guid::Create() }, Array<std::unique_ptr<Component>>{}));
    return { entities.size() - 1 };
}

Handle Scene::AddEntity(const EntityInfo& info)
{
    entities.emplace_back(std::make_pair(info, Array<std::unique_ptr<Component>>{}));
    return { entities.size() - 1 };
}

DEFINE_SERIALIZATION(Scene, sceneSettings, entities)

template <> void SerializeObject(nlohmann::json& json, const Array<std::pair<EntityInfo, Array<std::unique_ptr<Component>>>>& SER_VAL)
{
    for (auto& entity : SER_VAL)
    {
        auto& j = json.emplace_back();
        j["name"] = entity.first.name;
        j["guid"] = entity.first.guid;
        for (auto& comp : entity.second)
        {

            if (!EntityComponentSystem::GetRegisteredSerializers().contains(comp->GetType()))
                continue;

            EntityComponentSystem::GetRegisteredSerializers().at(comp->GetType())(j, comp.get());
        }
    }
}

BEGIN_SERIALIZATION(Array<EntityInfo>)
    for (auto& entity : SER_VAL)
    {
        auto& j = json.emplace_back();
        j.push_back(entity.name);
    }
END_SERIALIZATION()


BEGIN_DESERIALIZATION(Scene)
    PARSE_FIELD(sceneSettings);

    simdjson::ondemand::array arr;
    CHECK_ERROR(obj["entities"].get_array().get(arr));

    for (auto entity : arr)
    {
        auto entityId = PARSED_VAL.AddEntity();

        for (auto field : entity.get_object())
        {
            std::string name = simdjson::ParseUntilNextQuote(field.key().value().raw());

            if (name == "name")
            {
                std::string entityName{ field.value().get_string().value() };
                PARSED_VAL.entities[entityId].first.name = entityName;
            }
            else if (name == "guid")
            {
                std::string guid{ field.value().get_string().value() };
                PARSED_VAL.entities[entityId].first.guid = guid;
            }
            else
            {
                auto& desers = EntityComponentSystem::GetRegisteredDeserializers();
                for (auto& [type, deser] : desers)
                {
                    if (type.GetName().data() == name)
                    {
                        deser(PARSED_VAL, entityId, field);
                    }
                }
            }
        }
    }

END_DESERIALIZATION()
