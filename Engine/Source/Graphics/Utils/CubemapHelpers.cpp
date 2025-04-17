module Graphics:CubemapHelpers;

import Graphics;
import Components;
import Systems;
import Core;

import :CubemapHelpers;
import :ResourceManager;
import :RasterizerState;

static const std::array CubemapCaptureFacesDirections
{
    Vector3::UnitX,  // pos X
    -Vector3::UnitX, // neg X
    -Vector3::UnitY,  // pos Y
    Vector3::UnitY, // neg Y
    Vector3::UnitZ,  // pos Z
    -Vector3::UnitZ, // neg Z
};

static const std::array CubemapCaptureUpVectors
{
    Vector3::Down,   // pos X
    Vector3::Down,   // neg X
    -Vector3::UnitZ, // pos Y
    Vector3::UnitZ, // neg Y
    Vector3::Down,   // pos Z
    Vector3::Down,   // neg Z
};

DrawCubemapInfo CubemapHelpers::SetupDrawCubemap(ShadingModel& shadingModel, uint32_t cubemapResolutionSize, Device& device)
{
    DrawCubemapInfo cubemapDrawInfo
    {
        MeshPresets::GetCubeGeometry(),
        DepthStencil{ device, cubemapResolutionSize, cubemapResolutionSize },
        {}
    };

    cubemapDrawInfo.viewport.Width = static_cast<FLOAT>(cubemapResolutionSize);
    cubemapDrawInfo.viewport.Height = static_cast<FLOAT>(cubemapResolutionSize);
    cubemapDrawInfo.viewport.MinDepth = 0.0f;
    cubemapDrawInfo.viewport.MaxDepth = 1.0f;
    cubemapDrawInfo.viewport.TopLeftX = 0;
    cubemapDrawInfo.viewport.TopLeftY = 0;

    cubemapDrawInfo.geometry->LoadResource(device);

    device.SetShadingModel(shadingModel);
    device.SetRasterizerState(RasterizerStates::Get<RasterizerPresets::FrontfaceCulling>(device));

    return cubemapDrawInfo;
}

void CubemapHelpers::ExecuteDrawCubemap(Texture& renderTexture, uint32_t mip, uint32_t face, DrawCubemapInfo& drawCubemapInfo, Device& device)
{
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
    rtvDesc.Texture2DArray.ArraySize = 1;
    rtvDesc.Texture2DArray.FirstArraySlice = face;
    rtvDesc.Texture2DArray.MipSlice = mip;
    rtvDesc.Format = renderTexture.GetFormat();
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
    ComPtr<ID3D11RenderTargetView> rtv = device.CreateRTV(renderTexture.GetRawTexture(), rtvDesc);

    device.SetViewports({ drawCubemapInfo.viewport });
    device.SetRenderTargetsAndDepthStencilView({ rtv }, drawCubemapInfo.depthStencil.GetDSV());

    device.ClearRTV(Color{ 0, 0, 1, 1 }, rtv);
    device.ClearDepthStencilView(drawCubemapInfo.depthStencil.GetDSV());

    Graphics::DrawGeometry(device, {}, *drawCubemapInfo.geometry);
}

void CubemapHelpers::CleanupDrawCubemap(Device& device)
{
    device.SetRasterizerState(RasterizerStates::Get<RasterizerPresets::BackfaceCulling>(device));
    device.UnsetRenderTargets();
    device.UnsetDepthStencilState();
}

Texture CubemapHelpers::ConvertEquirectangularTextureToCubeMap(const Texture& equirectangularTexture, Device& device)
{
    const uint32_t cubemapResolutionSize = RoundUpToNextPow2(equirectangularTexture.GetWidth() / 4);
    CameraProjection projection(CameraProjection::ProjectionType::PERSPECTIVE, 90.0f, 1.0f, 0.1f, 2.0f);

    Texture::Description data = Texture::Description::CreateTextureCube(std::format("Cubemap: {}", equirectangularTexture.GetName()), cubemapResolutionSize, DXGI_FORMAT_R32G32B32A32_FLOAT, false);
    Texture renderTexture(data, device);

    ShadingModel shadingModel(ResourceManager::Get().shaderLoader, ShaderKey{ "Skybox.vs.hlsl" }, ShaderKey{ "Skybox.ps.hlsl", { { "EQUIRECTANGULAR_CUBEMAP", "1" }, { "NO_NORMAL_WRITE", "1" } } }, device);
    
    DrawCubemapInfo drawInfo = SetupDrawCubemap(shadingModel, cubemapResolutionSize, device);

    FrameData frameData;
    ConstantBuffer<FrameData> frameDataBuffer;

    for (int i = 0; i < 6; ++i)
    {
        frameData.UpdateView(Transform::CreateLookAt(Vector3::Zero, CubemapCaptureFacesDirections[i], CubemapCaptureUpVectors[i]), projection);
        frameData.screenSize = Vector2(drawInfo.viewport.Width, drawInfo.viewport.Height);
        frameDataBuffer.Update(device, frameData);
        device.SetConstantBuffers(Graphics::FrameCBufferIndex, ViewSpan{ &frameDataBuffer }, VERTEX_SHADER | PIXEL_SHADER);

        device.SetShaderResource(0, equirectangularTexture, PIXEL_SHADER);

        ExecuteDrawCubemap(renderTexture, 0, i, drawInfo, device);
    }

    CleanupDrawCubemap(device);

    return renderTexture;
}

