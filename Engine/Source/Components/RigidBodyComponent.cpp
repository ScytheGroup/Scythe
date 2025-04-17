module;
#include "Physics/PhysicsSystem-Headers.hpp"

#include "Jolt/Physics/Collision/Shape/Shape.h"
#include "Common/Macros.hpp"

module Systems.Physics;
import Core;

RigidBodyComponent::RigidBodyComponent()
{
}

RigidBodyComponent::RigidBodyComponent(const RigidBodyComponent& other)
    : Super(other) 
    , motionType{other.motionType}
    , layer{other.layer}
    , showDebug{other.showDebug}
{
}

RigidBodyComponent::~RigidBodyComponent()
{
}

#ifdef EDITOR
import ImGui;

bool RigidBodyComponent::DrawEditorInfo()
{
    bool result = false;
    result |= ImGui::Checkbox("Show Debug", &showDebug);
    ScytheGui::TextTooltip("Displays a wireframe mesh over the collision of that rigid body and it's shapes");
    
    ImGui::Separator();

    result |= ScytheGui::ComboEnum("MotionType", motionType);
    ScytheGui::TextTooltip(
        "The motion type will allow or not for an object to move."
        "\n\tSTATIC: Object will not be able to move"
        "\n\tDYNAMIC: Object will be able to move and interact with the scene"
    );
    
    result |= ScytheGui::ComboEnum("Layer", layer, true);
    if (ScytheGui::BeginTooltipOnHovered())
    {
        ImGui::Text("Layer that objects can be in, determines which other objects it can collide with.");
        // Warning
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::Text("At the moment selecting dynamic motion type with something other than moving and static with something other than non-moving will result in eratic behaviour");
        ImGui::PopStyleColor();
        ImGui::EndTooltip();
    }

    ImGui::SeparatorText("Body Settings");
    result |= bodySettings.ImGuiDraw();
    
    return result;
}
#endif

DEFINE_SERIALIZATION_AND_DESERIALIZATION(RigidBodyComponent, layer, motionType, bodySettings)
