module;

#include "ImGui/Icons/IconsFontAwesome6.h"

export module ImGui:ScytheGui;

import :Globals;
import Common;
import Reflection;
import "ImGui/AllImgui.h";
import <magic_enum/magic_enum.hpp>;
import <algorithm>;
import <memory>;

export class Component;
export class Texture;
// ImGui extensions for scythe engine
export namespace ScytheGui
{

template<class EnumT>
bool CheckboxFlags(std::string_view label, int& val, EnumT enumVal)
{
    return ImGui::CheckboxFlags(label.data(), &val, static_cast<int>(enumVal));
}

bool BeginContextMenu(std::string_view popupId);
bool ContextMenuButton(std::string_view label);
void EndContextMenu();

void Header(std::string_view name, ImFont* headerFont = ImGui::headerFont);
void SmallText(std::string_view name, ImFont* headerFont = ImGui::smallFont);

void TextTooltip(std::string_view sv, bool allowDisabled = false);

bool BeginTooltipOnHovered();

void InfoBubble(std::string_view text, std::string_view label = "(?)");

template <class T>
void Drag(const T& data, const std::function<void()>& dragStarted)
{
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    {
        // Tried using Type::GetTypeName with no avail
        ImGui::SetDragDropPayload(typeid(T).name(), &data, sizeof(T));
        dragStarted();
        ImGui::EndDragDropSource();
    }
}

template <class T>
bool IsDragDropPayloadOfType()
{
    if (!ImGui::IsDragDropActive())
        return false;
        
    auto payload = ImGui::GetDragDropPayload();
    return std::string_view(payload->DataType) == std::string_view(typeid(T).name());
}

template <class T>
void Drop(const std::function<void(const T&)>& dropped)
{
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(typeid(T).name()))
        {
            dropped(*static_cast<const T*>(payload->Data));
        }
        ImGui::EndDragDropTarget();
    }
}

template <class T>
void DragDrop(const T& data, const std::function<void()>& dragStarted, const std::function<void(const T&)>& dropped)
{
    Drag(data, dragStarted);
    Drop(dropped);
}

template<class E>
bool ComboEnum(std::string_view label, E& enumValue, bool skipLastValue = false)
{
    int selected = static_cast<int>(*magic_enum::enum_index(enumValue));
    int prevIndex = selected;

    using EnumNamesArray = std::array<const char*, magic_enum::enum_count<E>()>;
    static EnumNamesArray enumNames = []()
    {
        auto namesView = magic_enum::enum_names<E>();
        EnumNamesArray names;
        std::ranges::transform(namesView, names.begin(), [](auto&& strView)
        {
            return strView.data();
        });

        return names;
    }();

    ImGui::Combo(label.data(), &selected, enumNames.data(), std::max(0, static_cast<int>(enumNames.size()) - (skipLastValue ? 1 : 0)));

    enumValue = *magic_enum::enum_cast<E>(selected);
    
    return prevIndex != selected;
}

template<class T, class ...Ts> requires (std::derived_from<Ts, T> && ...)
constexpr bool ComboTypes(std::string_view label, std::unique_ptr<T>& value)
{
    constexpr std::array<StaticString, sizeof...(Ts)> names = { Type::GetTypeName<Ts>()... };
    static std::array<std::function<std::unique_ptr<T>()>, sizeof...(Ts)> constructors = { []{ return std::make_unique<Ts>(); }... };

    std::string previewName = value ? value->GetTypeName() : "None";

    bool result = false;
    if (ImGui::BeginCombo(label.data(), previewName.c_str()))
    {
        for (int i = 0; i < names.size(); ++i)
        {
            if (ImGui::Selectable(names[i]))
            {
                result = true;
                value = constructors[i]();
            }
        }
        
        ImGui::EndCombo();
    }
    
    return result;
}

ImRect BeginFlameGraph(const char* title, const char* overlayText, ImU8 maxDepth);
ImRect FlameGraphFrame(ImRect totalBB, float cumulativeWidth, float frameTime, float uiZoomFactor,
                       float totalProfilingTime,
                       int currentFrameNb, ImU8 maxDepth);
// Returns a pair representing the following: {bool: wasHovered, float: cumulativeWidth}
std::optional<std::pair<bool, float>> FlameGraphScope(const char* scopeName, ImVec2 scopeStartEnd,
                                                      float cumulativeWidth, float frameTime, ImRect frameBB,
                     ImU8 scopeDepth, ImU8 maxDepth);
void EndFlameGraph();

bool ComboBoxDetailsPanel(const char* title, int& selectedValue, std::vector<std::string>& comboBoxPossibleValues,
                          const char* noneOption = nullptr);

void MaskedImage(Texture& texture, const ImVec2& size, bool mask[4]);

template<class F>
void IfEnabled(bool enabled, F f)
{
    if (!enabled)
        ImGui::BeginDisabled();

    f();
    
    if (!enabled)
        ImGui::EndDisabled();
}

bool BeginLabeledField();
void EndLabeledField(std::string_view label);

std::string GetComponentCollapsingHeaderName(Component& component, std::string_view name);

enum PopupStatus
{
    NONE = 0,
    OK,
    CANCEL,
    YES,
    NO,
    CONFIRM,
};

enum class ModalType
{
    YES_NO,
    OK_CANCEL,
    OK,
    CONFIRM_CANCEL,
};

bool BeginModal(std::string_view title, bool openState);
PopupStatus EndModal(ModalType modalType);

template<class T>
bool EditValue(T& val);

template<>
bool EditValue(std::string& val)
{
    return ImGui::InputText("##", &val);
}

template<>
bool EditValue(int& val)
{
    return ImGui::DragInt("##", &val);
}

template<>
bool EditValue(float& val)
{
    return ImGui::DragFloat("##", &val);
}

// Returns true when value has been applied
template<class T>
bool EditValueModal(T& val)
{
    static T temp;
    
    bool openState = ImGui::Button(ICON_FA_PENCIL);
    if (BeginModal("Edit Value", openState))
    {
        if (openState)
            temp = val;
        
        EditValue<T>(temp);
        
        if (auto res = EndModal(ModalType::CONFIRM_CANCEL))
        {
            if (res == CONFIRM)
            {
                val = temp;
                return true;
            }
        }
    }
    return false;
}

}