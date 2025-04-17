module;
#include "Serialization/SerializationMacros.h"
module Components;

import :Character;

import Systems.Physics;

import Serialization;

DEFINE_SERIALIZATION_AND_DESERIALIZATION(CharacterControllerComponent, settings);

void CharacterControllerComponent::UpdateInnerSettings()
{
    innerCharacter->SetMass(settings.mass);
    innerCharacter->SetMaxStrength(settings.maxStrength);
    innerCharacter->SetShapeOffset(Convert(settings.shapeOffset));
}

Vector3 CharacterControllerComponent::GetGroundVelocity()
{
    return Convert(innerCharacter->GetGroundVelocity());
}

GroundState CharacterControllerComponent::GetGroundState() const
{
    return innerCharacter->GetGroundState();
}

#ifdef EDITOR

import ImGui;

bool CharacterControllerComponent::DrawEditorInfo()
{
    bool result = settings.ImGuiDraw();
    result |= ImGui::Checkbox("Enable Debug Display", &showDebug);
    return result;
}

#endif
