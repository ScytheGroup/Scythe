module;
#include "../Utils/EventScopeMacros.h"
module Graphics:RenderPass;

import :RenderPass;
import :Resources;
import :GraphicsDefs;
import :Scene;
import :Shader;
import :ComputeShaderHelper;
import Graphics;

import Common;
import Core;

void AtmospherePass::Init(Device& device, SceneSettings& sceneSettings)
{
    atmosphereSettings.densityScaleHeightRayleigh = sceneSettings.graphics.atmosphereSettings.rayleighScatteringDensity;
    atmosphereSettings.densityScaleHeightMie = sceneSettings.graphics.atmosphereSettings.mieScatteringDensity;
    groundAlbedo = sceneSettings.graphics.atmosphereSettings.groundAlbedo;
    atmosphereSettings.atmosphereIntensity = sceneSettings.graphics.atmosphereSettings.atmosphereIntensity;

    directionalLightSettings.color = sceneSettings.graphics.directionalLight.color;
    directionalLightSettings.intensity = sceneSettings.graphics.directionalLight.intensity;
    directionalLightSettings.direction = sceneSettings.graphics.directionalLight.direction;

    atmosphereSettingsCBuffer.Update(device, atmosphereSettings);
    GenerateTransmittanceLUT(device);
    GenerateMultipleScatteringLUT(device);
}

void AtmospherePass::Render(GraphicsScene& scene, SceneSettings& sceneSettings)
{
    SC_SCOPED_GPU_EVENT("Atmosphere");

    if (!sceneSettings.graphics.enableAtmosphere)
    {
        return;
    }
    
    // Make sure settings are up to date, regenerate the LUT if they aren't.
    {
        bool directionalLightHasChanged = 
            directionalLightSettings.color != sceneSettings.graphics.directionalLight.color ||
            directionalLightSettings.intensity != sceneSettings.graphics.directionalLight.intensity ||
            directionalLightSettings.direction != sceneSettings.graphics.directionalLight.direction;

        bool updateLUT = false;

        if (sceneSettings.graphics.atmosphereSettings.rayleighScatteringDensity != atmosphereSettings.densityScaleHeightRayleigh)
        {
            atmosphereSettings.densityScaleHeightRayleigh = sceneSettings.graphics.atmosphereSettings.rayleighScatteringDensity;
            updateLUT |= true;
        }
    
        if (sceneSettings.graphics.atmosphereSettings.mieScatteringDensity != atmosphereSettings.densityScaleHeightMie)
        {
            atmosphereSettings.densityScaleHeightMie = sceneSettings.graphics.atmosphereSettings.mieScatteringDensity;
            updateLUT |= true;
        }

        if (sceneSettings.graphics.atmosphereSettings.groundAlbedo != groundAlbedo)
        {
            groundAlbedo = sceneSettings.graphics.atmosphereSettings.groundAlbedo;
            updateLUT |= true;
        }

        if (sceneSettings.graphics.atmosphereSettings.atmosphereIntensity != atmosphereSettings.atmosphereIntensity)
        {
            atmosphereSettings.atmosphereIntensity = sceneSettings.graphics.atmosphereSettings.atmosphereIntensity;
            updateLUT |= true;
        }

        if (updateLUT || directionalLightHasChanged)
        {
            Init(scene.device, sceneSettings);
            scene.invalidateIBL = true;
        }
    }

    if (atmosphereSettings.useStaticSkyView)
    {
        GenerateSkyLUT(scene.device, scene.frameData.cameraPosition, scene.frameData.directionalLight.direction);
    }

    {
        SC_SCOPED_GPU_EVENT("DrawAtmosphere");

        ShaderKey csKey;
        csKey.filepath = "Atmosphere.cs.hlsl";
        csKey.defines = {{"USE_SKY_VIEW_LUT", atmosphereSettings.useStaticSkyView ? "1" : "0"}};
        
        ComputeShader cs = *scene.resourceManager.shaderLoader.Load<ComputeShader>(std::move(csKey), scene.device);
        scene.device.SetComputeShader(cs);

        scene.device.UnsetRenderTargets();
        scene.device.UnsetDepthStencilState();
        scene.device.SetConstantBuffers(cs.GetConstantBufferSlot("AtmosphereSettings"), ViewSpan{&atmosphereSettingsCBuffer}, COMPUTE_SHADER);
        scene.device.SetShaderResource(cs.GetTextureSlot("Depth"), scene.gBuffer.depthStencil, COMPUTE_SHADER);
        scene.device.SetShaderResource(cs.GetTextureSlot("TransmittanceLUT"), *transmittanceLUT, COMPUTE_SHADER);
        if (atmosphereSettings.useStaticSkyView)
        {
            scene.device.SetShaderResource(cs.GetTextureSlot("SkyLUT"), *skyLUT, COMPUTE_SHADER);
        }
        else
        {
            scene.device.SetShaderResource(cs.GetTextureSlot("MultipleScatteringLUT"), *multipleScatteringLUT, COMPUTE_SHADER);
        }
        scene.device.SetUnorderedAccessView(cs.GetTextureSlot("SceneColor"), scene.sceneColorBuffer.renderTextures[0]);
        
        auto [x, y, z] = GetThreadCountsForComputeSize(scene.sceneColorBuffer.GetSize(), UIntVector2{8, 8});
        scene.device.Dispatch(x, y, z);

        scene.device.UnsetShaderResource(cs.GetTextureSlot("Depth"), COMPUTE_SHADER);
        scene.device.UnsetShaderResource(cs.GetTextureSlot("TransmittanceLUT"), COMPUTE_SHADER);
        if (atmosphereSettings.useStaticSkyView)
        {
            scene.device.UnsetShaderResource(cs.GetTextureSlot("SkyLUT"), COMPUTE_SHADER);
        }
        else
        {
            scene.device.UnsetShaderResource(cs.GetTextureSlot("MultipleScatteringLUT"), COMPUTE_SHADER);
        }
        scene.device.UnsetUnorderedAccessView(cs.GetTextureSlot("SceneColor"));
    }

    directionalLightSettings.color = sceneSettings.graphics.directionalLight.color;
    directionalLightSettings.intensity = sceneSettings.graphics.directionalLight.intensity;
    directionalLightSettings.direction = sceneSettings.graphics.directionalLight.direction;
}

