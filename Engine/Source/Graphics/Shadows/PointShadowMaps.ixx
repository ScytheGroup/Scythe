export module Graphics:PointShadowMaps;

import Common;

import :ShadowMap;
import :StructuredBuffer;

export class MeshComponent;
export class PunctualLightComponent;
export class GlobalTransform;
export class EntityComponentSystem;
export struct GraphicsScene;

export class PointShadowMaps : public ShadowMap
{
public:
    struct PointLightData
    {
        Vector3 lightPosition;
        float farPlane;
    };

    uint32_t numMaps = 0;
private:
    
    ConstantBuffer<PointLightData> pointLightBuffer;
    std::vector<PointLightData> pointLightData;
    void Render(GraphicsScene& scene, uint32_t index, const std::tuple<GlobalTransform*, PunctualLightComponent*>& entity, const Array<std::tuple<GlobalTransform*, MeshComponent*>>& meshes);
public:
    void Init(Device& device) override;
    void Render(GraphicsScene& scene, EntityComponentSystem& ecs);

    StructuredBuffer<PointLightData> pointLightDataBuffer;
};
