module Graphics:DepthStencilState;

import :DepthStencilState;

DepthStencilState::DepthStencilState(Device& device, D3D11_COMPARISON_FUNC comparisonFunc, D3D11_DEPTH_WRITE_MASK depthWriteMask)
{
    // We could technically cache a depth state for each comparisonFunc/depthWriteMask combo which is a lot of combinations.
    // For now this unoptimized version is fine!
    D3D11_DEPTH_STENCIL_DESC depthDesc{};
    depthDesc.DepthEnable = true;
    depthDesc.DepthFunc = comparisonFunc;
    depthDesc.DepthWriteMask = depthWriteMask;

    depthStencilState = device.CreateDepthStencilState(depthDesc);
}
