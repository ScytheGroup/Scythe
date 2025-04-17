module;
#include "../Utils/EventScopeMacros.h"
module Graphics:RenderPass;

import :RenderPass;
import :RasterizerState;
import :Scene;
import :TextureHelpers;
import :CubemapHelpers;

#ifdef RENDERDOC
import :RenderDoc;
#endif

import Graphics;
import Core;

void DeferredLightingPass::Init(Device& device, const Skybox* skybox, const AtmospherePass& atmospherePass, const SceneSettings& sceneSettings)
{
    // BRDFLUT will never change.
    iblData.RecreateBRDFLUT(device);

    // IBL though can change depending on scene settings!
    RegenerateIBL(device, skybox ? &skybox->texture : nullptr, atmospherePass, sceneSettings);
}

void DeferredLightingPass::Render(
    GraphicsScene& scene,
    SceneShadows& shadowsData,
#if VXGI_ENABLED
    Texture* illuminationVolume,
#endif
    SceneSettings& sceneSettings, const AtmospherePass& atmospherePass, const Texture* skybox)
{
    SC_SCOPED_GPU_EVENT("Deferred Lighting");

    bool doIBLPass = settings.enableIBL && !scene.invalidateIBL;
    bool useSunTransmittanceForDirectional = doIBLPass && sceneSettings.graphics.enableAtmosphere;
    
    scene.device.SetRasterizerState(RasterizerStates::Get<RasterizerPresets::BackfaceCulling>(scene.device));
    scene.device.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    scene.device.SetConstantBuffers(Graphics::FrameCBufferIndex, ViewSpan{ &scene.frameDataBuffer }, VERTEX_SHADER | PIXEL_SHADER);
    scene.device.SetShaderResource(Graphics::PunctualLightSRVIndex, scene.lightDataBuffer, VERTEX_SHADER | PIXEL_SHADER);

    scene.device.UnsetShaderResource(0, PIXEL_SHADER);
    
    ShaderKey psKey;
    psKey.filepath = "DeferredShading.ps.hlsl";
    psKey.defines = {
        { "ENABLE_IBL", doIBLPass ? "1" : "0"},
        { "DIRECTIONAL_CASCADED_SHADOWS", settings.dirShadows ? "1" : "0"},
        { "POINT_SHADOWS", settings.pointShadows ? "1" : "0"},
        { "SPOT_SHADOWS", settings.spotShadows ? "1" : "0"},
        { "PCF", settings.shadowPCF ? "1" : "0"},
        { "ENABLE_VOXEL_GI", 
#if VXGI_ENABLED
        sceneSettings.graphics.enableVoxelGI && illuminationVolume ? "1" : "0"
#else
        "0"
#endif
        },
        { "DIFFUSE_IRRADIANCE", !settings.useDiffuseIrradianceSH ? "DIFFUSE_IRRADIANCE_MAP" : "DIFFUSE_IRRADIANCE_SH" },
        { "SUN_TRANSMITTANCE", useSunTransmittanceForDirectional ? "1" : "0" },
    };
    PixelShader* pixelShader = ResourceManager::Get().shaderLoader.Load<PixelShader>(std::move(psKey), scene.device);

    scene.device.SetFrameBuffer(scene.sceneColorBuffer);
    
    if (settings.dirShadows)
    {
        if (shadowsData.directionalShadowMap)
            scene.device.SetShaderResource(pixelShader->GetTextureSlot("DirectionalCascadedShadowMap"), *shadowsData.directionalShadowMap, PIXEL_SHADER);
        if (shadowsData.shadowMapCascades)
            scene.device.SetConstantBuffers(pixelShader->GetConstantBufferSlot("ShadowCascades"), ViewSpan{ shadowsData.shadowMapCascades }, PIXEL_SHADER);
    }

    if (settings.pointShadows)
    {
        if (shadowsData.pointShadowMaps)
            scene.device.SetShaderResource(pixelShader->GetTextureSlot("PointShadowMaps"), *shadowsData.pointShadowMaps, PIXEL_SHADER);
        if (shadowsData.pointLightData)
            scene.device.SetShaderResource(pixelShader->GetStructuredBufferSlot("PointLightDataBuffer"), *shadowsData.pointLightData, PIXEL_SHADER);
    }

    if (settings.spotShadows)
    {
        if (shadowsData.spotShadowMaps)
            scene.device.SetShaderResource(pixelShader->GetTextureSlot("SpotShadowMaps"), *shadowsData.spotShadowMaps, PIXEL_SHADER);
        if (shadowsData.spotLightViewMatrices)
            scene.device.SetShaderResource(pixelShader->GetStructuredBufferSlot("SpotLightMatrices"), *shadowsData.spotLightViewMatrices, PIXEL_SHADER);
    }
    
    scene.device.SetShaderResource(pixelShader->GetTextureSlot("DepthTex"), scene.gBuffer.depthStencil, PIXEL_SHADER);
    auto resources = scene.gBuffer.renderTextures
        | std::views::transform([](Texture& tex) -> IReadableResource* { return &tex; })
        | std::ranges::to<std::vector<IReadableResource*>>();
    scene.device.SetShaderResources(pixelShader->GetTextureSlot("DiffuseTexture"), { resources.begin(), resources.end() }, PIXEL_SHADER);

#if VXGI_ENABLED
    if (sceneSettings.graphics.enableVoxelGI && illuminationVolume)
    {
        voxelGIConfigBuffer.Update(scene.device, giConfig);
        scene.device.SetConstantBuffers(pixelShader->GetConstantBufferSlot("VoxelGIConfig"), ViewSpan{ &voxelGIConfigBuffer }, PIXEL_SHADER);
        scene.device.SetShaderResource(pixelShader->GetTextureSlot("VoxelIlluminationVolume"), *illuminationVolume, PIXEL_SHADER);
    }
#endif // VXGI_ENABLED

    if (doIBLPass)
    {
        // Diffuse Irradiance IBL
        if (!settings.useDiffuseIrradianceSH && iblData.diffuseIrradianceMap)
            scene.device.SetShaderResource(pixelShader->GetTextureSlot("DiffuseIrradianceMap"), *iblData.diffuseIrradianceMap, PIXEL_SHADER);
        else if (iblData.diffuseIrradianceSH9PackedConstantBuffer.IsSet())
            scene.device.SetConstantBuffers(pixelShader->GetConstantBufferSlot("DiffuseIrradianceSH9PackedConstantBuffer"), ViewSpan{ &iblData.diffuseIrradianceSH9PackedConstantBuffer }, PIXEL_SHADER);    
        
        // Specular Reflection IBL
        if (iblData.prefilteredEnvironmentMap)
            scene.device.SetShaderResource(pixelShader->GetTextureSlot("PrefilteredEnvironmentMap"), *iblData.prefilteredEnvironmentMap, PIXEL_SHADER);
        if (iblData.brdfIntegrationLUT)
            scene.device.SetShaderResource(pixelShader->GetTextureSlot("BRDFIntegrationLUT"), *iblData.brdfIntegrationLUT, PIXEL_SHADER);
    }
    
    if (useSunTransmittanceForDirectional)
    { 
        scene.device.SetShaderResource(pixelShader->GetTextureSlot("TransmittanceLUT"), atmospherePass.GetTransmittanceLUT(), PIXEL_SHADER);
    }

    Graphics::DrawFullscreenTriangle(scene.device, *pixelShader);

    // Unbind gbuffer textures once done with shading
    for (int i = pixelShader->GetTextureSlot("DiffuseTexture"); i < pixelShader->GetTextureSlot("DiffuseTexture") + scene.gBuffer.renderTextures.size(); ++i)
    {
        scene.device.UnsetShaderResource(i, PIXEL_SHADER);
    }
    scene.device.UnsetShaderResource(pixelShader->GetTextureSlot("DepthTex"), PIXEL_SHADER);

#if VXGI_ENABLED
    if (sceneSettings.graphics.enableVoxelGI && illuminationVolume)
    {
        scene.device.UnsetShaderResource(pixelShader->GetTextureSlot("VoxelIlluminationVolume"), PIXEL_SHADER);
    }
#endif // VXGI_ENABLED

    if (settings.dirShadows)
    {
        if (shadowsData.directionalShadowMap)
        {
            scene.device.UnsetShaderResource(pixelShader->GetTextureSlot("DirectionalCascadedShadowMap"), PIXEL_SHADER);
        }
    }
}

