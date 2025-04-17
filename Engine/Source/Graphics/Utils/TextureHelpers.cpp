module;
#include "EventScopeMacros.h"
#include "../DirectXTex.h"
module Graphics:TextureHelpers;

import :TextureHelpers;
import :BlendState;
import :Shader;
import :ShadingModel;
import :ResourceManager;
import :ComputeShaderHelper;
import :CubemapHelpers;

#ifdef SC_DEV_VERSION 
import :RenderDoc;
#endif

import Common;

Texture TextureHelpers::CreateBRDFIntegrationLUT(Device& device)
{
    static constexpr uint32_t TextureSize = 512;
    // Epic recommends using R16G16_UNORM for the LUT
    Texture texture{ Texture::Description::CreateTexture2D("Environment BRDF LUT", TextureSize, TextureSize, DXGI_FORMAT_R16G16_UNORM, Texture::TEXTURE_SRV | Texture::TEXTURE_RTV, false), device };

    device.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    device.SetBlendState(BlendStates::Get<BlendPresets::Opaque>(device));

    ShadingModel shadingModel(ResourceManager::Get().shaderLoader, ShaderKey{"FullscreenPass.vs.hlsl"}, ShaderKey{"GenerateEnvironmentBRDFLUT.ps.hlsl"}, device);
    device.SetVertexShader(*shadingModel.vertexShader);
    device.SetPixelShader(*shadingModel.pixelShader);

    D3D11_VIEWPORT viewport;
    viewport.Width = static_cast<FLOAT>(TextureSize);
    viewport.Height = static_cast<FLOAT>(TextureSize);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;

    device.SetViewports({viewport});
    device.UnsetDepthStencilState();
    device.SetRenderTargets({texture.GetRTV()});

    device.ClearRTV(Colors::Red, texture.GetRTV());

    device.Draw(3, 0);

    return texture;
}

ColorSH9 TextureHelpers::CreateDiffuseIrradianceSH(const Texture& environmentCubemap, Device& device)
{
    SC_SCOPED_EVENT_FUNC();

    ColorSH9 result;
    
    std::array<std::vector<std::vector<Color>>, 6> cubemapData = DownloadTextureCubeData<Color>(environmentCubemap, device, 0);
    uint32_t cubeFaceSize = environmentCubemap.GetWidth();
    
    static constexpr auto MaxSHColorValue = 1000.0f;
    
    //==========================================================================================================
    // SH Encoding inspired by Andrew Pham's blog:
    // - https://web.archive.org/web/20231108115516/https://andrew-pham.blog/2019/08/26/spherical-harmonics/
    //==========================================================================================================
    
    float weightSum = 0;
    for (uint32_t face = 0; face < 6; ++face)
    {
        for (uint32_t y = 0; y < cubeFaceSize; ++y)
        {
            for (uint32_t x = 0; x < cubeFaceSize; ++x)
            {
                uint32_t row = y;
                uint32_t col = x;
    
                Vector4 color = Vector4(cubemapData[face][0][row * cubeFaceSize + col]);
    
                // This is to prevent very high values to blowout the SH's precision
                color.Clamp(Vector4::Zero, Vector4::One * MaxSHColorValue);
    
                float u = (x + 0.5f) / cubeFaceSize;
                float v = (y + 0.5f) / cubeFaceSize;
    
                float unitCubeU = u * 2.0f - 1.0f;
                float unitCubeV = v * 2.0f - 1.0f;
                
                auto temp = 1.0f + unitCubeU * unitCubeU + unitCubeV * unitCubeV;
                float weight = 4.0f / (sqrt(temp) * temp);
    
                Vector3 normal = CubemapHelpers::ConvertCubemapFaceUVToVec3(face, Vector2(u,v));
                result += ProjectSH9(normal, color) * weight;
                weightSum += weight;
            }
        }
    }
    
    result *= 4.0f * std::numbers::pi_v<float> / weightSum;
    return result;
}

