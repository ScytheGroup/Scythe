module;

#include <filesystem>
#include <fstream>

module Common:ProjectSettings;

import :Debug;
import :ProjectSettings;

static ProjectSettings projectSettings;

const ProjectSettings& ProjectSettings::Get()
{
    return projectSettings;
}

ProjectSettings::ProjectSettings()
{
    projectRoot = std::filesystem::current_path() / ProjectRootFromSource;

    for (auto& dir : std::filesystem::directory_iterator{ projectRoot })
    {
        if (dir.path().extension().string() == ProjectFileExtension)
        {
            ParseProjectFile(dir.path());
            return;
        }
    }
    // if no engine root set, set as current dir
    if (engineRoot.empty())
        engineRoot = std::filesystem::current_path();
}

void ProjectSettings::ParseProjectFile(const std::filesystem::path& filepath)
{
    std::ifstream stream{ filepath };

    while (stream && !stream.eof())
    {
        std::string line;
        std::getline(stream, line);

        if (line.empty())
            continue;

        auto separatorIndex = line.find_first_of('=');

        if (separatorIndex == std::string::npos)
            continue;

        const std::string key = line.substr(0, separatorIndex);
        const std::string value = line.substr(separatorIndex + 1);

        if (key == ProjectNameSetting)
            projectName = value;

        if (key == EngineRootSetting)
        {
            std::filesystem::path engineDir = value;
            engineDir /= "Engine";
            if (engineDir.is_absolute())
                engineRoot = engineDir;
            else
                engineRoot = projectRoot / engineDir;
        }
    }

    DebugPrint("Loaded project settings from: {}", filepath.string());
}
