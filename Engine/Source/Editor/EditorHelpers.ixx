module;

#include "ImGui/Icons/IconsFontAwesome6.h"

export module Editor:EditorHelpers;

import Graphics;
import Common;
import ImGui;
import Tools;
import ResourceBrowser;

import :Icons;

template<class T>
void DrawAssetPreview(AssetRef<T>& ref, const Vector2& size)
{
    if constexpr (std::is_same_v<T, Texture>)
    {
        if (Texture* tex = ref.GetLoadedAsync())
            ImGui::Image(tex->GetSRV().Get(), Convert(size));
    }
    
    if constexpr (std::is_same_v<T, Material>)
    {
        ImGui::Image(EditorIcons::AssetBrowserMaterialsIcon->GetSRV().Get(), Convert(size));
    }
           
    if constexpr (std::is_same_v<T, Mesh>)
    {
        ImGui::Image(EditorIcons::AssetBrowserMeshIcon->GetSRV().Get(), Convert(size));
    }
}

template<class T>
std::optional<AssetRef<T>> ComponentModal(bool openModal)
{
    static AssetRef<T> selectedAsset{};
    static std::string searchBuffer = "";

    Array<AssetRef<T>> assets = ResourceLoader<T>::Get().GetAllAssetRefs();
    std::ranges::sort(assets, [](const AssetRef<T>& a, const AssetRef<T>& b) -> bool 
    {
        return ResourceLoader<T>::Get().GetName(a) < ResourceLoader<T>::Get().GetName(b); 
    });

    if (ScytheGui::BeginModal("Pick Asset", openModal))
    {
        // Search bar
        ImGui::SetNextItemWidth(400);
        ImGui::InputTextWithHint("##search", ICON_FA_MAGNIFYING_GLASS " Search Components", &searchBuffer);

        // Create a list of components that can be added
        if (ImGui::BeginListBox("##components", ImVec2(-1, 300)))
        {
            auto ToLowercase = [](const std::string& s) -> std::string
            {
                std::string copy = s;
                for (auto& c : copy)
                {
                    c = std::tolower(c);
                }
                return copy;
            };
            std::string searchStr = ToLowercase(searchBuffer);
            for (size_t i = 0; i < assets.size(); ++i)
            {
                ImGui::PushID(i);
                AssetRef<T>& asset = assets[i];
                std::string name = ResourceLoader<T>::Get().GetName(asset);
                std::string componentName = ToLowercase(name);
                
                // Skip if doesn't match search
                if (!searchStr.empty() && componentName.find(searchStr) == std::string::npos)
                    continue;

                bool isAssetSelected = selectedAsset == asset;
                if (ImGui::Selectable(ResourceLoader<T>::Get().GetName(assets[i]), isAssetSelected))
                { 
                    selectedAsset = asset;
                }
                
                if (ImGui::IsItemHovered() && isAssetSelected && ImGui::BeginTooltip())
                {
                    DrawAssetPreview(asset, Vector2{40,40});
                    ImGui::EndTooltip();
                }
                ImGui::PopID();
            }

            ImGui::EndListBox();
        }

        ImGui::Spacing();

        if (auto result = ScytheGui::EndModal(ScytheGui::ModalType::OK_CANCEL); result != ScytheGui::NONE)
        {
            searchBuffer.clear();
        
            if (result == ScytheGui::PopupStatus::OK)
            {
                if (selectedAsset.HasId())
                {
                    return selectedAsset;    
                }
            }
        }
    }

    return {};
}

export template<class T>
bool DrawAssetRefDragDrop(AssetRef<T>& ref)
{
    static constexpr auto MaxLength = 21;
    std::string name{ ResourceLoader<T>::Get().GetName(ref) };
    if (name.size() > MaxLength)
    {
        name = name.substr(0, MaxLength - 3) + "...";
    }

    ImGui::BeginGroup();

    static constexpr auto size = 85;
    if (ImGui::BeginTable("AssetTable", 2, ImGuiTableFlags_SizingStretchSame))
    {
        ImGui::TableSetupColumn("##1", ImGuiTableColumnFlags_WidthFixed, size);
        ImGui::TableSetupColumn("##2", ImGuiTableColumnFlags_WidthStretch);
        
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        Vector2 padding(2.0f, 2.0f);
        Vector2 sizeBox(size, size);
        
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Convert(padding));

        if (ImGui::IsDragDropActive() && ScytheGui::IsDragDropPayloadOfType<AssetRef<T>>())
        {
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1, 1, 0.6, 1));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.8, 0.8, 0.3, 1));
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.4,0.4,0.4,1));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.2,0.2,0.2,1));
        }

        bool isPreviewHovered = false;
        if (ImGui::BeginChild(ImGui::GetID("Asset"), Convert(sizeBox), true, ImGuiWindowFlags_NoScrollbar))
        {
            isPreviewHovered = ImGui::IsWindowHovered();
            if (ref.HasId())
            {
                DrawAssetPreview(ref, sizeBox - padding * 2);
            }
        }
    
        ImGui::EndChild();

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);

        ImGui::TableNextColumn();
        
        bool shouldClear = ImGui::Button(ICON_FA_TRASH_CAN);
        ScytheGui::TextTooltip("Clears the pointed asset");
        if (shouldClear)
        {
            ref = {};
        }
        ImGui::SameLine();
    
        bool openModal = ImGui::Button(ICON_FA_PENCIL);
        ScytheGui::TextTooltip("Edit the asset pointed. Same as double-clicking on it.");

        ImGui::SameLine();
        ImGui::Text(ResourceLoader<T>::Get().GetAssetDisplayName().data());

        ImGui::SeparatorText("Asset Name");
        ImGui::TextWrapped(name.c_str());

        bool doubleClick = isPreviewHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
        if (auto component = ComponentModal<T>(openModal || doubleClick))
        {
            ref = *component;
            ref.LoadAsset();
        }

        ImGui::EndTable();
    }
    
    ImGui::EndGroup();

    if (ref.HasId())
        ScytheGui::Drag(ref, [&] { ImGui::Text(std::format("dragging {}", name).c_str()); });

    bool result = false;
    ScytheGui::Drop<AssetRef<T>>([&](const AssetRef<T>& newRef)
    {
        ref = newRef;
        ref.LoadAsset();
        result = true;
    });
    return result;
}