Texture CubemapHelpers::CreateDiffuseIrradianceMap(const Texture& environmentMap, Device& device)
{
    static constexpr uint32_t Resolution = 32;
    CameraProjection projection(CameraProjection::ProjectionType::PERSPECTIVE, 90.0f, 1.0f, 0.1f, 2.0f);

    uint32_t numMipsDiffuseIrradianceTexture = static_cast<uint32_t>(std::floor(std::log2(Resolution)) + 1);
    uint32_t mipOffset = environmentMap.GetMipsCount() - numMipsDiffuseIrradianceTexture;

    Texture::Description data = Texture::Description::CreateTextureCube(std::format("Diffuse Irradiance Map: {}", environmentMap.GetName()), Resolution, DXGI_FORMAT_R32G32B32A32_FLOAT, false);
    Texture renderTexture(data, device);
    ShadingModel shadingModel(ResourceManager::Get().shaderLoader, ShaderKey{ "Skybox.vs.hlsl" }, ShaderKey{ "DiffuseIrradianceMap.ps.hlsl" }, device);
    ConstantBuffer mipOffsetBuffer(device, mipOffset);
    
    DrawCubemapInfo drawInfo = SetupDrawCubemap(shadingModel, Resolution, device);

    FrameData frameData;
    ConstantBuffer<FrameData> frameDataBuffer;

    for (int i = 0; i < 6; ++i)
    {
        frameData.UpdateView(Transform::CreateLookAt(Vector3::Zero, CubemapCaptureFacesDirections[i], CubemapCaptureUpVectors[i]), projection);
        frameData.screenSize = Vector2(drawInfo.viewport.Width, drawInfo.viewport.Height);
        frameDataBuffer.Update(device, frameData);
        device.SetConstantBuffers(Graphics::FrameCBufferIndex, ViewSpan{ &frameDataBuffer }, VERTEX_SHADER | PIXEL_SHADER);
        device.SetConstantBuffers(shadingModel.pixelShader->GetConstantBufferSlot("MipOffset"), ViewSpan{ &mipOffsetBuffer }, PIXEL_SHADER);
        device.SetShaderResource(0, environmentMap, PIXEL_SHADER);

        ExecuteDrawCubemap(renderTexture, 0, i, drawInfo, device);
    }
    
    CleanupDrawCubemap(device);

    return renderTexture;
}

Matrix CubemapHelpers::GetViewMatrixForFace(int faceIndex, const Vector3& position)
{
    return Matrix::CreateLookAt(position, position + CubemapCaptureFacesDirections[faceIndex], CubemapCaptureUpVectors[faceIndex]);
}

Texture CubemapHelpers::CreatePrefilteredEnvironmentMap(const Texture& environmentMap, Device& device)
{ 
    // Could be converted to dynamic size when/if we add local probes
    static constexpr uint32_t Resolution = 512;

    Texture::Description data = Texture::Description::CreateTextureCube(std::format("Prefiltered Environment Map: {}", environmentMap.GetName()), Resolution, DXGI_FORMAT_R32G32B32A32_FLOAT, true);
    Texture renderTexture(data, device);

    ShadingModel shadingModel(ResourceManager::Get().shaderLoader, ShaderKey{ "Skybox.vs.hlsl" }, ShaderKey{ "PrefilteredEnvironmentMap.ps.hlsl" }, device);

    struct FilterData
    {
        uint32_t mipLevel;
        uint32_t mipOffset;
    };

    ConstantBuffer<FilterData> filterDataBuffer;
    // Could be dynamic depending on resolution (computed from size), currently hard-coded as 5
    static constexpr uint32_t MaxMipLevels = 5;
    uint32_t mipOffset = std::max<uint32_t>(environmentMap.GetMipsCount() - renderTexture.GetMipsCount(), 0);

    CameraProjection projection(CameraProjection::ProjectionType::PERSPECTIVE, 90.0f, 1.0f, 0.1f, 2.0f);
    FrameData frameData;
    ConstantBuffer<FrameData> frameDataBuffer;

    DrawCubemapInfo drawInfo = SetupDrawCubemap(shadingModel, Resolution, device);

    for (uint32_t mip = 0; mip < MaxMipLevels; ++mip)
    {
        filterDataBuffer.Update(device, { .mipLevel = mip, .mipOffset = mipOffset });

        uint32_t mipSize = 1 << (renderTexture.GetMipsCount() - 1 - mip);

        drawInfo.depthStencil = { device, mipSize, mipSize };
        drawInfo.viewport.Width = static_cast<FLOAT>(mipSize);
        drawInfo.viewport.Height = static_cast<FLOAT>(mipSize);
        frameData.screenSize = Vector2(drawInfo.viewport.Width, drawInfo.viewport.Height);

        for (int i = 0; i < 6; ++i)
        {
            frameData.UpdateView(Transform::CreateLookAt(Vector3::Zero, CubemapCaptureFacesDirections[i], CubemapCaptureUpVectors[i]), projection);
            frameDataBuffer.Update(device, frameData);
            device.SetConstantBuffers(Graphics::FrameCBufferIndex, ViewSpan{ &frameDataBuffer }, VERTEX_SHADER | PIXEL_SHADER);

            device.SetConstantBuffers(shadingModel.pixelShader->GetConstantBufferSlot("FilterData"), ViewSpan{ &filterDataBuffer }, PIXEL_SHADER);
            device.SetShaderResource(0, environmentMap, PIXEL_SHADER);

            ExecuteDrawCubemap(renderTexture, mip, i, drawInfo, device);
        }
    }

    CleanupDrawCubemap(device);

    return renderTexture; 
}

