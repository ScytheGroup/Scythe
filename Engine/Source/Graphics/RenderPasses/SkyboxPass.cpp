module;
#include "../Utils/EventScopeMacros.h"
module Graphics:RenderPass;

import :RenderPass;
import :Skybox;
import :RasterizerState;
import :FrameBuffer;
import :DepthStencil;
import :Scene;

import Core;
import Graphics;

void SkyboxPass::Init(Device& device, SceneSettings& sceneSettings)
{
    Recreate(device, sceneSettings);
}

void SkyboxPass::Render(GraphicsScene& scene, SceneSettings& sceneSettings)
{
    SC_SCOPED_GPU_EVENT("Skybox");

    if (sceneSettings.graphics.enableAtmosphere)
    {
        return;
    }
    
    if (!skybox || skyboxTexturePath != sceneSettings.graphics.skyboxPath)
    {
        Recreate(scene.device, sceneSettings);
        scene.invalidateIBL = true;
    }
    
    scene.device.SetRasterizerState(RasterizerStates::Get<RasterizerPresets::FrontfaceCulling>(scene.device));

    scene.device.SetShadingModel(skybox->shadingModel);
    scene.device.SetShaderResource(skybox->shadingModel.pixelShader->GetTextureSlot("SkyboxTexture"), skybox->texture, PIXEL_SHADER);
    
    scene.device.SetFrameBuffer(scene.sceneColorBuffer, scene.gBuffer.depthStencil);

    scene.device.SetShadingModel(skybox->shadingModel);
    scene.device.SetShaderResource(skybox->shadingModel.pixelShader->GetTextureSlot("SkyboxTexture"), skybox->texture, PIXEL_SHADER);
    
    Graphics::DrawGeometry(scene.device, skybox->transform, *skybox->geometry);
}

const Skybox* SkyboxPass::GetSkybox() const
{
    return !skybox ? nullptr : &*skybox;
}

void SkyboxPass::Recreate(Device& device, SceneSettings& sceneSettings)
{
    // Atmosphere currently doesn't support IBL.
    // Supporting this would require quite a few changes to allow for rendering the scene in a stateless way in any arbitrary camera position/direction.
    if (!sceneSettings.graphics.enableAtmosphere)
    {
        skyboxTexturePath = sceneSettings.graphics.skyboxPath;
        Texture::TextureData textureData{ sceneSettings.graphics.skyboxPath, Texture::TEXTURE_2D, Texture::DEFAULT, Texture::TEXTURE_SRV, false };
        Texture equirectangleTexture{ textureData, device };
        Texture cubemapTexture = CubemapHelpers::ConvertEquirectangularTextureToCubeMap(equirectangleTexture, device);
        skybox.emplace(std::move(cubemapTexture), device);
    }
}

#ifdef IMGUI_ENABLED


bool SkyboxPass::ImGuiDraw()
{
    if (!skybox)
        return false;
    
    return skybox->ImGuiDraw();
}
#endif
