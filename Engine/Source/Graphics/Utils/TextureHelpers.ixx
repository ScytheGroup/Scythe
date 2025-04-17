export module Graphics:TextureHelpers;

import :Texture;
import Common;


export namespace TextureHelpers
{
    UINT ComputeMipCount(int maxDimension) { return static_cast<UINT>(1 + log2(maxDimension)); }

    Texture CreateBRDFIntegrationLUT(Device& device);
    ColorSH9 CreateDiffuseIrradianceSH(const Texture& environmentCubemap, Device& device);
    ColorSH9 CreateDiffuseIrradianceSH_Compute(const Texture& environmentCubemap, Device& device);
    Texture ReduceToMaxLuminance(Device& device, const Texture& texture);
    Texture BuildARMTextureFromData(Device& device, const std::optional<Texture::TextureData>& ambientOcclusionData, const std::optional<Texture::TextureData>& roughnessData, const std::optional<Texture::TextureData>& metallicData);

    uint8_t GetPixelByteSizeFromFormat(DXGI_FORMAT format);

    // Return structure: vector of the contents of each mip
    template <typename PixelFormat>
    std::vector<std::vector<PixelFormat>> DownloadTexture2DData(const Texture& stagingTexture, Device& device);

    std::vector<char> DownloadTexture2DMip0(const Texture& texture, Device& device);

    // Return structure: Each array value corresponds to a cubemap face and contains the data for each mip from mip-index 0 to max-mip.
    template<typename PixelFormat>
    std::array<std::vector<std::vector<PixelFormat>>, 6> DownloadTextureCubeData(const Texture& cubemap, Device& device, int onlySpecificMip = -1);
    
    void SaveTextureToDisk(std::filesystem::path& path, const Texture& texture, Device& device);
};

template <typename PixelFormat>
std::vector<std::vector<PixelFormat>> TextureHelpers::DownloadTexture2DData(const Texture& texture, Device& device)
{
    Assert(texture.GetType() == Texture::TEXTURE_2D, "The texture passed must be a 2D texture.");
    
    static constexpr int BytesPerPixel = sizeof(PixelFormat);
    static int formatPixelByteSize = GetPixelByteSizeFromFormat(texture.GetFormat());
    Assert(BytesPerPixel == formatPixelByteSize, "The texture's format pixel size must match the specified return format.");

    Texture stagingTexture = texture.CreateStagingCopy(device);
    
    std::vector<std::vector<PixelFormat>> surfaceData;
    surfaceData.reserve(stagingTexture.GetMipsCount());

    Vector2 mipSize = {static_cast<float>(stagingTexture.GetWidth()), static_cast<float>(stagingTexture.GetHeight())};
    for (int mip = 0; mip < stagingTexture.GetMipsCount(); ++mip)
    {
        surfaceData[mip].resize(static_cast<size_t>(mipSize.x * mipSize.y));
        D3D11_MAPPED_SUBRESOURCE resourceDesc = {};
        int index = D3D11CalcSubresource(mip, 0, stagingTexture.GetMipsCount());
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

        mipSize /= 2;
    }

    return surfaceData;
}

template <typename PixelFormat>
std::array<std::vector<std::vector<PixelFormat>>, 6> TextureHelpers::DownloadTextureCubeData(const Texture& cubemap, Device& device, int onlySpecificMip)
{
    Assert(cubemap.GetType() == Texture::TEXTURE_CUBE, "The texture passed must be a cube texture.");
    Assert(cubemap.GetWidth() == cubemap.GetHeight(), "A cubemap cannot have non-cubed faces.");
    
    static constexpr int BytesPerPixel = sizeof(PixelFormat);
    static int formatPixelByteSize = GetPixelByteSizeFromFormat(cubemap.GetFormat());
    Assert(BytesPerPixel == formatPixelByteSize, "The cubemap's format pixel size must match the specified return format.");
    
    int cubeFaceSize = cubemap.GetWidth();
    Assert(IsPowerOfTwo(cubeFaceSize), "Cubemap must have power of two dimensions in order to be downloaded.");
    Texture stagingCubemap = cubemap.CreateStagingCopy(device);
    
    std::array<std::vector<std::vector<PixelFormat>>, 6> faceMips;
    for (int face = 0; face < 6; ++face)
    {
        auto&& faceData = faceMips[face];
        faceData.resize(stagingCubemap.GetMipsCount());
        Vector2 mipSize = {static_cast<float>(cubeFaceSize), static_cast<float>(cubeFaceSize)};
        for (int mip = 0; mip < stagingCubemap.GetMipsCount(); ++mip)
        {
            if (onlySpecificMip != -1 && onlySpecificMip != mip)
                continue;
            
            auto&& mipData = faceData[mip];
            mipData.resize(static_cast<size_t>(mipSize.x * mipSize.y));
            D3D11_MAPPED_SUBRESOURCE resourceDesc = {};
            int index = D3D11CalcSubresource(mip, face, stagingCubemap.GetMipsCount());
            device.Map(stagingCubemap.GetRawTexture(), index, D3D11_MAP_READ, resourceDesc);

            if (resourceDesc.pData)
            {
                for (int i = 0; i < mipSize.y; ++i)
                {
                    std::memcpy(
                        reinterpret_cast<uint8_t*>(mipData.data()) + static_cast<int>(mipSize.x * BytesPerPixel * i),
                        static_cast<uint8_t*>(resourceDesc.pData) + resourceDesc.RowPitch * i,
                        static_cast<size_t>(mipSize.x) * BytesPerPixel);
                }
            }

            device.Unmap(stagingCubemap.GetRawTexture(), index);

            mipSize /= 2;
        }
    }

    return faceMips;
}