Vector3 CubemapHelpers::ConvertCubemapFaceUVToVec3(const int face, Vector2 uv)
{
    Vector3 xyz;
    // convert range 0 to 1 to -1 to 1
    uv = uv * 2 - Vector2::One;
    switch (face)
    {
    case 0:
        xyz = Vector3(1.0f, -uv.y, -uv.x);
        break; // POSITIVE X
    case 1:
        xyz = Vector3(-1.0f, -uv.y, uv.x);
        break; // NEGATIVE X
    case 2:
        xyz = Vector3(uv.x, 1.0f, uv.y);
        break; // POSITIVE Y
    case 3:
        xyz = Vector3(uv.x, -1.0f, -uv.y);
        break; // NEGATIVE Y
    case 4:
        xyz = Vector3(uv.x, -uv.y, 1.0f);
        break; // POSITIVE Z
    case 5:
        xyz = Vector3(uv.x, -uv.y, -1.0f);
        break; // NEGATIVE Z
    default:
        break;
    }
    return xyz;
}

Texture CubemapHelpers::CreateCubemapFromAtmosphere(
    Device& device,
    const DirectionalLightData& directionalLight,
    const ConstantBuffer<AtmosphereSettings>& atmosphereSettingsCBuffer,
    const Texture& transmittanceLUT,
    const Texture& multipleScatteringLUT
) {
    static constexpr uint32_t Resolution = 512;

    Texture::Description data = Texture::Description::CreateTextureCube("Atmosphere Cubemap", Resolution, DXGI_FORMAT_R32G32B32A32_FLOAT, true);
    Texture atmosphereCubemap(data, device);

    CameraProjection projection(CameraProjection::ProjectionType::PERSPECTIVE, 90.0f, 1.0f, 0.1f, 2.0f);

    // We want to sample the atmosphere from ground level (y = 0) for consistent results
    Vector3 viewPos(0, 0.001f, 0); // Slightly above ground to avoid artifacts

    ShadingModel sm = ShadingModel(ResourceManager::Get().shaderLoader, ShaderKey{ "Skybox.vs.hlsl" }, ShaderKey{ "AtmosphereToCubemap.ps.hlsl" }, device);

    DrawCubemapInfo drawInfo = SetupDrawCubemap(sm, Resolution, device);

    FrameData frameData;
    ConstantBuffer<FrameData> frameDataBuffer;

    // Setup frame data that will be consistent across faces
    frameData.directionalLight = directionalLight;
    frameData.cameraPosition = viewPos;

    for (int i = 0; i < 6; ++i)
    {
        frameData.UpdateView(Transform::CreateLookAt(viewPos, viewPos + CubemapCaptureFacesDirections[i], CubemapCaptureUpVectors[i]), projection);
        frameData.screenSize = Vector2(drawInfo.viewport.Width, drawInfo.viewport.Height);
        frameDataBuffer.Update(device, frameData);

        device.SetConstantBuffers(Graphics::FrameCBufferIndex, ViewSpan{ &frameDataBuffer }, VERTEX_SHADER | PIXEL_SHADER);
        device.SetConstantBuffers(sm.pixelShader->GetConstantBufferSlot("AtmosphereSettings"), ViewSpan{ &atmosphereSettingsCBuffer }, PIXEL_SHADER);
        
        device.SetShaderResource(sm.pixelShader->GetTextureSlot("TransmittanceLUT"), transmittanceLUT, PIXEL_SHADER);
        device.SetShaderResource(sm.pixelShader->GetTextureSlot("MultipleScatteringLUT"), multipleScatteringLUT, PIXEL_SHADER);

        ExecuteDrawCubemap(atmosphereCubemap, 0, i, drawInfo, device);

        device.UnsetShaderResource(sm.pixelShader->GetTextureSlot("TransmittanceLUT"), PIXEL_SHADER);
        device.UnsetShaderResource(sm.pixelShader->GetTextureSlot("MultipleScatteringLUT"), PIXEL_SHADER);
    }

    CleanupDrawCubemap(device);

    // Generate mips for specular IBL to be able to blur.
    //device.GenerateMips(atmosphereCubemap.GetSRV());

    return atmosphereCubemap;
}
