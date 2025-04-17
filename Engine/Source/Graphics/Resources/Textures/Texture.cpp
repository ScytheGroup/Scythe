module;

#include <d3d11.h>
#include "Serialization/SerializationMacros.h"

module Graphics:Texture;

// The following includes are only so that intellisense stops having a stroke...
import :Texture;

import :STBImage;
import :TextureHelpers;

import Common;
import Tools;
import Core;

Texture::Texture(const Description& description, Device& device)
{
    data.description = description;
    InitializeTexture(nullptr, device);
    loaded = true;
}

Texture::Texture(const Description& description)
{
    data.description = description;
    RegisterLazyLoadedResource(true);
}

Texture::Texture(const std::string& name, const ComPtr<ID3D11Texture2D>& sourceTexture, DXGI_FORMAT format, uint8_t bindFlags, Device& device)
    : rawTexture{ sourceTexture }
{
    D3D11_TEXTURE2D_DESC sourceTextureDesc{};
    sourceTexture->GetDesc(&sourceTextureDesc);
    data.description.name = name;
    data.description.format = format;
    data.description.bindFlags = bindFlags;
    data.description.type = TEXTURE_2D;
    data.description.width = sourceTextureDesc.Width;
    data.description.height = sourceTextureDesc.Height;
    Texture::GenerateViews(device);
    loaded = true;
}

Texture Texture::CreateCopy(Device& device, bool forceGenerateMips) const
{
    Texture copy = *this;
    if (forceGenerateMips)
    {
        copy.data.description.generateMips = true;
        copy.data.description.mips = 0;    
    }
    copy.InitializeTexture(nullptr, device);

    if (forceGenerateMips)
    {
        for (int faceId = 0; faceId < (copy.GetType() != TEXTURE_CUBE ? 1 : 6); ++faceId)
        {
            device.CopySubresource(copy.GetRawTexture(), D3D11CalcSubresource(0, faceId, copy.data.description.mips), this->GetRawTexture(), D3D11CalcSubresource(0, faceId, data.description.mips));
        }

        if (copy.data.description.mips != 1)
        {
            Assert(copy.srv, "Should have an SRV to generate the mips");
            device.GenerateMips(copy.srv);    
        }
    }
    else
    {
        device.CopyTexture(copy, *this);
    }

    return copy;
}

Texture Texture::CreateStagingCopy(Device& device) const
{
    Texture copy = *this;
    copy.data.description.usage = STAGING;
    // Staging textures can't be bound to graphics pipeline.
    copy.data.description.bindFlags = 0;
    
    copy.data.description.generateMips = false;
    // Create copy without mip autogen but with the space to fit all of this texture's data.
    copy.InitializeTexture(nullptr, device);
    // Both textures should have the same formats! Copy data directly using CopyResource.
    device.CopyTexture(copy, *this);
    return copy;
}

