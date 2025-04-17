module;

#include "Common/Macros.hpp"
#include "Physics/PhysicsSystem-Headers.hpp"
#include <Jolt/Physics/PhysicsSystem.h>

export module Systems.Physics:BodySettings;

import Serialization;

export using AllowedDegreesOfFreedom = JPH::EAllowedDOFs;
export using MotionQuality = JPH::EMotionQuality;

export struct PhysicsBodySettings
{
    // Which axis is the system free to use
    int degreesOfFreedom = static_cast<int>(AllowedDegreesOfFreedom::All);

    // Whether the collision detection will be descrete or continuous
    MotionQuality motionQuality = MotionQuality::Discrete;

    float mass = 0.0f;
    float friction = 0.2f;
    float restitution = 0.0f;

    bool ImGuiDraw();
};

#ifdef IMGUI_ENABLED

import ImGui;

bool PhysicsBodySettings::ImGuiDraw()
{
    bool result = false;

    if (ScytheGui::BeginLabeledField())
    {
        result |= ScytheGui::CheckboxFlags("X", degreesOfFreedom, AllowedDegreesOfFreedom::TranslationX);
        ImGui::SameLine();
        result |= ScytheGui::CheckboxFlags("Y", degreesOfFreedom, AllowedDegreesOfFreedom::TranslationY);
        ImGui::SameLine();
        result |= ScytheGui::CheckboxFlags("Z", degreesOfFreedom, AllowedDegreesOfFreedom::TranslationZ);

        ScytheGui::EndLabeledField("Translation Constraint");
    }
    ScytheGui::TextTooltip("This constraints this body's translation on different axes. Axes are in world space.");
    
    if (ScytheGui::BeginLabeledField())
    {
        result |= ScytheGui::CheckboxFlags("Pitch", degreesOfFreedom, AllowedDegreesOfFreedom::RotationX);
        ImGui::SameLine();
        result |= ScytheGui::CheckboxFlags("Yaw", degreesOfFreedom, AllowedDegreesOfFreedom::RotationY);
        ImGui::SameLine();
        result |= ScytheGui::CheckboxFlags("Roll", degreesOfFreedom, AllowedDegreesOfFreedom::RotationZ);
        
        ScytheGui::EndLabeledField("Rotation Constraint");
    }
    ScytheGui::TextTooltip("This constraints this body's rotation on different axes. Axes are in world space.");

    result |= ScytheGui::ComboEnum("Motion Quality", motionQuality);
    ScytheGui::TextTooltip("Motion Quality defines the precision of collision and simulation."
                           "\n\tUsing Discrete is faster but may cause imprecision in collisions."
                           "\n\tUsing LinearCast will give much more precise results with a higher cost");
    
    result |= ImGui::DragFloat("Mass", &mass, 0.5, 0.00001, std::numeric_limits<float>::max(), "%.3f", ImGuiSliderFlags_AlwaysClamp);
    ScytheGui::TextTooltip("Defines the custom mass of this body."
                           "\nSetting it to 0 will compute the mass by the properties of the body");

    result |= ImGui::SliderFloat("Friction", &friction, 0, 1);
    ScytheGui::TextTooltip("Defines the friction property of this body."
                           "\n0 means no friction and 1 means full friction");
                           
    result |= ImGui::SliderFloat("Restitution", &restitution, 0, 1);
    ScytheGui::TextTooltip("Defines the restitution property of this body."
                           "\n0 means a completely inelastic collision and 1 means fully ellastic (or bouncy)");

    return result;
}

#endif

DECLARE_SERIALIZABLE(PhysicsBodySettings)
DEFINE_SERIALIZATION_AND_DESERIALIZATION(PhysicsBodySettings, degreesOfFreedom, motionQuality, mass, friction, restitution)
