module;
#include "Common/Macros.hpp"
#include "Serialization/SerializationMacros.h"
export module Components:PunctualLight;

import :Component;
import :Transform;

import Common;

export class PunctualLightComponent : public Component
{
    SCLASS(PunctualLightComponent, Component);
    REQUIRES(TransformComponent);
    MANUALLY_DEFINED_EDITOR();
public:
    PunctualLightComponent();

    enum LightType : uint8_t
    {
        POINT,
        SPOT
    } type = POINT;

    // Point and spot lights
    Vector3 color = Vector3::One;
    float intensity = 100.0f;
    float attenuationRadius = 20.0f;
    bool useInverseSquaredFalloff = false;

    // Spot lights only
    float innerAngle = std::numbers::pi_v<float> / 4;
    float outerAngle = std::numbers::pi_v<float> / 3;
};

DECLARE_SERIALIZABLE(PunctualLightComponent)