void Texture::InitializeTexture(const void* textureData, Device& device)
{
    const int pixelByteSize = TextureHelpers::GetPixelByteSizeFromFormat(data.description.format);

    uint32_t bindFlagsDescFormat = 0;
    if (data.description.bindFlags & TEXTURE_RTV)
        bindFlagsDescFormat |= D3D11_BIND_RENDER_TARGET;
    if (data.description.bindFlags & TEXTURE_SRV)
        bindFlagsDescFormat |= D3D11_BIND_SHADER_RESOURCE;
    if (data.description.bindFlags & TEXTURE_UAV)
        bindFlagsDescFormat |= D3D11_BIND_UNORDERED_ACCESS;

    switch (data.description.type)
    {
    case TEXTURE_1D:
    {
        DebugPrintError("1D textures not implemented...");
        break;
    }
    case TEXTURE_2D:
    {
        D3D11_TEXTURE2D_DESC desc;
        desc.Usage = static_cast<D3D11_USAGE>(data.description.usage);
        desc.Format = data.description.format;
        desc.Width = data.description.width;
        desc.Height = data.description.height;
        desc.BindFlags = bindFlagsDescFormat;

        desc.CPUAccessFlags = 0;
        if (data.description.usage == STAGING)
            desc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
        if (data.description.usage == DYNAMIC || data.description.usage == STAGING)
            desc.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;

        desc.ArraySize = 1;
        desc.MiscFlags = 0;
        desc.MipLevels = 1;

        if (data.description.generateMips)
            desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

        if (data.description.mips > 0)
        {
            desc.MipLevels = data.description.mips;
        }
        else if (data.description.generateMips)
        {
            desc.MipLevels = TextureHelpers::ComputeMipCount(std::max(data.description.width, data.description.height));
            data.description.mips = desc.MipLevels;
        }
        
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;

        // No need for data upload if no data is defined
        if (!textureData)
        {
            rawTexture = device.CreateTexture2D(desc, nullptr);
            break;
        }

        D3D11_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pSysMem = textureData;
        subresourceData.SysMemPitch = data.description.width * pixelByteSize;

        if (data.description.generateMips)
        {
            rawTexture = device.CreateTexture2D(desc, nullptr);
            device.UpdateSubresource(rawTexture, subresourceData);
        }
        else
        {
            // We can directly pass in the subresourceData if our texture requires no mipmaps.
            rawTexture = device.CreateTexture2D(desc, &subresourceData);
        }

        break;
    }
    case TEXTURE_3D:
    {
        D3D11_TEXTURE3D_DESC desc;
        desc.Usage = static_cast<D3D11_USAGE>(data.description.usage);
        desc.Format = data.description.format;
        desc.Width = data.description.width;
        desc.Height = data.description.height;
        desc.Depth = data.description.depth;
        desc.BindFlags = bindFlagsDescFormat;

        desc.CPUAccessFlags = 0;
        if (data.description.usage == STAGING)
            desc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
        if (data.description.usage == DYNAMIC || data.description.usage == STAGING)
            desc.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;

        desc.MiscFlags = 0;
        desc.MipLevels = 1;

        if (data.description.generateMips)
            desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

        if (data.description.mips > 0)
        {
            desc.MipLevels = data.description.mips;
        }
        else if (data.description.generateMips)
        {
            desc.MipLevels = TextureHelpers::ComputeMipCount(std::max(data.description.width, data.description.height));
            data.description.mips = desc.MipLevels;
        }
        
        // No need for data upload if no data is defined
        if (!textureData)
        {
            rawTexture = device.CreateTexture3D(desc, nullptr);
            break;
        }

        D3D11_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pSysMem = textureData;
        subresourceData.SysMemPitch = data.description.width * pixelByteSize;

        if (data.description.generateMips)
        {
            rawTexture = device.CreateTexture3D(desc, nullptr);
            device.UpdateSubresource(rawTexture, subresourceData);
        }
        else
        {
            // We can directly pass in the subresourceData if our texture requires no mipmaps.
            rawTexture = device.CreateTexture3D(desc, &subresourceData);
        }
        
        break;
    }
    case TEXTURE_CUBE:
    {
        static constexpr int NbOfFacesToACube = 6;
        static constexpr int NbFacesOnWidth = 4;

        if (data.description.width != data.description.height)
            DebugPrintError("Texture cube can only have square faces.");

        const uint32_t faceSize = data.description.width;

        D3D11_TEXTURE2D_DESC desc;
        desc.Usage = static_cast<D3D11_USAGE>(data.description.usage);
        desc.Format = data.description.format;
        desc.Width = data.description.width;
        desc.Height = data.description.height;
        desc.BindFlags = bindFlagsDescFormat;

        desc.CPUAccessFlags = 0;
        if (data.description.usage == STAGING)
            desc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
        if (data.description.usage == DYNAMIC || data.description.usage == STAGING)
            desc.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;

        desc.ArraySize = NbOfFacesToACube;
        desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
        desc.MipLevels = 1;

        if (data.description.generateMips)
            desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

        if (data.description.mips > 0)
        {
            desc.MipLevels = data.description.mips;
        }
        else if (data.description.generateMips)
        {
            desc.MipLevels = TextureHelpers::ComputeMipCount(std::max(data.description.width, data.description.height));
            data.description.mips = desc.MipLevels;
        }
        
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;

        // No need for data upload if no data is defined
        if (!textureData)
        {
            rawTexture = device.CreateTexture2D(desc, nullptr);
            break;
        }

        const uint32_t faceSideByteSize = faceSize * pixelByteSize;
        const uint32_t faceAreaByteSize = faceSize * faceSideByteSize;
        const uint32_t faceRowByteSize = faceAreaByteSize * NbFacesOnWidth;
        const uint32_t rowByteSize = data.description.width * NbFacesOnWidth * pixelByteSize;

        std::vector<uint8_t> cubemapData(faceAreaByteSize * NbOfFacesToACube);

        D3D11_SUBRESOURCE_DATA subresourceData[NbOfFacesToACube]{};

        // maps a face from a cubemap texture to a specific face in the resulting texture.
        // x and y are indexes in terms of faces in the texture x is between 0 and 3 and y is between 0 and 2
        auto readFace = [&](D3D11_TEXTURECUBE_FACE face, int x, int y) {
            const int destinationByteOffset = face * faceAreaByteSize;
            const uint32_t initalReadPosition = faceRowByteSize * y + faceSideByteSize * x;
            
            for (uint32_t rowI = 0; rowI < faceSize; ++rowI)
            {
                std::copy_n(
                    static_cast<const char*>(textureData) + initalReadPosition + rowI * rowByteSize, 
                    faceSideByteSize,
                    &cubemapData[destinationByteOffset + rowI * faceSideByteSize]
                );
            }

            subresourceData[face].pSysMem = cubemapData.data() + destinationByteOffset;
            subresourceData[face].SysMemPitch = faceSideByteSize;
        };

        readFace(D3D11_TEXTURECUBE_FACE_POSITIVE_Y, 1, 0);
        readFace(D3D11_TEXTURECUBE_FACE_NEGATIVE_X, 0, 1);
        readFace(D3D11_TEXTURECUBE_FACE_POSITIVE_Z, 1, 1);
        readFace(D3D11_TEXTURECUBE_FACE_POSITIVE_X, 2, 1);
        readFace(D3D11_TEXTURECUBE_FACE_NEGATIVE_Z, 3, 1);
        readFace(D3D11_TEXTURECUBE_FACE_NEGATIVE_Y, 1, 2);

        rawTexture = device.CreateTexture2D(desc, subresourceData);

        break;
    }
    default:
    {
        DebugPrintError("Invalid texture type passed: {}", std::to_underlying(data.description.type));
        break;
    }
    }

    GenerateViews(device);

    if (data.description.generateMips)
    {
        if (textureData)
        {
            GenerateMips(device);
        }
    }

    chk << rawTexture->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(data.description.name.size()), data.description.name.c_str());
}

