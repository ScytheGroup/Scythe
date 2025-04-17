export module Graphics:DirectionalCascadedShadowMap;

import Common;
import :Resources;
import :ShadowMap;

export struct DirectionalLightData;
export class VertexShader;
export class PixelShader;
export struct GraphicsScene;
export class CameraComponent;
export class EntityComponentSystem;

class DirectionalCascadedShadowMap : public ShadowMap
{
	std::vector<Vector3> cascades = {
		Vector3{0.1f, 50.0f, 0.001},
		Vector3{47.5f, 200.0f, 0.001},
		Vector3{180.0f, 1000.0f, 0.003}
	};

    bool debugBounds = false;
public:

    static constexpr uint8_t CASCADE_COUNT = 3;
    struct alignas(16) CascadeShadowData
    {
        Matrix matrix;
        Vector3 cascadeBound alignas(16);
    };

    using CascadesData = std::array<CascadeShadowData, CASCADE_COUNT>;
private:
    std::array<std::pair<Matrix, DirectX::BoundingOrientedBox>, CASCADE_COUNT> cascadeInfo;
    void UpdateCascadeBuffer(Device& device);
    std::pair<Matrix, DirectX::BoundingOrientedBox> GetLightSpaceMatrix(const Transform& cameraTransform, const CameraComponent& cameraComp, const DirectionalLightData& light, int cascadeId);

public:
    ConstantBuffer<CascadesData> cascadeShadowBuffer;
    
	void Init(Device& device) override;
    void Render(GraphicsScene& scene, EntityComponentSystem& ecs);

#ifdef IMGUI_ENABLED
    bool ImGuiDraw();
#endif
};