ColorSH9 TextureHelpers::CreateDiffuseIrradianceSH_Compute(const Texture& environmentCubemap, Device& device)
{
    SC_SCOPED_EVENT_FUNC();

    ColorSH9 result;
    
    ShaderKey key{ "DiffuseIrradianceCompute.cs.hlsl" };

    ComputeShader* compute = ResourceManager::Get().shaderLoader.Load<ComputeShader>(std::move(key), device);

    std::vector<ColorSH9> colors{ {} };
    StructuredBuffer resultBuffer(device, colors, true);

    device.SetComputeShader(*compute);

    device.SetUnorderedAccessView(0, resultBuffer);
    device.SetShaderResource(0, environmentCubemap, COMPUTE_SHADER);

    device.Dispatch(1, 1, 1);

    device.UnsetUnorderedAccessView(0);
    device.UnsetShaderResource(0, COMPUTE_SHADER);

    D3D11_MAPPED_SUBRESOURCE subresource;
    device.Map(resultBuffer.GetRawBuffer(), 0, D3D11_MAP_READ, subresource);
    std::memcpy(&result, subresource.pData, sizeof(ColorSH9));
    device.Unmap(resultBuffer.GetRawBuffer(), 0);

    return result;
}

Texture TextureHelpers::ReduceToMaxLuminance(Device& device, const Texture& texture)
{
    SC_SCOPED_GPU_EVENT_FUNC();

    uint32_t width = texture.GetWidth();
    uint32_t height = texture.GetHeight();

    auto reduceSide = [](uint32_t& side)
    {
        side /= 2;
        if (side > 1 && side % 2 == 1)
            side++;
    };

    std::vector<Texture> downsamplingTextures;
    downsamplingTextures.push_back(texture.CreateCopy(device));

    while (width > 1 || height > 1)
    {
        if (width > 1)
            reduceSide(width);
        if (height > 1)
            reduceSide(height);

        downsamplingTextures.emplace_back(Texture::Description::CreateTexture2D(std::format("MaxLuminanceTexture {}x{}", width, height), width, height, DXGI_FORMAT_R16_FLOAT, Texture::TEXTURE_UAV | Texture::TEXTURE_SRV, false), device);
    }

    ShaderKey csKey;
    csKey.filepath = "Compute/MaxLuminance.cs.hlsl";
    ComputeShader* computeShader = ResourceManager::Get().shaderLoader.Load<ComputeShader>(std::move(csKey), device);

    ShaderKey firstCsKey;
    firstCsKey.filepath = "Compute/MaxLuminance.cs.hlsl";
    firstCsKey.defines = {{"FIRST_PASS", "1"}};
    ComputeShader* firstPassComputeShader = ResourceManager::Get().shaderLoader.Load<ComputeShader>(std::move(firstCsKey), device);

    device.SetComputeShader(*firstPassComputeShader);

    for (int i = 0; i < downsamplingTextures.size() - 1; ++i)
    {
        device.SetShaderResource(0, downsamplingTextures[i], COMPUTE_SHADER);
        device.SetUnorderedAccessView(0, downsamplingTextures[i + 1]);

        auto [x,y,z] = GetThreadCountsForComputeSize(DirectX::XMUINT2{downsamplingTextures[i + 1].GetWidth(), downsamplingTextures[i + 1].GetHeight()}, {8, 8});

        device.Dispatch(x, y, z);

        device.UnsetUnorderedAccessView(0);
        device.UnsetShaderResource(0, COMPUTE_SHADER);

        device.SetComputeShader(*computeShader);
    }

    return std::move(downsamplingTextures.back());
}