void Texture::GenerateViews(Device& device)
{
    if (data.description.bindFlags & TEXTURE_RTV)
    {
        if (data.description.type == TEXTURE_2D)
        {
            D3D11_RENDER_TARGET_VIEW_DESC desc;
            desc.Format = data.description.format;
            desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipSlice = 0;
            rtv = device.CreateRTV(rawTexture, desc);
        }
        else
        {
            // No rtv description is voluntary, this creates a view that accesses all of the subresources in mipmap level 0.
            rtv = device.CreateRTV(rawTexture);    
        }
    }

    if (data.description.bindFlags & TEXTURE_SRV)
    {
        if (data.description.type == TEXTURE_2D)
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc;
            desc.Format = data.description.format;
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipLevels = data.description.mips;
            desc.Texture2D.MostDetailedMip = 0;
            srv = device.CreateSRV(rawTexture, desc);
        }
        else
        {
            // No srv description creates a view that can access the entire resource.
            srv = device.CreateSRV(rawTexture);
        }
    }

    if (data.description.bindFlags & TEXTURE_UAV)
    {
        if (data.description.type == TEXTURE_2D)
        {
            D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
            desc.Format = data.description.format;
            desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipSlice = 0;
            uav = device.CreateUAV(rawTexture, desc);
        }
        else
        {
            // No uav description creates a view that can access the entire resource.
            uav = device.CreateUAV(rawTexture);
        }
    }
}

