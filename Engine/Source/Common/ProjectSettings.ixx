export module Common:ProjectSettings;

import <string>;
import <filesystem>;

export class ProjectSettings
{
    // Relative path to directory with ".scythe" file
    static constexpr const char* ProjectRootFromSource = "";
    static constexpr const char* ProjectFileExtension = ".scythe";

    static constexpr const char* EngineRootSetting = "engine_dir";
    static constexpr const char* ProjectNameSetting = "name";

public:
    // Use this to not parse the .schythe file everytime
    static const ProjectSettings& Get();
    ProjectSettings();

    std::filesystem::path engineRoot;
    std::filesystem::path projectRoot;
    std::string projectName;
private:
    void ParseProjectFile(const std::filesystem::path& filepath);
};
