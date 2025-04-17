module;
#include "Common/Macros.hpp"

#include "Physics/PhysicsSystem-Headers.hpp"
#include "Jolt/Physics/Character/CharacterVirtual.h"
export module Components:CharacterSettings;

import Common;
import Serialization;

export class CharacterSettings final
{
public:
    float mass = 70.0f;
    float maxStrength = 100.f;
    Vector3 shapeOffset = Vector3::Zero;

    JPH::CharacterVirtualSettings GetInnerSettings()
    {
        JPH::CharacterVirtualSettings innerSettings;
        innerSettings.mMass = mass;
        innerSettings.mMaxStrength = maxStrength;
        innerSettings.mShapeOffset = JPH::Vec3(shapeOffset.x,shapeOffset.y, shapeOffset.z);

        return innerSettings;
    }

#ifdef IMGUI_ENABLED
    bool ImGuiDraw();
#endif
};

DECLARE_SERIALIZABLE(CharacterSettings);