void Texture::GenerateMips(Device& device)
{
    if (data.description.mips <= 1)
    {
        DebugPrint("Generating mips is not supported on this texture ({})", data.description.name);
        return;
    }
    
    // Reasonable assumption considering the earlier bindFlags check
    Assert(srv != nullptr && rtv != nullptr, "SRV and RTV should have been defined when creating the texture initially to generate mips.");
    device.GenerateMips(srv);
}

void Texture::Resize(uint32_t inWidth, uint32_t inHeight, Device& device)
{
    data.description.width = inWidth;
    data.description.height = inHeight;
    InitializeTexture(nullptr, device);
}

Texture::TextureData CreateUniformColorTextureData(Color color, int width, int height, Texture::Type type, Texture::Usage usage, uint8_t bindFlags)
{
    Texture::Description description;
    description.name = std::format("UniformColorTextureData: {}", color);
    description.width = width;
    description.height = height;
    description.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.type = type;
    description.usage = usage;
    description.generateMips = false;
    description.mips = 1;
    description.bindFlags = bindFlags;
    // Ensure that the color's values are between 0 and 1
    color.Saturate();
    static constexpr float MaxUnsignedByteValue = std::numeric_limits<uint8_t>::max();
    UByteColor packedColor = { static_cast<uint8_t>(color.x * MaxUnsignedByteValue),
                                           static_cast<uint8_t>(color.y * MaxUnsignedByteValue),
                                           static_cast<uint8_t>(color.z * MaxUnsignedByteValue),
                                           static_cast<uint8_t>(color.w * MaxUnsignedByteValue) };

    Texture::TextureData textureData;
    textureData.FillWith(packedColor, width * height);
    textureData.description = description;
    return textureData;
}

// Tiles two colors on a grid pattern
// cellCount is the number of cells per sides
// cellResolution is the number of pixels per cell. Note: cellResolution must be powers of 2
Texture::TextureData CreateCheckerboardTexture(Color inColorA, Color inColorB, int cellCout, int cellResolution)
{
    uint32_t colorA = inColorA.RGBA().v;
    uint32_t colorB = inColorB.RGBA().v;
    std::vector<uint32_t> colors;

    int size = cellResolution * cellCout;
    colors.reserve(size * size);

    bool useColorA = true;
        
    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            colors.push_back(useColorA ? colorA : colorB);

            if ((x + 1) % cellResolution == 0) useColorA = !useColorA;
        }

        if ((y + 1) % cellResolution == 0) useColorA = !useColorA;
    }

    Texture::TextureData data;
    data.description = Texture::Description::CreateTexture2D("", size, size, DXGI_FORMAT_R8G8B8A8_UNORM, Texture::BindFlags::TEXTURE_SRV, false);
    auto byteSize = colors.size() * sizeof(uint32_t);
    data.data.resize(byteSize);
    std::memcpy(data.data.data(), colors.data(), byteSize);
    return data;
}


Texture::Texture(const TextureData& textureData, Device& device)
    : data{textureData}
{
    InitializeTexture(textureData.data.empty() ? nullptr : textureData.data.data(), device);
    loaded = true;
}

Texture::Texture(const TextureData& textureData)
    : data{textureData}
{
    RegisterLazyLoadedResource(true);
}

