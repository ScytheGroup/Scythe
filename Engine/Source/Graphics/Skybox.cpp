module Graphics:Skybox;

import :Skybox;
import :Device;
import :ResourceManager;

Skybox::Skybox(Texture&& cubemapTexture, Device& device)
    : geometry{ MeshPresets::GetCubeGeometry() }
    , texture{std::move(cubemapTexture)}
{
    if (texture.GetType() != Texture::TEXTURE_CUBE)
        DebugPrintError("Skybox texture passed was not a cubemap");
    
    auto& resourceManager = ResourceManager::Get();

    ShaderKey vsKey;
    vsKey.filepath = "Skybox.vs.hlsl";
    shadingModel.vertexShader = resourceManager.shaderLoader.Load<VertexShader>(std::move(vsKey), device);

    ShaderKey psKey;
    psKey.filepath = "Skybox.ps.hlsl";
    psKey.defines = { { "NO_NORMAL_WRITE", "" } };
    shadingModel.pixelShader = resourceManager.shaderLoader.Load<PixelShader>(std::move(psKey), device);

    transform = Transform{ Matrix::CreateScale(2, 2, 2) };
}

#ifdef IMGUI_ENABLED

import ImGui;

bool Skybox::ImGuiDraw()
{
    texture.ImGuiDraw();
    return false;
}


#endif