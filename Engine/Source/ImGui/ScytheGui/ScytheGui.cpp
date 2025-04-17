module;
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "Common/Macros.hpp"
module ImGui:ScytheGui;
import ImGui;

import :ScytheGui;
import Graphics;
import Components;

import <format>;

namespace ScytheGui
{

bool BeginLabeledField()
{
    float w = ImGui::CalcItemWidth();
    bool table = ImGui::BeginTable("##", 2, ImGuiTableFlags_SizingStretchSame);
    if (table)
    {
        ImGui::TableSetupColumn("##1", ImGuiTableColumnFlags_WidthFixed, w - 5);
        ImGui::TableSetupColumn("##2", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
    }
    return table;
}

void EndLabeledField(std::string_view label)
{
    ImGui::TableNextColumn();
    ImGui::Text(label.data());
    ImGui::EndTable();
}

bool BeginContextMenu(std::string_view popupId)
{
    return ImGui::BeginPopupContextItem(popupId.data());
}

bool ContextMenuButton(std::string_view label)
{
    const bool ret = ImGui::Button(label.data());
    if (ret)
        ImGui::CloseCurrentPopup();
    return ret;
}

void EndContextMenu()
{
    ImGui::EndPopup();
}

void Header(std::string_view name, ImFont* headerFont)
{
    ImGui::PushFont(headerFont);
    ImGui::Text(name.data());
    ImGui::PopFont();
}

void SmallText(std::string_view name, ImFont* smallFont)
{
    ImGui::PushFont(smallFont);
    ImGui::Text(name.data());
    ImGui::PopFont();
}

void TextTooltip(std::string_view sv, bool allowDisabled)
{
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | (allowDisabled ? ImGuiHoveredFlags_AllowWhenDisabled : ImGuiHoveredFlags_None)) && ImGui::BeginTooltip())
    {
        ImGui::Text(sv.data());
        ImGui::EndTooltip();
    }
}

bool BeginTooltipOnHovered()
{
    return ImGui::IsItemHovered() && ImGui::BeginTooltip();
}

