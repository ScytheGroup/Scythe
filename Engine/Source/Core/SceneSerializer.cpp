module;

#include "Serialization/SerializationMacros.h"

module Core:SceneSerializer;

import Common;
import Systems.Physics;
import Graphics;
import Core;
import Components;
import :SceneSerializer;

std::string SceneSerializer::SerializeToJson(const Scene& scene)
{
    nlohmann::json j;
    to_json(j, scene);
    return j.dump();
}

Scene SceneSerializer::ParseFromJson(const std::string& jsonString)
{
    using namespace simdjson;
    padded_string str(jsonString);
    ondemand::parser parser;

    auto document = parser.iterate(str);

    Scene scene;
    if (auto error = document.get_value().get(scene))
    {
#ifdef EDITOR
        DebugPrintWindow("Error loading scene: {}", magic_enum::enum_name(error));
#else
        Assert<std::string_view>(false, "Error loading scene: {}", magic_enum::enum_name(error));
#endif
        return scene;
    }
    
    return scene;
}
