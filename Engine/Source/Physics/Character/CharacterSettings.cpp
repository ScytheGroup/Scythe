module;
#include "Common/Macros.hpp"
module Components;

import :CharacterSettings;

DEFINE_SERIALIZATION_AND_DESERIALIZATION(CharacterSettings, mass, maxStrength, shapeOffset);


#ifdef IMGUI_ENABLED

import ImGui;

bool CharacterSettings::ImGuiDraw()
{
    bool result = false;
    result |= ImGui::DragFloat("Mass", &mass, 0.5, 0.00001, std::numeric_limits<float>::max(), "%.3f", ImGuiSliderFlags_AlwaysClamp);
    ScytheGui::TextTooltip("Defines the custom mass of this body."
                           "\nSetting it to 0 will compute the mass by the properties of the body");

    result |= ImGui::SliderFloat("MaxStrength", &maxStrength, 0, 1);
    ScytheGui::TextTooltip("Defines the max strength at which the character can push things.");
                           
    result |= ImGui::DragFloat3("Shape offset", &shapeOffset.x);
    ScytheGui::TextTooltip("Offset of the character shape");

    return result;
}

#endif