void InfoBubble(std::string_view text, std::string_view label)
{
    ImGui::TextDisabled(label.data());
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(text.data());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

ImRect BeginFlameGraph(const char* title, const char* overlayText, ImU8 maxDepth)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return {};

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    const float blockHeight = ImGui::GetTextLineHeight() + style.FramePadding.y * 2;
    const ImVec2 labelSize = ImGui::CalcTextSize(title, nullptr, true);
    ImVec2 graphSize = { ImGui::CalcItemWidth() + 200,
                         labelSize.y + style.FramePadding.y * 3 + blockHeight * (maxDepth + 1) };

    const ImRect frameBB(window->DC.CursorPos, window->DC.CursorPos + graphSize);
    const ImRect innerBB(frameBB.Min + style.FramePadding, frameBB.Max - style.FramePadding);
    const ImRect totalBB(frameBB.Min,
                         frameBB.Max + ImVec2(labelSize.x > 0.0f ? style.ItemInnerSpacing.x + labelSize.x : 0.0f, 0));
    ImGui::ItemSize(totalBB, style.FramePadding.y);
    if (!ImGui::ItemAdd(totalBB, 0, &frameBB))
        return {};

    ImGui::RenderFrame(frameBB.Min, frameBB.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

    // Text inside the flame graph
    if (overlayText)
        ImGui::RenderTextClipped(ImVec2(frameBB.Min.x, frameBB.Min.y + style.FramePadding.y), frameBB.Max, overlayText,
                                 nullptr, nullptr, ImVec2(0.5f, 0.0f));

    // Text to the right of the flame graph (title)
    if (labelSize.x > 0.0f)
        ImGui::RenderText(ImVec2(frameBB.Max.x + style.ItemInnerSpacing.x, innerBB.Min.y), title);

    const float scopeHeight = ImGui::GetTextLineHeight() + style.FramePadding.y * 2;
    // Adjust totalBB to exclude scope area from clipping
    ImRect clipBB(innerBB.Min, ImVec2(innerBB.Max.x - style.ItemInnerSpacing.x, totalBB.Max.y + scopeHeight));
    ImGui::PushClipRect(clipBB.Min, clipBB.Max, true);

    return innerBB;
}

ImRect FlameGraphFrame(ImRect totalBB, float cumulativeWidth, float frameTime, float uiZoomFactor, float totalProfilingTime,
                                  int currentFrameNb, ImU8 maxDepth)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return {};

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    const float totalGraphWidth = uiZoomFactor * totalBB.GetWidth();
    const float frameTimeProportionOfTotalTime = frameTime / totalProfilingTime;

    const ImRect frameBB = { totalBB.Min.x + cumulativeWidth, totalBB.Min.y,
                       totalBB.Min.x + cumulativeWidth + totalGraphWidth * frameTimeProportionOfTotalTime, totalBB.Max.y };

    const float scopeHeight = ImGui::GetTextLineHeight() + style.FramePadding.y * 2;
    const float height = scopeHeight * (maxDepth + 1) - style.FramePadding.y;

    ImVec2 pos0 = { frameBB.Min.x, frameBB.Min.y + height + scopeHeight };
    ImVec2 pos1 = { frameBB.Max.x, frameBB.Min.y + height + 2 * scopeHeight };

    std::string frameName = std::format("Frame {}", currentFrameNb).c_str();

    // Draw frame name (clipped if not enough size)
    ImVec2 textSize = ImGui::CalcTextSize(frameName.c_str());
    if (ImVec2 boxSize = pos1 - pos0; textSize.x < boxSize.x)
    {
        ImVec2 textOffset = ImVec2(0.5f, 0.5f) * (boxSize - textSize);
        ImGui::RenderText(pos0 + textOffset, frameName.c_str());
    }
    else
    {
        std::string shortFrameName = std::format("F.{}", currentFrameNb).c_str();
        ImVec2 shortTextSize = ImGui::CalcTextSize(shortFrameName.c_str());
        if (shortTextSize.x < boxSize.x)
        {
            ImVec2 textOffset = ImVec2(0.5f, 0.5f) * (boxSize - shortTextSize);
            ImGui::RenderText(pos0 + textOffset, shortFrameName.c_str());
        }
    }

    // Draw dotted lines for frame boundaries
    const ImU32 lineColor = ImGui::GetColorU32(ImGuiCol_Border);
    const float dashLength = 4.0f;
    const float gapLength = 2.0f;
    float currentPos = frameBB.Min.y;
    while (currentPos < pos1.y)
    {
        ImVec2 lineStart = ImVec2(frameBB.Max.x, currentPos);
        ImVec2 lineEnd = ImVec2(frameBB.Max.x, std::min(currentPos + dashLength, frameBB.Max.y));
        window->DrawList->AddLine(lineStart, lineEnd, lineColor);
        currentPos += dashLength + gapLength;
    }

    return frameBB;
}

