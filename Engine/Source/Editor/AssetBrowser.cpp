module;

#include "Common/Macros.hpp"
#include "ImGui/Icons/IconsFontAwesome6.h"

module Editor:AssetBrowser;

import ImGui;
import Editor;
import Graphics;
import :AssetBrowser;

bool IsFiltered(const std::string& filter, const std::regex& regex, bool useRegex, const std::string& itemName)
{
    return useRegex ? std::regex_match(itemName, regex) : itemName.contains(filter);
}

template<class T>
void FillBrowserItems(auto& items, Texture* icon, const Guid& selectedItem)
{
    auto materialAssets = ResourceLoader<T>::Get().GetAllAssetRefs();
    items.clear();
    items.reserve(materialAssets.size());
    std::ranges::transform(materialAssets, std::back_inserter(items), [icon, &selectedItem](const AssetRef<T>& ref) { return AssetItem<T>{ ref, icon, ref.GetId() == selectedItem }; }); 
}

void UnselectAllItems(auto& items)
{
    for (auto& item : items)
        item.selected = false;
}

AssetBrowser::AssetBrowser()
{
    ReloadAssets();
    
    auto markNeedsRefresh = [this](const auto&, auto*) { needsRefresh = true; };
    auto markNeedsRefreshDel = [this](const auto&) {
        needsRefresh = true;
        selectedAsset = {};
    };
    
    ResourceLoader<Mesh>::Get().OnAssetLoaded.Subscribe(markNeedsRefresh);
    ResourceLoader<Texture>::Get().OnAssetLoaded.Subscribe(markNeedsRefresh);
    ResourceLoader<Material>::Get().OnAssetLoaded.Subscribe(markNeedsRefresh);
    ResourceManager::Get().OnAssetDeleted.Subscribe(markNeedsRefreshDel);
    ResourceManager::Get().OnAssetImported.Subscribe(markNeedsRefreshDel);
    
    EditorDelegates::OnAssetSelected.Subscribe([this](IAssetRef* assetRef) {
        selectedAsset = assetRef;
        UnselectAllItems(materials);
        UnselectAllItems(textures);
        UnselectAllItems(meshes);
    });
    
    IMGUI_REGISTER_WINDOW_OPEN("Asset Browser", ASSETS, AssetBrowser::ImGuiDraw);
    IMGUI_REGISTER_WINDOW_OPEN("Asset Details", ASSETS, AssetBrowser::DrawAssetDetails);

    IMGUI_REGISTER_WINDOW_LAMBDA("New Material", ASSETS, [this](bool* opened)
    {
        static std::string error = "";

        auto close = [&]
        {
            *opened = false;
            error = "";
        };
        
        if (*opened)
        {
            if (ScytheGui::BeginModal("New Scene", opened))
            {
                static std::string newMaterialName = "";
                ImGui::InputText("##", &newMaterialName);

                if (!error.empty())
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, 0xFF0000FF);
                    ImGui::Text(error.c_str());
                    ImGui::PopStyleColor();
                }

                if (auto result = ScytheGui::EndModal(ScytheGui::ModalType::CONFIRM_CANCEL); result != ScytheGui::NONE)
                {
                    if (result == ScytheGui::CONFIRM)
                    {
                        if (newMaterialName.empty())
                        {
                            error = "Material name cannot be empty";
                        }
                        else
                        {
                            AssetMetadata metadata{ newMaterialName };
                            auto material = ResourceLoader<Material>::Get().ImportFromMemory(metadata);
                            material->RegisterLazyLoadedResource(true);
                            needsRefresh = true;
                            close();
                        }
                    }
                    else
                    {
                        close();
                    }
                }
            }
        }
    });
}

void AssetBrowser::ReloadAssets()
{
    Guid selectedAssetId = selectedAsset ? selectedAsset->GetId() : Guid{}; 
    FillBrowserItems<Material>(materials, EditorIcons::AssetBrowserMaterialsIcon.get(), selectedAssetId);
    FillBrowserItems<Mesh>(meshes, EditorIcons::AssetBrowserMeshIcon.get(), selectedAssetId);
    FillBrowserItems<Texture>(textures, EditorIcons::AssetBrowserTextureIcon.get(), selectedAssetId);
}

void AssetBrowser::ImGuiDraw(bool* stayOpen)
{
    if (ImGui::Begin("Asset Browser", stayOpen))
    {
        if (needsRefresh)
        {
            ReloadAssets();
            needsRefresh = false;
        }
        
        const ImVec2 region = ImGui::GetContentRegionAvail();

        int columnCount = static_cast<int>(region.x / (style.itemWidth + style.cellPadding));

        columnCount = std::max(columnCount, 1);
        
        static std::string filter;
        static bool asRegex = false;

        ImGui::Text("Search:");
        ImGui::SameLine();
        ImGui::InputText("##", &filter);
        ImGui::SameLine();
        ImGui::Text("use regex");
        ImGui::SameLine();
        ImGui::Checkbox("##_1", &asRegex);
    
        std::regex regex(filter);

        auto drawBrowserTab = [&](const std::string& tabName, auto& items)
        {
            if (ImGui::BeginTabItem(tabName.c_str()))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(style.cellPadding, style.cellPadding));

                if (ImGui::BeginTable("table", columnCount))
                {
                    ImGui::TableNextRow();
                    for (auto& item : items)
                    {
                        std::string name{ item.GetItemName() };
                        if (IsFiltered(filter, regex, asRegex, name))
                        {
                            ImGui::TableNextColumn();
                            item.size = ImVec2{ style.itemWidth, style.itemHeight };
                            item.ImGuiDraw();
                        }
                    }
                    
                    ImGui::EndTable();
                }
                ImGui::PopStyleVar();
                    
                ImGui::EndTabItem();    
            }
        };
        
        if (ImGui::Button(ICON_FA_ARROW_ROTATE_LEFT))
        {
            ReloadAssets();
        }

        if (ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_None))
        {
            drawBrowserTab("Meshes", meshes);
            drawBrowserTab("Materials", materials);
            drawBrowserTab("Textures", textures);
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void AssetBrowser::DrawAssetDetails(bool* stayOpen)
{
    if (ImGui::Begin("Asset Details", stayOpen))
    {
        if (selectedAsset)
        {
            bool result = false;

            ScytheGui::Header("Metadata:");

            ImGui::Indent();
            
            result |= selectedAsset->GetMetadata().ImGuiDraw();

            ImGui::Unindent();
            
            ScytheGui::Header("Asset:");

            ImGui::Indent();
            
            if (!selectedAsset->HasAsset())
                selectedAsset->LoadAsset();
            
            result |= selectedAsset->ImGuiDrawEditContent();
            
            if (result)
            {
                selectedAsset->MarkDirty();
            }

            ImGui::Unindent();
        }
    }
    ImGui::End();
}