void DeferredLightingPass::RegenerateIBL(Device& device, const Texture* skybox, const AtmospherePass& atmospherePass, const SceneSettings& sceneSettings)
{
    if (!sceneSettings.graphics.enableAtmosphere && skybox)
    {
        Texture mippedSkybox = skybox->CreateCopy(device, true);
        iblData.RecreateDiffuseIrradiance(device, mippedSkybox, *skybox);
        iblData.RecreateEnvironmentMap(device, mippedSkybox);
    }
    else if (sceneSettings.graphics.enableAtmosphere)
    {
        DirectionalLightData lightData;
        lightData.direction = sceneSettings.graphics.directionalLight.direction;
        lightData.color = sceneSettings.graphics.directionalLight.color;
        lightData.intensity = sceneSettings.graphics.directionalLight.intensity;
        
        Texture atmosphereCubemap = CubemapHelpers::CreateCubemapFromAtmosphere(device,
            lightData,
            atmospherePass.GetSettingsBuffer(),
            atmospherePass.GetTransmittanceLUT(),
            atmospherePass.GetMultipleScatteringLUT());
        
        iblData.RecreateDiffuseIrradiance(device, atmosphereCubemap, atmosphereCubemap);
        iblData.RecreateEnvironmentMap(device, atmosphereCubemap);
    }
}