std::optional<std::pair<bool, float>> FlameGraphScope(const char* scopeName, ImVec2 scopeStartEnd, float cumulativeWidth, float frameTime, ImRect frameBB, ImU8 scopeDepth, ImU8 maxDepth)
{
    static const ImU32 ColBase = ImGui::GetColorU32(ImGuiCol_PlotHistogram) & 0x77FFFFFF;
    static const ImU32 ColHovered = ImGui::GetColorU32(ImGuiCol_PlotHistogramHovered) & 0x77FFFFFF;
    static const ImU32 ColOutlineBase = ImGui::GetColorU32(ImGuiCol_PlotHistogram) & 0x7FFFFFFF;
    static const ImU32 ColOutlineHovered = ImGui::GetColorU32(ImGuiCol_PlotHistogramHovered) & 0x7FFFFFFF;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return std::nullopt;

    float scopeTime = scopeStartEnd.y - scopeStartEnd.x;
    float scopeTimeProportionOfFrameTime = scopeTime / frameTime;
    const float scopeHeight = ImGui::GetTextLineHeight() + style.FramePadding.y * 2;

    float frameWidth = frameBB.GetWidth();
    float frameHeight = scopeHeight * (maxDepth - scopeDepth + 1) - style.FramePadding.y;

    ImVec2 pos0 = frameBB.Min + ImVec2(cumulativeWidth, frameHeight);
    ImVec2 pos1 = frameBB.Min + ImVec2(cumulativeWidth + frameWidth * scopeTimeProportionOfFrameTime, frameHeight + scopeHeight);

    float scopeWidth = pos1.x - pos0.x;

    bool hasHovered = false;
    if (ImGui::IsMouseHoveringRect(pos0, pos1))
    {
        ImGui::SetTooltip("%s: %8.4gms", scopeName, scopeTime);
        hasHovered = true;
    }

    window->DrawList->AddRectFilled(pos0, pos1, hasHovered ? ColHovered : ColBase);
    window->DrawList->AddRect(pos0, pos1, hasHovered ? ColOutlineHovered : ColOutlineBase);
    ImVec2 textSize = ImGui::CalcTextSize(scopeName);
    ImVec2 boxSize = pos1 - pos0;
    if (textSize.x < boxSize.x)
    {
        ImVec2 textOffset = ImVec2(0.5f, 0.5f) * (boxSize - textSize);
        ImGui::RenderText(pos0 + textOffset, scopeName);
    }

    return { { hasHovered, scopeWidth } };
}

void EndFlameGraph()
{
    ImGui::PopClipRect();
}

// Selected value will equal -1 if the noneOption is selected.
// If noneOption is nullptr, the selected value will belong in [0, numElements - 1]
bool ComboBoxDetailsPanel(const char* title, int& selectedValue, std::vector<std::string>& comboBoxPossibleValues, const char* noneOption)
{
    int numElements = static_cast<int>(comboBoxPossibleValues.size() + (noneOption != nullptr));
    std::vector<const char*> namesConstChar(numElements);

    if (noneOption)
        namesConstChar[0] = noneOption;

    for (int i = noneOption != nullptr; i < numElements; ++i)
        namesConstChar[i] = comboBoxPossibleValues[i - (noneOption != nullptr)].c_str();

    if (noneOption)
        ++selectedValue;

    bool valueChanged = ImGui::Combo(title, &selectedValue, namesConstChar.data(),
                        static_cast<int>(namesConstChar.size()));

    if (noneOption)
        --selectedValue;

    return valueChanged;
}

void MaskedImage(Texture& texture, const ImVec2& size, bool mask[4])
{
    ImVec4 maskTint(mask[0], mask[1], mask[2], mask[3]);
    ImGui::Image(texture.GetSRV().Get(), size, ImVec2(0,0), ImVec2(1,1), maskTint);
}

std::string GetComponentCollapsingHeaderName(Component& component, std::string_view name) 
{ 
    return std::format("{}##{}", name, component.GetOwningEntity());
}

bool BeginModal(std::string_view title, bool openState)
{
    if (openState)
    {
        ImGui::OpenPopup(title.data());
    }
    
    return ImGui::BeginPopupModal(title.data(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);
}

PopupStatus EndModal(ModalType modalType)
{
    PopupStatus status = NONE;

    switch (modalType) 
    {
        case ModalType::YES_NO:
        {
            if (ImGui::Button("Yes"))
                status = YES;
            ImGui::SameLine();
            if (ImGui::Button("No"))
                status = NO;
        } break;
        case ModalType::OK:
        {
            if (ImGui::Button("Ok"))
                status = OK;
        } break;
        case ModalType::OK_CANCEL:
        {
            if (ImGui::Button("Ok"))
                status = OK;
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
                status = CANCEL;
        } break;
        case ModalType::CONFIRM_CANCEL:
        {
            if (ImGui::Button("Confirm"))
                status = CONFIRM;
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
                status = CANCEL;
        } break;
    };

    if (status != NONE)
        ImGui::CloseCurrentPopup();
    
    ImGui::EndPopup();

    return status;
}

}
