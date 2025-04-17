module Graphics:BlendState;

import :BlendState;

BlendState::BlendState(Device& device, BlendFormula colorBlending, BlendFormula alphaBlending, uint8_t blendMask)
{
    D3D11_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].BlendEnable = true;
    blendDesc.RenderTarget[0].SrcBlend = colorBlending.srcBlend;
    blendDesc.RenderTarget[0].DestBlend = colorBlending.destBlend;
    blendDesc.RenderTarget[0].BlendOp = colorBlending.blendOp;
    blendDesc.RenderTarget[0].SrcBlendAlpha = alphaBlending.srcBlend;
    blendDesc.RenderTarget[0].DestBlendAlpha = alphaBlending.destBlend;
    blendDesc.RenderTarget[0].BlendOpAlpha = alphaBlending.blendOp;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = blendMask;
    blendState = device.CreateBlendState(blendDesc);
}
