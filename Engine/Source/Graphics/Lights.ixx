export module Graphics:Lights;

import Common;

export struct DirectionalLightData
{
    Vector3 color alignas(16);
    float intensity;
    Vector3 direction;
};

export constexpr int MaxLights = 20;

export struct PunctualLightData
{
    enum PunctualLightType : uint8_t // Structured buffer means that we no longer have to manage padding :-)
    {
        POINT,
        SPOT
    } type;
    Vector3 position;
    Vector3 color;
    float intensity;
    float falloff; // 1 / (attenuationRadius * attenuationRadius)

    // For spot lights
    Vector3 direction;
    Vector2 angleScaleOffset; // Contains angleScale and angleOffset, computed from inner and outer angles
};