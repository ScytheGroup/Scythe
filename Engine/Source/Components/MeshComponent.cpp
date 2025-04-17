module;

#include "Serialization/SerializationMacros.h"
#include "ImGui/Icons/IconsFontAwesome6.h"

module Components:Mesh;

import :Mesh;

import Common;
import Core;

#ifdef EDITOR
import Editor;

bool MeshComponent::DrawEditorInfo()
{
    Super::DrawEditorInfo();
    bool result = false;

    result |= DrawEditorEntry("Mesh", mesh);
    result |= DrawEditorEntry("IsVisible", isVisible);
    result |= DrawEditorEntry("IsFrustumCulled", isFrustumCulled);
    
    if (mesh)
    {
        ImGui::SeparatorText("Material Overrides");
        
        std::vector<int> overridesToRemove;
        for (int i = 0; i < mesh->materials.size(); ++i)
        {
            ImGui::PushID(i);
            ImGui::Text(std::format("{}", i).c_str());
            ImGui::SameLine();
            if (materialOverrides.contains(i))
            {
                if (ImGui::Button(ICON_FA_TRASH_CAN))
                {
                    overridesToRemove.push_back(i);
                }
                ImGui::SameLine();
                auto& material = materialOverrides[i];
                if (ImGui::CollapsingHeader(ResourceLoader<Material>::Get().GetName(material), ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Indent();
                    result |= DrawAssetRefDragDrop(material);
                    ImGui::Unindent();
                }    
            }
            else
            {
                if (ImGui::Button("Override"))
                {
                    AssetRef<Material> defaultRef{Guid("Default")};
                    ResourceManager::Get().materialLoader.FillAsset(defaultRef);
                    materialOverrides[i] = defaultRef;
                }
                
                if (mesh->materials[i].asset)
                {
                    ImGui::SameLine();
                    ImGui::Text(ResourceLoader<Material>::Get().GetName(mesh->materials[i]));
                }
            }
            ImGui::PopID();
        }

        for (auto id : overridesToRemove)
            materialOverrides.erase(id);
    }
    return result;
}
#endif

DEFINE_SERIALIZATION_AND_DESERIALIZATION(MeshComponent, mesh, materialOverrides, isFrustumCulled, isVisible);