void AtmospherePass::GenerateTransmittanceLUT(Device& device)
{
    SC_SCOPED_GPU_EVENT_FUNC();
    
    Texture::Description data = Texture::Description::CreateTexture2D("TransmittanceLUT", TransmittanceLUTSize.x, TransmittanceLUTSize.y, DXGI_FORMAT_R32G32B32A32_FLOAT, Texture::BindFlags::TEXTURE_SRV | Texture::BindFlags::TEXTURE_UAV, false);
    transmittanceLUT.emplace(data, device);

    ConstantBuffer<UIntVector2> textureSizeConstantBuffer = ConstantBuffer<UIntVector2>(device, TransmittanceLUTSize);
    
    ShaderKey csKey;
    csKey.filepath = "AtmosphereTransmittanceLUT.cs.hlsl";
    ComputeShader cs = *ResourceManager::Get().shaderLoader.Load<ComputeShader>(std::move(csKey), device);
    device.SetComputeShader(cs);

    device.SetConstantBuffers(cs.GetConstantBufferSlot("AtmosphereSettings"), ViewSpan{&atmosphereSettingsCBuffer}, COMPUTE_SHADER);
    device.SetConstantBuffers(cs.GetConstantBufferSlot("TextureSize"), ViewSpan{&textureSizeConstantBuffer}, COMPUTE_SHADER);
    device.SetUnorderedAccessView(cs.GetTextureSlot("TransmittanceLUT"), *transmittanceLUT);
    
    auto [x, y, z] = GetThreadCountsForComputeSize(UIntVector2{transmittanceLUT->GetWidth(), transmittanceLUT->GetHeight()}, UIntVector2{8, 8});
    device.Dispatch(x, y, z);

    device.UnsetUnorderedAccessView(cs.GetTextureSlot("TransmittanceLUT"));
}

void AtmospherePass::GenerateMultipleScatteringLUT(Device& device)
{
    SC_SCOPED_GPU_EVENT_FUNC();
    
    Texture::Description data = Texture::Description::CreateTexture2D("MultipleScatteringLUT", MultipleScatteringLUTSize.x, MultipleScatteringLUTSize.y, DXGI_FORMAT_R32G32B32A32_FLOAT, Texture::BindFlags::TEXTURE_SRV | Texture::BindFlags::TEXTURE_UAV, false);
    multipleScatteringLUT.emplace(data, device);

    struct MSLUTParams // MS LUT, not M-SLUT
    {
        UIntVector2 textureSize;
        Vector3 groundAlbedo alignas(16);
    };
    ConstantBuffer<MSLUTParams> textureSizeConstantBuffer = ConstantBuffer<MSLUTParams>(device, {MultipleScatteringLUTSize, groundAlbedo});

    ShaderKey csKey;
    csKey.filepath = "AtmosphereMultipleScatteringLUT.cs.hlsl";
    ComputeShader cs = *ResourceManager::Get().shaderLoader.Load<ComputeShader>(std::move(csKey), device);
    device.SetComputeShader(cs);

    device.SetConstantBuffers(cs.GetConstantBufferSlot("AtmosphereSettings"), ViewSpan{&atmosphereSettingsCBuffer}, COMPUTE_SHADER);
    device.SetConstantBuffers(cs.GetConstantBufferSlot("TextureSize"), ViewSpan{&textureSizeConstantBuffer}, COMPUTE_SHADER);
    device.SetShaderResource(cs.GetTextureSlot("TransmittanceLUT"), *transmittanceLUT, COMPUTE_SHADER);
    device.SetUnorderedAccessView(cs.GetTextureSlot("MultipleScatteringLUT"), *multipleScatteringLUT);

    auto [x, y, z] = GetThreadCountsForComputeSize(UIntVector2{multipleScatteringLUT->GetWidth(), multipleScatteringLUT->GetHeight()}, UIntVector2{8, 8});
    device.Dispatch(x, y, z);

    device.UnsetShaderResource(cs.GetTextureSlot("TransmittanceLUT"), COMPUTE_SHADER);
    device.UnsetUnorderedAccessView(cs.GetTextureSlot("MultipleScatteringLUT"));
}

