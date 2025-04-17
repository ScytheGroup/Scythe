module;

#include "ImGui/Icons/IconsFontAwesome6.h"

module Editor;

import Graphics;

import :EditorHelpers;

template <>
bool DrawEditorEntry<Transform>(std::string_view label, Transform& ref)
{
    bool result = false;
    result |= DrawEditorEntry("Position", ref.position);
    bool rotaRes = false;
    Quaternion rotation = ref.rotation;
    result |= (rotaRes = DrawEditorEntry("Rotation", rotation));
    result |= ImGui::DragFloat3("Scale", &ref.scale.x, 0.1, FloatSmallNumber, std::numeric_limits<float>::max(), "%.3f", ImGuiSliderFlags_AlwaysClamp);
    
    if (rotaRes)
       ref.RotateTo(rotation.ToEuler());
        
    return result;
}

template <>
bool DrawEditorEntry<Handle>(std::string_view label, Handle& handle)
{
    return DrawEditorEntry(label, handle.id);
}

template <>
bool DrawEditorEntry<std::string>(std::string_view label, std::string& ref)
{
    return ImGui::InputText(label.data(), &ref);
}

template <>
bool DrawEditorEntry<bool>(std::string_view label, bool& ref)
{
    return ImGui::Checkbox(label.data(), &ref);
}

template <>
bool DrawEditorEntry<Quaternion>(std::string_view label, Quaternion& ref)
{
    bool result = false;
        
    Vector3 euler = RadToDeg(ref.ToEuler());
    result = ImGui::DragFloat3(label.data(), &euler.x, 0.01);
    euler = DegToRad(euler);

    if (result)
        ref = Quaternion::CreateFromYawPitchRoll(euler.y, euler.x, euler.z);
        
    return result;
}

template <>
bool DrawEditorEntry<Vector2>(std::string_view label, Vector2& ref)
{
    return ImGui::DragFloat2(label.data(), &ref.x);
}

template <>
bool DrawEditorEntry<Vector3>(std::string_view label, Vector3& ref)
{
    return ImGui::DragFloat3(label.data(), &ref.x);
}

template <>
bool DrawEditorEntry<Vector4>(std::string_view label, Vector4& ref)
{
    return ImGui::DragFloat4(label.data(), &ref.x);
}

template <>
bool DrawEditorEntry<Color>(std::string_view label, Color& ref)
{
    return ImGui::ColorEdit4(label.data(), &ref.x);
}

template <>
bool DrawEditorEntry<std::filesystem::path>(std::string_view label, std::filesystem::path& path)
{
    bool result = false;
    if (ScytheGui::BeginLabeledField())
    {
        if (ImGui::Button(ICON_FA_TRASH))
        {
            path.clear();
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_ELLIPSIS))
        {
            if (auto resultPath = FileBrowser::BrowseForFile(ProjectSettings::Get().projectRoot); resultPath.has_value())
            {                
                path = ResourceBrowser::FormatPath(*resultPath).string();
                result = true;
            }
        }
        ImGui::SameLine();
        
        ImGui::BeginDisabled();
        ImGui::SetNextItemWidth(-1);
        std::string pathName = path.string();
        ImGui::InputText("##", &pathName);
        ImGui::EndDisabled();
        ScytheGui::EndLabeledField(label);
    }

    return result;
}
