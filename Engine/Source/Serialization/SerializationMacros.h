#pragma once

#include <unordered_map>
#include <utility>
#include <vector>
#include "simdjson.h"
#include "Common/MacroHelpers.hpp"

#define TO_JSON_REFLECTION_CASE(json, component, T) case Type::GetTypeName<T>(): (json)[Type::GetTypeName<T>().data()] = *((component)->Cast<T>()); break;

#define BEGIN_SERIALIZATION(TYPE) \
template<>\
void SerializeObject(nlohmann::json& json, const TYPE& SER_VAL) {

#define END_SERIALIZATION() }

#define CHECK_ERROR(BODY)\
if (auto err = BODY; err && err != simdjson::NO_SUCH_FIELD){\
    assert(false);\
    return err;\
}

#define BEGIN_DESERIALIZATION(TYPE) \
template<>\
simdjson::error_code DeserializeObject(simdjson::ondemand::value& obj, TYPE& PARSED_VAL){ \

#define END_DESERIALIZATION() \
return simdjson::SUCCESS; }

#define PARSE_CHECKED(JSON, VALUE) CHECK_ERROR((JSON).get(VALUE))

#define PARSE_VARIABLE(VALUE) PARSE_CHECKED(obj, VALUE)

#define PARSE_KEY_VARIABLE(KEY, VALUE) PARSE_CHECKED(obj[KEY], VALUE)

#define PARSE_FIELD(FIELD) PARSE_KEY_VARIABLE(#FIELD, (PARSED_VAL).FIELD)

#define PARSE_KEY_FIELD(KEY, FIELD) PARSE_KEY_VARIABLE(KEY, (PARSED_VAL).FIELD)

#define WRITE_FIELD(FIELD) json[ #FIELD] = SER_VAL.FIELD;

#define WRITE_FIELD_OPTIONAL(CONDITION, FIELD) if ((SER_VAL).CONDITION) WRITE_FIELD(FIELD)

#define WRITE_KEY_FIELD(KEY, FIELD) json[KEY] = SER_VAL.FIELD;

#define WRITE_KEY_FIELD_OPTIONAL(CONDITION, KEY, FIELD) if ((SER_VAL).CONDITION) WRITE_KEY_FIELD(KEY, FIELD)

#define WRITE_VARIABLE(VARIABLE) json = VARIABLE;

#define WRITE_VARIABLE_OPTIONAL(CONDITION, VARIABLE) if (CONDITION) WRITE_VARIABLE(VARIABLE)

#define WRITE_KEY_VARIABLE(KEY, VARIABLE) json[KEY] = VARIABLE;

#define WRITE_KEY_VARIABLE_OPTIONAL(CONDITION, KEY, VARIABLE) if (CONDITION) WRITE_KEY_VARIABLE(KEY, VARIABLE)

#define WRITE_ASSET_REF(FIELD) WRITE_KEY_VARIABLE( #FIELD, static_cast<const IAssetRef&>((SER_VAL).FIELD))

#define DEFINE_GLOBAL_TO_JSON() \
export template<class T>\
void to_json(nlohmann::json& obj, const T& val)\
{ ::SerializeObject(obj, val); }\

#define BEGIN_NAMESPACE(N) namespace N {
#define END_NAMESPACE(N) }

#define DEFINE_GLOBAL_TO_JSON_IN_NAMESPACE(...)\
FOR_EACH(BEGIN_NAMESPACE, __VA_ARGS__)\
DEFINE_GLOBAL_TO_JSON()\
FOR_EACH(END_NAMESPACE, __VA_ARGS__)\

#define DEFINE_GLOBAL_FROM_JSON() \
template <class T> requires !simdjson::is_builtin_deserializable_v<T>\
simdjson::error_code tag_invoke(simdjson::deserialize_tag, simdjson::ondemand::value& j, T& t)\
{ return DeserializeObject(j, t); }\

#define DEFINE_GLOBAL_FROM_JSON_IN_NAMESPACE(...)\
FOR_EACH(BEGIN_NAMESPACE, __VA_ARGS__)\
DEFINE_GLOBAL_FROM_JSON()\
FOR_EACH(END_NAMESPACE, __VA_ARGS__)\

#define DEFINE_SERIALIZATION(TYPE, ...)\
BEGIN_SERIALIZATION(TYPE)\
FOR_EACH(WRITE_FIELD, __VA_ARGS__)\
END_SERIALIZATION()

#define DEFINE_DESERIALIZATION(TYPE, ...)\
BEGIN_DESERIALIZATION(TYPE)\
FOR_EACH(PARSE_FIELD, __VA_ARGS__)\
END_DESERIALIZATION()

#define DEFINE_SERIALIZATION_AND_DESERIALIZATION(TYPE, ...)\
DEFINE_SERIALIZATION(TYPE, __VA_ARGS__)\
DEFINE_DESERIALIZATION(TYPE, __VA_ARGS__)

#define DECLARE_SERIALIZABLE(TYPE)\
export template<>\
struct IsSerializable<TYPE> : std::true_type {};\

template<class T>
simdjson::error_code DeserializeObject(simdjson::ondemand::value&, T&);

template<class T> requires std::is_enum_v<T>
simdjson::error_code DeserializeObject(simdjson::ondemand::value& json, T& val)
{
    std::underlying_type_t<T> enumVal;
    PARSE_CHECKED(json, enumVal)
    val = static_cast<T>(enumVal);
    return simdjson::SUCCESS;
}

namespace simdjson
{
    inline std::string ParseUntilNextQuote(const char* str)
    {
        auto end = str;
        while (*++end != '"') {}
        return { str, end };
    }
}

template<class K, class V>
simdjson::error_code DeserializeObject(simdjson::ondemand::value& obj, std::unordered_map<K, V>& val) {
    for (auto pair : obj.get_object())
    {
        std::stringstream ss{simdjson::ParseUntilNextQuote(pair.key().value().raw())};
        
        V m;

        CHECK_ERROR(pair.value().get(m))    
        
        K k;
        ss >> k;
        val[k] = std::move(m);
    }
    return simdjson::SUCCESS;
}

template<class T> requires std::is_constructible_v<T, std::string>
simdjson::error_code DeserializeObject(simdjson::ondemand::value& obj, T& val){
    CHECK_ERROR(obj.get_string(val))
    return simdjson::SUCCESS;
}

template<class T1, class T2>
simdjson::error_code DeserializeObject(simdjson::ondemand::value& obj, std::pair<T1, T2>& val) {
    auto arr = obj.get_array();
    PARSE_CHECKED(arr.at(0), val.first)
    PARSE_CHECKED(arr.at(1), val.second)
    return simdjson::SUCCESS;
}

template<class T>
simdjson::error_code DeserializeObject(simdjson::ondemand::value& obj, std::vector<T>& val){
    for (auto elem : obj.get_array())
    {
        T newElement;
        PARSE_CHECKED(elem, newElement)
        val.push_back(std::move(newElement));
    }   
        
    return simdjson::SUCCESS;
}

template <class K, class V>
simdjson::error_code tag_invoke(simdjson::deserialize_tag, simdjson::ondemand::value& j, std::unordered_map<K, V>& t)
{ return DeserializeObject(j, t); }

DEFINE_GLOBAL_FROM_JSON_IN_NAMESPACE(std)
DEFINE_GLOBAL_FROM_JSON_IN_NAMESPACE(std, filesystem)
DEFINE_GLOBAL_FROM_JSON_IN_NAMESPACE(DirectX, SimpleMath)
DEFINE_GLOBAL_FROM_JSON_IN_NAMESPACE(JPH)
DEFINE_GLOBAL_FROM_JSON()