export
{
    template <class T>
    bool DrawEditorEntry(std::string_view label, Array<T>& refs)
    {
        bool result = false;
        
        ScytheGui::Header(label.data());
        if constexpr (std::is_default_constructible_v<T>)
        {
            if (ImGui::Button(ICON_FA_PLUS))
                refs.push_back({});
        }
        for (auto&& ref : refs)
            result |= DrawEditorEntry<T>("", ref);
        return result;
    }

    template <class EnumT> requires std::is_enum_v<EnumT>
    bool DrawEditorEntry(std::string_view label, EnumT& enumVal)
    {
        return ScytheGui::ComboEnum(label, enumVal);
    }

    template <class K, class V>
    bool DrawEditorEntry(std::string_view label, std::unordered_map<K, V>& refs)
    {
        bool result = false;

        ScytheGui::Header(label.data());
        for (auto&& [k, v] : refs)
        {
            result |= DrawEditorEntry(std::format("{}:", k).c_str(), v);
        }
        
        return result;
    }

    template <class T>
    bool DrawEditorEntry(std::string_view label, T* ptr)
    {
        if (ptr)
            return DrawEditorEntry(label, *ptr);
        return false;
    }

    template <class T>
    bool DrawEditorEntry(std::string_view label, const T* ptr)
    {
        if (ptr)
            return DrawEditorEntry(label, *ptr);
        return false;
    }
    
    template <class T>
    bool DrawEditorEntry(std::string_view label, T& ref)
    {
        return false;
    }

    template <class T>
    bool DrawEditorEntry(std::string_view label, const T& ref)
    {
        ImGui::BeginDisabled();
        DrawEditorEntry(label, const_cast<T&>(ref));
        ImGui::EndDisabled();
        return false;
    }

    template <class T> requires IsImGuiDrawable<T>
    bool DrawEditorEntry(std::string_view label, T& ref)
    {
        if (label.empty())
            return ref.ImGuiDraw();
        
        ScytheGui::Header(label.data());

        ImGui::Indent();
        bool res = ref.ImGuiDraw();
        ImGui::Unindent();
        
        return res;
    }

    template <>
    bool DrawEditorEntry<Transform>(std::string_view label, Transform& ref);
    
    template <>
    bool DrawEditorEntry<Vector4>(std::string_view label, Vector4& ref);

    template <>
    bool DrawEditorEntry<Vector3>(std::string_view label, Vector3& ref);

    template <>
    bool DrawEditorEntry<Vector2>(std::string_view label, Vector2& ref);

    template <>
    bool DrawEditorEntry<Quaternion>(std::string_view label, Quaternion& ref);

    template <>
    bool DrawEditorEntry<Color>(std::string_view label, Color& ref);

    template <>
    bool DrawEditorEntry<std::string>(std::string_view label, std::string& ref);

    template <class T>
        requires std::is_integral_v<T>
    bool DrawEditorEntry(std::string_view label, T& ref)
    {
        bool result = false;
        int value = static_cast<int>(ref);
        result |= ImGui::DragInt(label.data(), &value);
        ref = static_cast<T>(value);
        return result;
    }

    template <class T>
        requires std::is_floating_point_v<T>
    bool DrawEditorEntry(std::string_view label, T& ref)
    {
        bool result = false;
        float value = ref;
        result |= ImGui::DragFloat(label.data(), &value);
        ref = value;
        return result;
    }

    template<>
    bool DrawEditorEntry(std::string_view label, bool& ref);

    template<class T>
    bool DrawEditorEntry(std::string_view label, AssetRef<T>& ref)
    {
        bool changed = false;

        if (ScytheGui::BeginLabeledField())
        {
            changed |= DrawAssetRefDragDrop(ref);
            ScytheGui::EndLabeledField(label);
        }

        if (changed)
            ref.MarkDirty();
        
        return changed;
    }

    template<>
    bool DrawEditorEntry<Handle>(std::string_view label, Handle& handle);

    template <> 
    bool DrawEditorEntry(std::string_view label, std::filesystem::path& path);
}
