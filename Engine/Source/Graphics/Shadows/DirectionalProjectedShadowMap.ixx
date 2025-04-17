export module Graphics:DirectionalProjectedShadowMap;

import :ShadowMap;

export class MeshComponent;
export class PunctualLightComponent;

export class DirectionalProjectedShadowMap : public ShadowMap
{
    Vector3 boundsCenter = Vector3::Zero;
    Vector3 boundsExtents = Vector3::One * 50;

    Matrix GetViewMatrix(const Vector3& lightDirection);
    Matrix GetProjectionMatrix(const Matrix& viewMatrix, const Vector3& lightDirection);
public:
    void Init(Device& device) override;
    void Render(GraphicsScene& scene, EntityComponentSystem& ecs);
    void SetBounds(const Vector3& center, const Vector3& extents);

    ConstantBuffer<Matrix> viewProjMatrixBuffer;
};