Texture::TextureData::TextureData(const std::string& name, const STBImage& image, Texture::Type type, Texture::Usage usage, uint8_t bindFlags, bool generateMips)
    : description{
        .name = name,
        .width = image.GetWidth(),
        .height = image.GetHeight(),
        .format = image.IsFloatingPointFormat() ? DXGI_FORMAT_R32G32B32A32_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM,
        .type = type,
        .usage = usage,
        .bindFlags = bindFlags,
        .generateMips = generateMips,
        .mips = generateMips ? TextureHelpers::ComputeMipCount(std::max(description.width, std::max(description.height, description.depth))) : 0,
    }
{
    data.resize(image.GetByteSize());
    std::copy_n(static_cast<char*>(image.Data()), image.GetByteSize(), data.begin());
}

Texture::TextureData::TextureData(const std::filesystem::path& filename, Texture::Type type, Texture::Usage usage, uint8_t bindFlags, bool generateMips)
{
    STBImage image{filename};
    description.name = filename.stem().string();
    description.generateMips = generateMips;
    description.type = type;
    description.usage = usage;
    description.bindFlags = bindFlags;
    description.width = image.GetWidth();
    description.height = image.GetHeight();
    description.format = image.IsFloatingPointFormat() ? DXGI_FORMAT_R32G32B32A32_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM;

    data.resize(image.GetByteSize());
    std::copy_n(static_cast<char*>(image.Data()), image.GetByteSize(), data.begin());
}

Texture::Description Texture::Description::CreateTextureCube(const std::string& name, uint32_t size, DXGI_FORMAT format, bool generateMips)
{
    return {
        .name = name,
        .width = size,
        .height = size,
        .format = format,
        .type = TEXTURE_CUBE,
        .usage = DEFAULT,
        .bindFlags = TEXTURE_RTV | TEXTURE_SRV,
        .generateMips = generateMips,
        .mips = generateMips ? TextureHelpers::ComputeMipCount(size) : 1,
    };
}

Texture::Description Texture::Description::CreateTexture3D(const std::string& name, uint32_t width, uint32_t height, uint32_t depth, DXGI_FORMAT format, uint8_t bindFlags, bool generateMips)
{
    return {
        .name = name,
        .width = width,
        .height = height,
        .depth = depth,
        .format = format,
        .type = TEXTURE_3D,
        .bindFlags = bindFlags,
        .generateMips = generateMips,
        .mips = generateMips ? TextureHelpers::ComputeMipCount(std::max(width, std::max(height, depth))) : 1,
    };
}

Texture::Description Texture::Description::CreateTexture2D(const std::string& name, uint32_t width, uint32_t height, DXGI_FORMAT format, uint8_t bindFlags, bool generateMips)
{
    return {
        .name = name,
        .width = width,
        .height = height,
        .format = format,
        .type = TEXTURE_2D,
        .bindFlags = bindFlags,
        .generateMips = generateMips,
        .mips = generateMips ? TextureHelpers::ComputeMipCount(std::max(width, height)) : 1,
    };
}

void Texture::LoadResource(Device& device)
{
    ILazyLoadedResource::LoadResource(device);
    InitializeTexture(data.data.empty() ? nullptr : data.data.data(), device);
}

#ifdef IMGUI_ENABLED

import ImGui;

