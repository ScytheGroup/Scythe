module Graphics:RasterizerState;

import :RasterizerState;

RasterizerState::RasterizerState(Device& device, Culling culling, FillMode fillMode, TriangleWinding winding, const DepthBiasInfo& biasInfo)
{
    D3D11_RASTERIZER_DESC desc;
    desc.CullMode = static_cast<D3D11_CULL_MODE>(culling);
    desc.FillMode = static_cast<D3D11_FILL_MODE>(fillMode);
    desc.FrontCounterClockwise = winding == TriangleWinding::COUNTER_CLOCKWISE;
    
    desc.SlopeScaledDepthBias = biasInfo.slopeScaledDepthBias;
    desc.DepthBias = biasInfo.depthBias;
    desc.DepthBiasClamp = biasInfo.depthBiasClamp;
    desc.DepthClipEnable = biasInfo.depthClipEnable;

    desc.AntialiasedLineEnable = true;
    desc.MultisampleEnable = false;
    desc.ScissorEnable = false;
    rasterizerState = device.CreateRasterizerState(desc);
}