void AtmospherePass::GenerateSkyLUT(Device& device, Vector3 cameraPosition, Vector3 sunDirection)
{
    SC_SCOPED_GPU_EVENT_FUNC();

    Texture::Description data = Texture::Description::CreateTexture2D("SkyLUT", SkyLUTSize.x, SkyLUTSize.y, DXGI_FORMAT_R32G32B32A32_FLOAT, Texture::BindFlags::TEXTURE_SRV | Texture::BindFlags::TEXTURE_UAV, false);
    skyLUT.emplace(data, device);

    struct SkyLUTParams
    {
        Vector3 cameraPosition;
        uint32_t skyLUTSizeX;
        Vector3 sunDirection;
        uint32_t skyLUTSizeY;
    };
    ConstantBuffer<SkyLUTParams> textureSizeConstantBuffer = ConstantBuffer<SkyLUTParams>(device, {cameraPosition, SkyLUTSize.x, sunDirection, SkyLUTSize.y});
    
    ShaderKey csKey;
    csKey.filepath = "AtmosphereSkyLUT.cs.hlsl";
    ComputeShader cs = *ResourceManager::Get().shaderLoader.Load<ComputeShader>(std::move(csKey), device);
    device.SetComputeShader(cs);

    device.SetConstantBuffers(cs.GetConstantBufferSlot("AtmosphereSettings"), ViewSpan{&atmosphereSettingsCBuffer}, COMPUTE_SHADER);
    device.SetConstantBuffers(cs.GetConstantBufferSlot("SkyLUTParams"), ViewSpan{&textureSizeConstantBuffer}, COMPUTE_SHADER);
    device.SetShaderResource(cs.GetTextureSlot("TransmittanceLUT"), *transmittanceLUT, COMPUTE_SHADER);
    device.SetShaderResource(cs.GetTextureSlot("MultipleScatteringLUT"), *multipleScatteringLUT, COMPUTE_SHADER);
    device.SetUnorderedAccessView(cs.GetTextureSlot("SkyLUT"), *skyLUT);
    
    auto [x, y, z] = GetThreadCountsForComputeSize(UIntVector2{skyLUT->GetWidth(), skyLUT->GetHeight()}, UIntVector2{8, 8});
    device.Dispatch(x, y, z);

    device.UnsetShaderResource(cs.GetTextureSlot("TransmittanceLUT"), COMPUTE_SHADER);
    device.UnsetShaderResource(cs.GetTextureSlot("MultipleScatteringLUT"), COMPUTE_SHADER);
    device.UnsetUnorderedAccessView(cs.GetTextureSlot("SkyLUT"));
}

#ifdef IMGUI_ENABLED

import ImGui;

void AtmospherePass::ImGuiDraw(Device& device)
{
    ImGui::Text("WARNING: Settings exposed here but not in the SceneSettings should be viewed as experimental and not usable in an actual game.");
    
    ImGui::DragFloat("Planet Radius", &atmosphereSettings.planetRadius, 1, 0, 1000000);
    ImGui::DragFloat("Atmosphere Height", &atmosphereSettings.atmosphereHeight, 1, 0, 1000000);
    ImGui::Text("Rayleigh Density Scale: %f", atmosphereSettings.densityScaleHeightRayleigh);
    ImGui::Text("Mie Density Scale: %f", atmosphereSettings.densityScaleHeightMie);
    ImGui::DragFloat3("Planet Center", &atmosphereSettings.planetCenter.x);

    ImGui::Checkbox("Use Static Sky View", &atmosphereSettings.useStaticSkyView);

    if (ImGui::Button("Reset Changes"))
    {
        atmosphereSettings = {};
    }
}

#endif