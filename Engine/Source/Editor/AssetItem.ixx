module;

export module Editor:AssetItem;

import Tools;
import Common;
import ImGui;
import Graphics;

import :EditorDelegates;

export template<class T>
struct AssetItem
{
    AssetRef<T> assetRef{};
    Texture* icon;
    ImVec2 size{1,1};
    uint8_t maxLength = 40;
    bool selected = false;
    bool isHovered = false;

    AssetItem(const AssetRef<T>& assetRef, Texture* icon, bool wasSelected)
        : assetRef{assetRef}
        , icon{icon}
        , selected{wasSelected}
    {
    }

    AssetItem(const AssetItem& other)
    {
        assetRef = other.assetRef;
        icon = other.icon;
        size = other.size;
        maxLength = other.maxLength;
        selected = other.selected;
        isHovered = other.isHovered;
    }

    std::string_view GetItemName() const;
    void ImGuiDraw();
    void DrawPreview();

    Texture* GetIcon();
};

template <class T>
std::string_view AssetItem<T>::GetItemName() const
{
    return ResourceLoader<T>::Get().GetName(assetRef);
}

template<class T>
void AssetItem<T>::ImGuiDraw()
{
    ImGui::PushID(assetRef.GetId().ToString().c_str());
    DrawPreview();
    ImGui::PopID();
}

template <class T>
void AssetItem<T>::DrawPreview()
{
    Vector2 padding(2.0f, 2.0f);
    
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Convert(padding));

    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.4,0.4,0.4,1));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.2,0.2,0.2,1));
    
    if (selected)
    {
        ImGui::PopStyleColor(2);
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1, 1, 1, 1));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xa65946FF);
    }

    if (isHovered)
    {
        ImGui::PopStyleColor(2);
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1, 1, 1, 1));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xf77d5eFF);
    }

    if (ImGui::BeginChild(ImGui::GetID("AssetItem"), size, true, ImGuiWindowFlags_NoScrollbar))
    {
        ImGui::PushID(assetRef.GetId().ToString().c_str());
        ImGui::BeginGroup();
        
        isHovered = ImGui::IsWindowHovered();
        if (auto displayIcon = GetIcon())
        {
            if (displayIcon->IsLoaded())
                ImGui::Image(displayIcon->GetSRV().Get(), Convert(Vector2(size.x, size.x) - padding * 2));
            else
                displayIcon->RegisterLazyLoadedResource(true);
        }

        std::string name{ GetItemName() };
        if (name.size() > maxLength)
        {
            name = name.substr(0, maxLength - 3) + "...";
        }

        ImGui::PushFont(ImGui::smallFont);
        ImGui::TextWrapped(name.c_str());
        ImGui::PopFont();

        ImGui::EndGroup();

        ScytheGui::Drag(assetRef, [this]{ DrawPreview(); });

        if (ScytheGui::BeginContextMenu(assetRef.GetId().ToString().c_str()))
        {
            AssetMetadata& metadata = ResourceLoader<T>::Get().GetMetadata(assetRef);

            if (!metadata.isTransient)
            {
                if (ScytheGui::ContextMenuButton("Delete"))
                {
                    ResourceLoader<T>::Get().MarkToDelete(assetRef);
                }    
            }
            
            ScytheGui::EndContextMenu();
        }

        ImGui::PopID();
    }
    
    ImGui::EndChild();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);

    if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        EditorDelegates::OnAssetSelected.Execute(&assetRef);
        selected = true;
    }
}

template <class T>
Texture* AssetItem<T>::GetIcon()
{
    if constexpr (std::is_same_v<T, Texture>)
        return assetRef.asset;
    return icon;
}
