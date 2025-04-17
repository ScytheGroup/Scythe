export module Graphics:GraphicsDefs;

import Common;
import :Lights;

struct CameraProjection;

enum class ToneMappingType
{
    ACES = 0,
    REINHARD = 1,
    REINHARD_JODIE = 2,
};

export enum PipelineStage : uint8_t
{
    VERTEX_SHADER = 1 << 0,
    PIXEL_SHADER = 1 << 1,
    COMPUTE_SHADER = 1 << 2,
    GEOMETRY_SHADER = 1 << 3
};

export using PipelineStageFlags = std::underlying_type_t<PipelineStage>;

export struct FrameData
{
    Matrix view;
    Matrix projection;
    Matrix viewProjection;
    Matrix invView;
    Matrix invProjection;
    Matrix invViewProjection;
    Vector3 cameraPosition;
    Vector2 screenSize alignas(16);
    float frameTime;
    float elapsedTime = 0;
    uint32_t punctualLightsCount; // Number of punctual lights
    DirectionalLightData directionalLight;

    void UpdateView(const Transform& cameraTransform, const CameraProjection& cameraProjection);
};

export struct ObjectData
{
    Matrix modelMatrix;
    Matrix normalMatrix;
};