Texture TextureHelpers::BuildARMTextureFromData(Device& device,
    const std::optional<Texture::TextureData>& ambientOcclusionData,
    const std::optional<Texture::TextureData>& roughnessData,
    const std::optional<Texture::TextureData>& metallicData)
{
    Assert(ambientOcclusionData || roughnessData || metallicData);

    auto defaultTextureData = CreateUniformColorTextureData(Color(1, 1, 1), 1, 1, Texture::TEXTURE_2D, Texture::DEFAULT, Texture::TEXTURE_SRV);
    Texture ambientTex{ambientOcclusionData ? *ambientOcclusionData : defaultTextureData, device};
    Texture roughnessTex{roughnessData ? *roughnessData : defaultTextureData, device};
    Texture metallicTex{metallicData ? *metallicData : defaultTextureData, device};

    const Texture::TextureData* validTextData;
    if (ambientOcclusionData)
        validTextData = &*ambientOcclusionData;
    else if (roughnessData)
        validTextData = &*roughnessData;
    else if (metallicData)
        validTextData = &*metallicData;

    using namespace std::string_literals;
    auto fullPath = std::filesystem::path(validTextData->description.name);
    std::string path = (fullPath.remove_filename() / fullPath.stem()).string() + "_ARMTexture.png"s;
    Texture outputTexture{ Texture::Description::CreateTexture2D(path,
        validTextData->description.width, validTextData->description.height,
        DXGI_FORMAT_R8G8B8A8_UNORM, Texture::TEXTURE_SRV | Texture::TEXTURE_RTV, false
    ), device };
    
    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<FLOAT>(validTextData->description.width);
    viewport.Height = static_cast<FLOAT>(validTextData->description.height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    device.SetRenderTargets({outputTexture.GetRTV()});
    device.SetViewports({viewport});

    ShadingModel shadingModel(ResourceManager::Get().shaderLoader, ShaderKey{"FullscreenPass.vs.hlsl"}, ShaderKey{"CombineARM.ps.hlsl"}, device);
    device.SetVertexShader(*shadingModel.vertexShader);
    device.SetPixelShader(*shadingModel.pixelShader);

    device.SetShaderResource(0, ambientTex, PIXEL_SHADER);
    device.SetShaderResource(1, roughnessTex, PIXEL_SHADER);
    device.SetShaderResource(2, metallicTex, PIXEL_SHADER);

    device.Draw(3, 0);

    return outputTexture;
}

uint8_t TextureHelpers::GetPixelByteSizeFromFormat(DXGI_FORMAT format)
{
    const std::string_view enumName = magic_enum::enum_name(format);

    std::regex r{"([0-9]+)+"};

    std::match_results<std::string_view::const_iterator> results;

    auto it = enumName.begin();

    int totalBits = 0;
    while (regex_search(it, enumName.end(), results, r))
    {
        std::string value = results.str();
        totalBits += std::stoi(value);
        std::advance(it, results.prefix().length() + value.length());
    }

    return static_cast<uint8_t>(totalBits / 8);
}

void TextureHelpers::SaveTextureToDisk(std::filesystem::path& path, const Texture& texture, Device& device)
{
    /*using namespace DirectX;

    Texture staging(std::format("Staging for saving {}", texture.GetName()), texture.GetType(), Texture::STAGING, texture.GetBindFlags(), texture.GetMipsCount() != 1, device);

    device.CopyResource(staging.rawTexture, texture.rawTexture);

    Image tempImage;*/

    // TODO: finish this in (https://trello.com/c/oZSA9zxZ), its out of scope for this MR.
}

std::vector<char> TextureHelpers::DownloadTexture2DMip0(const Texture& texture, Device& device)
{
    Assert(texture.GetType() == Texture::TEXTURE_2D, "The texture passed must be a 2D texture.");
    
    static int BytesPerPixel = GetPixelByteSizeFromFormat(texture.GetFormat());
    Texture stagingTexture = texture.CreateStagingCopy(device);
    
    std::vector<char> surfaceData;
    surfaceData.reserve(stagingTexture.GetMipsCount());

    Vector2 mipSize = {static_cast<float>(stagingTexture.GetWidth()), static_cast<float>(stagingTexture.GetHeight())};
    
    surfaceData.resize(static_cast<size_t>(mipSize.x * mipSize.y * BytesPerPixel));
    D3D11_MAPPED_SUBRESOURCE resourceDesc = {};
    int index = D3D11CalcSubresource(0, 0, stagingTexture.GetMipsCount());
    device.Map(stagingTexture.GetRawTexture(), index, D3D11_MAP_READ, resourceDesc);

    if (resourceDesc.pData)
    {
        for (int i = 0; i < mipSize.y; ++i)
        {
            std::memcpy(
                reinterpret_cast<uint8_t*>(surfaceData.data()) + static_cast<int>(mipSize.x * BytesPerPixel * i),
                static_cast<uint8_t*>(resourceDesc.pData) + resourceDesc.RowPitch * i,
                static_cast<size_t>(mipSize.x) * BytesPerPixel);
        }
    }

    device.Unmap(stagingTexture.GetRawTexture(), 0);

    return surfaceData;
}