bool Texture::ImGuiDraw()
{
    bool result = false;
    
    bool useTwoColumns = data.description.type == Type::TEXTURE_2D;
    if (ImGui::BeginTable("MaterialTable", useTwoColumns ? 2 : 1))
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        if (data.description.type == Type::TEXTURE_2D)
        {
            ImGui::Image(srv.Get(), ImVec2(200, 200));

            ImGui::TableNextColumn();        
        }

        switch (data.description.type)
        {
        case TEXTURE_1D:
            ImGui::Text(std::format("Size: {}", data.description.width).c_str());
            break;
        case TEXTURE_CUBE:
        case TEXTURE_2D:
            ImGui::Text(std::format("Size: {}x{}", data.description.width, data.description.height).c_str());
            break;
        case TEXTURE_3D:
            ImGui::Text(std::format("Size: {}x{}x{}", data.description.width, data.description.height, data.description.depth).c_str());
            break;
        }

        ImGui::Text("Bindings:");
        if (srv)
        {
            ImGui::SameLine();
            ImGui::Text("SRV");
        }

        if (rtv)
        {
            ImGui::SameLine();
            ImGui::Text("RTV");
        }

        if (uav)
        {
            ImGui::SameLine();
            ImGui::Text("UAV");
        }

        ImGui::Text("Format: ");
        ImGui::SameLine();
        ImGui::Text(std::string(magic_enum::enum_name(data.description.format)).c_str());

        ImGui::Text("Type: ");
        ImGui::SameLine();
        ImGui::Text(std::string(magic_enum::enum_name(data.description.type)).c_str());

        ImGui::EndTable();
    }

    return result;
}

#endif

DEFINE_SERIALIZATION_AND_DESERIALIZATION(Texture::Description, name, width, height, depth, arraySize, format, type, usage, bindFlags, generateMips, mips)

template<>
ArchiveStream& operator<<(ArchiveStream& ar, const Texture::Description& desc)
{
    ar << desc.name;
    ar << desc.width;
    ar << desc.height;
    ar << desc.depth;
    ar << desc.arraySize;
    ar << desc.format;
    ar << desc.type;
    ar << desc.usage;
    ar << desc.bindFlags;
    ar << desc.generateMips;
    ar << desc.mips;
    return ar;
}

template<>
ArchiveStream& operator>>(ArchiveStream& ar, Texture::Description& desc)
{
    ar >> desc.name;
    ar >> desc.width;
    ar >> desc.height;
    ar >> desc.depth;
    ar >> desc.arraySize;
    ar >> desc.format;
    ar >> desc.type;
    ar >> desc.usage;
    ar >> desc.bindFlags;
    ar >> desc.generateMips;
    ar >> desc.mips;
    return ar;
}

template<>
ArchiveStream& operator<<(ArchiveStream& ar, const Texture::TextureData& data)
{
    ar << data.data;
    ar << data.description;
    return ar;
}

template<>
ArchiveStream& operator>>(ArchiveStream& ar, Texture::TextureData& data)
{
    ar >> data.data;
    ar >> data.description;
    return ar;
}

std::unique_ptr<Texture> TextureLoaderUtils::LoadAsset(ArchiveStream& loadStream)
{
    Texture::TextureData textureData;
    loadStream >> textureData;
    return std::make_unique<Texture>(std::move(textureData));
}

void TextureLoaderUtils::LoadPresets(Device& device, ResourceLoader<Texture>::CacheType& cache)
{
    {
        Texture::TextureData data = CreateCheckerboardTexture(Colors::Purple, Colors::Black, 8, 32);
        data.description.name = "MissingTexture";
                
        auto tex = std::make_unique<Texture>(data);
        tex->RegisterLazyLoadedResource(true);
        cache[Guid("Missing")] = ResourceLoader<Texture>::LoaderEntry{
            .metadata = AssetMetadata{
                .name = "MissingTexture",
                .isLoaded = true,
                .isTransient = true,
            },
            .asset = std::move(tex)
        };
    }
}

bool TextureLoaderUtils::HandlesExtension(const std::string& extension)
{
    constexpr const char* supportedAssets[] { "png", "jpg", "hdr" }; // Add extensions as needed
    return std::ranges::find(supportedAssets, extension) != std::end(supportedAssets);
}

// This needs to be after the Texture's operator>> and operator<<
void TextureLoaderUtils::SaveAsset(const Texture& asset, ArchiveStream& saveStream, EntityComponentSystem& ecs)
{
    const auto& data = asset.GetData();
    if (data.data.empty())
    {
        ecs.GetSystem<Graphics>().DownloadResource(asset);
    }
    saveStream << data;    
}
