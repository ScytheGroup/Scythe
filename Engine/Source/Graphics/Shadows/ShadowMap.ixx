export module Graphics:ShadowMap;

import Common;
import :DepthStencil;
import :ConstantBuffer;

export class EntityComponentSystem;
export struct GraphicsScene;
export class PixelShader;
export class VertexShader;

export class ShadowMap
{
protected:
    D3D11_VIEWPORT viewport{};
    
    VertexShader* vsShader{};
    PixelShader* psShader{};
    
    ConstantBuffer<Matrix> viewProjBuffer;
public:
    uint32_t size;
    bool linearDepth = false;
    std::optional<DepthStencil> shadowMap;

    ShadowMap() = default;
    ShadowMap(ShadowMap&&) = default;
    ShadowMap& operator=(ShadowMap&&) = default;
    
    virtual ~ShadowMap() = default;

    virtual void Init(Device& device);
};

