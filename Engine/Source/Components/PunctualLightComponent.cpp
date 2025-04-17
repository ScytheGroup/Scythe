module;

#include "Serialization/SerializationMacros.h"

module Components:PunctualLight;

import Core;
import :PunctualLight;

PunctualLightComponent::PunctualLightComponent()
{}

#ifdef EDITOR

import ImGui;

bool PunctualLightComponent::DrawEditorInfo()
{
    bool result = false;

    result |= ScytheGui::ComboEnum("Type", type);
    result |= ImGui::ColorEdit3("Color", &color.x);
    result |= ImGui::DragFloat("Intensity (in Candela)", &intensity, 0.1f, 0.1f, std::numeric_limits<float>::max());

    ImGui::Checkbox("Use Inverse-Squared Attenuation", &useInverseSquaredFalloff);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Default (Non-Inverse-Squared) attenuation looks better in most situations.\nInverse squared will give a sharper falloff at the edge of the attenuation radius.");
    }

    result |= ImGui::DragFloat("Attenuation Radius", &attenuationRadius, 0.1f, 0.1f, std::numeric_limits<float>::max());
    
    if (type == SPOT)
    {
        result |= ImGui::SliderAngle("Inner Angle", &innerAngle, 0, 89.999f);
        result |= ImGui::SliderAngle("Outer Angle", &outerAngle, 0, 89.999f);
    }
    return result;
}

#endif

DEFINE_SERIALIZATION_AND_DESERIALIZATION(PunctualLightComponent, color, intensity, type, attenuationRadius, intensity, innerAngle, outerAngle);
