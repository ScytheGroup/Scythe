module;

#include "ImGui/Icons/IconsFontAwesome6.h"
#include "Common/Macros.hpp"

module Core;

import :SceneSettings;
import <future>;
import ResourceBrowser;
import Serialization;

#ifdef IMGUI_ENABLED

import ImGui;

std::vector<std::pair<const std::string*, SceneSettings::Graphics::PostProcessPass*>> SceneSettings::Graphics::GetSortedPostProcessPasses()
{
    std::vector<std::pair<const std::string*, PostProcessPass*>> passes;
    for (auto& [path, passInfo] : customPostProcessPasses)
        passes.push_back(std::pair{&path, &passInfo});

    std::ranges::sort(passes, [](const auto& v1, const auto& v2) {
       return v1.second->priority < v2.second->priority;  
    });

    return passes;
}

std::vector<std::pair<const std::string*, const SceneSettings::Graphics::PostProcessPass*>> SceneSettings::Graphics::GetSortedPostProcessPasses() const
{
    std::vector<std::pair<const std::string*, const PostProcessPass*>> passes;
    for (auto& [path, passInfo] : customPostProcessPasses)
        passes.push_back(std::pair{&path, &passInfo});

    std::ranges::sort(passes, [](const auto& v1, const auto& v2) {
       return v1.second->priority < v2.second->priority;  
    });

    return passes;
}

bool SceneSettings::ImGuiDraw(bool* opened)
{
    bool result = false;
    if (ImGui::Begin("Scene Settings", opened))
    {
        result |= ImGuiDrawContent();
    }
    ImGui::End();
    return result;
}

bool SceneSettings::ImGuiDrawContent()
{
    ScytheGui::Header("Scene Settings");
    
    bool result = false;

    ImGui::InputText("World Name", &worldName, 256);

    ImGui::SeparatorText("Directional Light");
    result |= ImGui::ColorEdit3("Color", &graphics.directionalLight.color.x);
    result |= ImGui::DragFloat("Intensity", &graphics.directionalLight.intensity, 0.01f, 0.0f, std::numeric_limits<float>::max());
    result |= ImGui::DragFloat3("Direction", &graphics.directionalLight.direction.x, 0.001f);

    ImGui::BeginDisabled(graphics.enableAtmosphere);
    ImGui::SeparatorText("Skybox");
    if (ScytheGui::BeginLabeledField())
    {
        std::string path = graphics.skyboxPath.string();

        if (ImGui::Button(ICON_FA_ELLIPSIS))
        {
            if (auto resultPath = FileBrowser::BrowseForFile(ProjectSettings::Get().projectRoot, {{"HDR Files", "hdr"}}); resultPath.has_value())
            {                
                graphics.skyboxPath = ResourceBrowser::FormatPath(*resultPath).string();
            }
        }
        ImGui::SameLine();
        
        ImGui::BeginDisabled();
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##", &path);
        ImGui::EndDisabled();
        ScytheGui::EndLabeledField("Skybox Path");
    }
    ImGui::EndDisabled();

#if VXGI_ENABLED
    ImGui::SeparatorText("Global Illumination");
    result |= ImGui::Checkbox("Enable Global Illumination", &graphics.enableVoxelGI);
#endif

    ImGui::SeparatorText("Atmosphere Settings");
    result |= ImGui::Checkbox("Enable Atmosphere", &graphics.enableAtmosphere);

    ImGui::BeginDisabled(!graphics.enableAtmosphere);
    result |= ImGui::DragFloat("Rayleigh Density Scale", &graphics.atmosphereSettings.rayleighScatteringDensity, 0.1f, 0, 1'000'000);
    result |= ImGui::DragFloat("Mie Density Scale", &graphics.atmosphereSettings.mieScatteringDensity, 0.1f, 0, 1'000'000);
    result |= ImGui::DragFloat3("Ground Albedo", &graphics.atmosphereSettings.groundAlbedo.x, 0.001f, 0, 1);
    result |= ImGui::DragFloat("Atmosphere Intensity", &graphics.atmosphereSettings.atmosphereIntensity, 0.01f, 0, 1'000'000);
    ImGui::EndDisabled();

    ImGui::SeparatorText("Custom Postprocess Passes");
    static std::future<std::optional<std::filesystem::path>> browserFuture;
    // Top toolbar style buttons
    if (ImGui::Button("+"))
    {
        if (!browserFuture.valid())
        {
            browserFuture = FileBrowser::BrowseForFileAsync(ProjectSettings::Get().projectRoot, { { "HLSL Shader File", "hlsl" } });
        }
    }

    // Handle adding new path with popup
    if (browserFuture.valid() && browserFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        if (auto resultFile = browserFuture.get(); resultFile)
        {
            auto relativePath = ResourceBrowser::FormatPath(*resultFile);
            graphics.customPostProcessPasses[relativePath.stem().string()] = { relativePath.string(), false, 0 };
            result = true;
        }
    }
    
    // Display paths in a table format
    if (ImGui::BeginTable("Paths", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 20.0f);
        ImGui::TableSetupColumn("Priority", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Remove", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableHeadersRow();

        std::vector<const std::string*> postFxToRemove;

        int i = 0;
        for (auto& [passName, passInfo] : graphics.GetSortedPostProcessPasses())
        {
            ImGui::TableNextRow();
            ImGui::PushID(i++);

            // Checkbox column
            ImGui::TableSetColumnIndex(0);
            result |= ImGui::Checkbox("##enabled", &passInfo->enabled);

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            result |= ImGui::InputInt("##prio", &passInfo->priority);
            
            // Path column
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", passName->c_str());
            
            // Remove button column
            ImGui::TableSetColumnIndex(3);
            if (ImGui::Button("Remove"))
            {
                postFxToRemove.push_back(passName);
                result = true;
            }
            else
            {
                ++i;
            }

            ImGui::PopID();
        }

        for (const auto& path : postFxToRemove)
        {
            graphics.customPostProcessPasses.erase(*path);
        }
        
        ImGui::EndTable();
    }

    ImGui::SeparatorText("Physics Settings");
    result |= ImGui::InputFloat3("Gravity Vector", &physics.gravity.x);

    return result;
}

#endif