void DeferredLightingPass::RegenerateIBL(GraphicsScene& scene, const Texture* skybox, const AtmospherePass& atmospherePass, const SceneSettings& sceneSettings)
{
    RegenerateIBL(scene.device, skybox, atmospherePass, sceneSettings);
    scene.invalidateIBL = false;
}

void IBLData::RecreateDiffuseIrradiance(Device& device, const Texture& mippedSkybox, const Texture& skybox)
{
    diffuseIrradianceMap = CubemapHelpers::CreateDiffuseIrradianceMap(mippedSkybox, device);
    
    // We compute this on CPU to simplify code (otherwise we could do this in a compute shader via a divide and conquer strategy)
    // We only need the first mip so we can pass the cubemap without mips (we have to create a copy to readback on CPU)
    if (!useComputeForIrradianceData)
        diffuseIrradianceSH9 = TextureHelpers::CreateDiffuseIrradianceSH(skybox, device);
    else
        diffuseIrradianceSH9 = TextureHelpers::CreateDiffuseIrradianceSH_Compute(skybox, device);
    
    std::array<Vector4, 7> diffuseIrradianceSH9Packed;
    std::memcpy(&diffuseIrradianceSH9Packed[0].x, &diffuseIrradianceSH9.r[0], 3 * 9 * sizeof(float));
    // Setting unused variable to 0
    diffuseIrradianceSH9Packed[6].w = 0;
    diffuseIrradianceSH9PackedConstantBuffer.Update(device, diffuseIrradianceSH9Packed);
}

void IBLData::RecreateEnvironmentMap(Device& device, const Texture& mippedSkybox)
{
    prefilteredEnvironmentMap = CubemapHelpers::CreatePrefilteredEnvironmentMap(mippedSkybox, device);
}

void IBLData::RecreateBRDFLUT(Device& device)
{
    brdfIntegrationLUT = TextureHelpers::CreateBRDFIntegrationLUT(device);
}

#ifdef IMGUI_ENABLED

import ImGui;

bool DeferredLightingPass::ImGuiDraw()
{
    bool result = false;
    result |= ImGui::Checkbox("Activate IBL", &settings.enableIBL);
    result |= ImGui::Checkbox("Use diffuse irradiance SH", &settings.useDiffuseIrradianceSH);
    ImGui::Separator();
#if VXGI_ENABLED 
    result |= ImGui::InputFloat("Indirect Lighting Boost", &giConfig.indirectBoost);
    ImGui::Separator();
#endif
    result |= ImGui::Checkbox("Directional Shadows", &settings.dirShadows);
    result |= ImGui::Checkbox("Point Shadows", &settings.pointShadows);
    result |= ImGui::Checkbox("Spot Shadows", &settings.spotShadows);
    result |= ImGui::Checkbox("Shadows PCF", &settings.shadowPCF);

    result |= ImGui::Checkbox("Use Compute Shader for Diffuse IBL", &iblData.useComputeForIrradianceData);

    return result;
}

void IBLData::RecreateDiffuseIrradiance(Device& device, const Skybox& skybox)
{
    Texture mippedSkybox = skybox.texture.CreateCopy(device, true);
    RecreateDiffuseIrradiance(device, mippedSkybox, skybox.texture);
}

void IBLData::RecreateEnvironmentMap(Device& device, const Skybox& skybox)
{
    Texture mippedSkybox = skybox.texture.CreateCopy(device, true);
    RecreateEnvironmentMap(device, mippedSkybox);
}


#endif
