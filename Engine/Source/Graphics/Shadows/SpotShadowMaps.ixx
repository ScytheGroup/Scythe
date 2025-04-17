export module Graphics:SpotShadowMap;

import Common;

import :DepthStencil;
import :ShadowMap;
import :StructuredBuffer;

export class MeshComponent;
export class GlobalTransform;
export class PunctualLightComponent;
export struct GraphicsScene;
export class EntityComponentSystem;

export class SpotShadowMaps : public ShadowMap
{
    std::vector<Matrix> spotLightMatrices;
    void Render(GraphicsScene& scene, int index, const std::tuple<GlobalTransform*, PunctualLightComponent*>& entity, const Array<std::tuple<GlobalTransform*, MeshComponent*>>& meshes);
public:
    void Init(Device& device) override;
    void Render(GraphicsScene& scene, EntityComponentSystem& ecs);

    uint32_t numMaps = 0;
    StructuredBuffer<Matrix> spotLightMatricesBuffer;
};

