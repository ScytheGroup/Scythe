export module Core:SceneSerializer;

import Common;
import :Scene;

export class SceneSerializer
{
public:
    static std::string SerializeToJson(const Scene& scene);
    static Scene ParseFromJson(const std::string& jsonString);
};

