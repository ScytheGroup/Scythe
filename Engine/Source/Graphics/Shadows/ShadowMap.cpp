module Graphics:ShadowMap;

import :ShadowMap;
import :ResourceManager;

void ShadowMap::Init(Device& device)
{
    ResourceManager& rm = ResourceManager::Get();

    vsShader = rm.shaderLoader.Load<VertexShader>(ShaderKey{ "WriteToDepth.vs.hlsl", { { "STORE_LINEAR_DEPTH", linearDepth ? "1" : "0" } } }, device);
    psShader = rm.shaderLoader.Load<PixelShader>(ShaderKey{ "WriteToDepth.ps.hlsl", { { "STORE_LINEAR_DEPTH", linearDepth ? "1" : "0" } } }, device);

    viewport.Width = static_cast<FLOAT>(size);
    viewport.Height = static_cast<FLOAT>(size);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
}
