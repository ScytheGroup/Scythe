module;

#include "nlohmann/json.hpp"
#include "Serialization/SerializationMacros.h"

export module Serialization;

export import Common;
import Tools;

export namespace nlohmann
{
    using nlohmann::json;
}

export template<class T>
struct IsSerializable : std::false_type {};

import Generated;

export template<class T>
struct FindWrapper
{
    using Type = T;
};

template<class T, class F>
void FindType(Type t, F f)
{
    if (t == Type::GetType<T>())
        f(FindWrapper<T>{});
}

template<class T, class F>
void FindTypeByName(const std::string& name, F f)
{
    if (name == Type::GetTypeName<T>().data())
        f(FindWrapper<T>{});
}

export template<class ...Types, class Base, class F>
void SwitchClass(const Base& object, F arg)
{
    Type t = object.GetType();
    (FindType<Types>(t, arg), ...);
}

export template<class ...Types, class F>
void SwitchClassByName(const std::string& name, F arg)
{
    (FindTypeByName<Types>(name, arg), ...);
}

export template<class ...Types, class F>
void DeserializeFieldSwitch(simdjson::simdjson_result<simdjson::ondemand::field>& field, F arg)
{
    std::string name = simdjson::ParseUntilNextQuote(field.key().value().raw());
    SwitchClassByName<Types...>(name, arg);
}

export template<class T>
void SerializeObject(nlohmann::json&, const T&);

export template<class T> requires std::is_convertible_v<T, std::string>
void SerializeObject(nlohmann::json& json, const T& enumVal)
{
    json = static_cast<const std::string>(enumVal);
}

export template<class T> requires std::is_enum_v<T>
void SerializeObject(nlohmann::json& json, const T& enumVal)
{
    json = std::to_underlying(enumVal);
}

export template<class K, class V>
void SerializeObject(nlohmann::json& json, const std::unordered_map<K, V>& map)
{
    json = nlohmann::json::object();
    for (const auto& [k, v] : map)
    {
        std::stringstream ss;
        ss << k;
        json[ss.str()] = v;
    }
}

BEGIN_SERIALIZATION(Guid)
    json = SER_VAL.ToString();
END_SERIALIZATION()

BEGIN_DESERIALIZATION(Guid)
    std::string s;
    DeserializeObject(obj, s);
    PARSED_VAL = Guid(s);
END_DESERIALIZATION()

export template<class T>
void SerializeObject(nlohmann::json& json, const AssetRef<T>& SER_VAL)
{
    SerializeObject(json, SER_VAL.GetId());
}

export template<class T1, class T2>
void SerializeObject(nlohmann::json& json, const std::pair<T1, T2>& SER_VAL)
{
    SerializeObject(json[0], SER_VAL.first);
    SerializeObject(json[1], SER_VAL.second);
}

export template<class T>
simdjson::error_code DeserializeObject(simdjson::ondemand::value& obj, AssetRef<T>& PARSED_VAL)
{
    Guid guid;
    auto result = DeserializeObject(obj, guid);
    PARSED_VAL = {guid};
    return result;
}

DEFINE_GLOBAL_TO_JSON_IN_NAMESPACE(DirectX, SimpleMath)
DEFINE_GLOBAL_TO_JSON_IN_NAMESPACE(JPH)
DEFINE_GLOBAL_TO_JSON()

