module Graphics:DepthStencil;

import :DepthStencil;
import :Device;

DepthStencil::DepthStencil(Device& device, uint32_t width, uint32_t height, uint32_t arraySize, bool cubeMap)
    : arraySize{arraySize}
    , cubeMap{cubeMap}
{
    D3D11_TEXTURE2D_DESC desc;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.Format = DXGI_FORMAT_R32_TYPELESS;
    desc.Height = height;
    desc.Width = width;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
    desc.CPUAccessFlags = 0;
    desc.ArraySize = arraySize;
    desc.MipLevels = 1;
    desc.MiscFlags = cubeMap ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    rawTexture = device.CreateTexture2D(desc, nullptr);
    
    GenerateViews(device);
}

void DepthStencil::GenerateViews(Device& device)
{
    D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
    desc.Format = DXGI_FORMAT_R32_FLOAT;
    if (cubeMap)
    {
        Assert(arraySize % 6 == 0);
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
        desc.TextureCubeArray.NumCubes = arraySize / 6; 
        desc.TextureCubeArray.First2DArrayFace = 0;
        desc.TextureCubeArray.MipLevels = 1;
        desc.TextureCubeArray.MostDetailedMip = 0;
    }
    else if (arraySize > 1)
    {
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        desc.Texture2DArray.ArraySize = arraySize; 
        desc.Texture2DArray.FirstArraySlice = 0;
        desc.Texture2DArray.MipLevels = 1;
        desc.Texture2DArray.MostDetailedMip = 0;
    }
    else
    {
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipLevels = 1;
        desc.Texture2D.MostDetailedMip = 0;
    }
    srv = device.CreateSRV(rawTexture, desc);   

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;

    if (arraySize > 1)
    {
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Texture2DArray.ArraySize = 1; 
        dsvDesc.Texture2DArray.MipSlice = 0;
        for (uint32_t i = 0; i < arraySize; ++i)
        {
            dsvDesc.Texture2DArray.FirstArraySlice = i;
            dsvs.push_back(device.CreateDSV(rawTexture, dsvDesc));
        }
    }
    else
    {
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsvs.push_back(device.CreateDSV(rawTexture, dsvDesc));
    }
}
