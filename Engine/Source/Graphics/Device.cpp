module;
// Need macros :L
#include <d3d11.h>
module Graphics:Device;

import :Resources;
import :Shader;
import :InputLayout;
import :ShadingModel;
import :BlendState;
import :RasterizerState;
import :FrameBuffer;

Device::Device(HWND window, uint32_t width, uint32_t height)
{
    chk << CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
    CreateDevice();
    CreateSwapchain(window, width, height);
}

void Device::ClearRTV(const Color& clearColor, const ComPtr<ID3D11RenderTargetView>& renderTarget)
{
    deviceContext->ClearRenderTargetView(renderTarget.Get(), reinterpret_cast<const FLOAT*>(&clearColor));
}

void Device::ClearUAV(const ComPtr<ID3D11UnorderedAccessView>& uav, uint32_t value)
{
    UINT clear[4]{ value, 0,0,0 };
    deviceContext->ClearUnorderedAccessViewUint(uav.Get(), clear);
}

void Device::ClearDepthStencilView(const ComPtr<ID3D11DepthStencilView>& depthStencilView)
{
    // TODO: if we add stencil, we will have to clear it as well
    deviceContext->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void Device::UnsetRenderTargets()
{
    ID3D11RenderTargetView* views[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] { nullptr };
    deviceContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, views, nullptr, 0, 0, nullptr, nullptr);
}

void Device::SetRenderTargets(const std::vector<ComPtr<ID3D11RenderTargetView>>& renderTargets)
{
    if (renderTargets.empty())
        return;
    deviceContext->OMSetRenderTargets(static_cast<UINT>(renderTargets.size()), renderTargets.data()->GetAddressOf(),
                                      nullptr);
}

void Device::SetRenderTargetsAndUnorderedAccessViews(
    const std::vector<ComPtr<ID3D11RenderTargetView>>& renderTargets,
    const ComPtr<ID3D11DepthStencilView>& depthStencilTarget,
    const std::vector<IReadWriteResource*>& unorderedAcessViews)
{
    std::vector<ID3D11UnorderedAccessView*> uavs = unorderedAcessViews
        | std::views::transform([](IReadWriteResource* res) { return res->GetUAV().Get(); })
        | std::ranges::to<std::vector>();
    
    deviceContext->OMSetRenderTargetsAndUnorderedAccessViews(
        static_cast<UINT>(renderTargets.size()), renderTargets.data()->GetAddressOf(),
        depthStencilTarget.Get(),
        0, static_cast<UINT>(unorderedAcessViews.size()),
        uavs.data(), nullptr
    );
}

void Device::SetRenderTargetsAndDepthStencilView(const std::vector<ComPtr<ID3D11RenderTargetView>>& renderTargets,
                                                 const ComPtr<ID3D11DepthStencilView>& depthStencilTarget)
{
    deviceContext->OMSetRenderTargets(static_cast<UINT>(renderTargets.size()), renderTargets.data()->GetAddressOf(), depthStencilTarget.Get());
}

void Device::SetDepthStencilState(const DepthStencilState& depthStencilState)
{
    deviceContext->OMSetDepthStencilState(depthStencilState.depthStencilState.Get(), 0);
}

void Device::UnsetDepthStencilState()
{
    ID3D11DepthStencilView* dsv = nullptr;
    deviceContext->OMSetRenderTargets(0, nullptr, dsv);
}

void Device::SetRasterizerState(const RasterizerState& rasterizerState)
{
    deviceContext->RSSetState(rasterizerState.rasterizerState.Get());
}

void Device::SetRasterizerState(const ComPtr<ID3D11RasterizerState>& rasterizerState)
{
    deviceContext->RSSetState(rasterizerState.Get());
}

void Device::SetSamplers(uint32_t startSlot, ViewSpan<ComPtr<ID3D11SamplerState>> samplersViewSpan,
                         PipelineStageFlags stageFlags)
{
    uint32_t numElements = samplersViewSpan.count;

    if (numElements > MaxSamplerSlots - startSlot)
    {
        DebugPrintError(
            "Too many samplers passed to SetSamplers (startSlot: {}), max amount authorized is {}.",
            startSlot, MaxSamplerSlots);
    }

    std::array<ID3D11SamplerState*, MaxSamplerSlots> rawSamplers;
    std::ranges::transform(samplersViewSpan, std::begin(rawSamplers),
                           [](ComPtr<ID3D11SamplerState>& samplerState) { return samplerState.Get(); });

    if (stageFlags & VERTEX_SHADER)
    {
        deviceContext->VSSetSamplers(startSlot, samplersViewSpan.count, rawSamplers.data());
    }

    if (stageFlags & PIXEL_SHADER)
    {
        deviceContext->PSSetSamplers(startSlot, samplersViewSpan.count, rawSamplers.data());
    }

    if (stageFlags & COMPUTE_SHADER)
    {
        deviceContext->CSSetSamplers(startSlot, samplersViewSpan.count, rawSamplers.data());
    }
}

void Device::SetViewports(const std::vector<D3D11_VIEWPORT>& viewportSpan)
{
    deviceContext->RSSetViewports(static_cast<UINT>(viewportSpan.size()), viewportSpan.data());
}

void Device::SetBlendState(const BlendState& blendState)
{
    deviceContext->OMSetBlendState(blendState.blendState.Get(), nullptr, 0xffffffff);
}

void Device::DisableBlending()
{
    deviceContext->OMSetBlendState(nullptr,  nullptr, 0xffffffff);
}

void Device::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
{
    deviceContext->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void Device::Present(bool activateVsync)
{
    chk << swapChain->Present(activateVsync, 0);
}

void Device::Flush()
{
    deviceContext->Flush();
}

void Device::OnResize(uint32_t width, uint32_t height)
{
    chk << swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
}

ComPtr<ID3D11VertexShader> Device::CreateVertexShader(const ComPtr<ID3DBlob>& vertexShaderBytecode)
{
    ComPtr<ID3D11VertexShader> vs;
    chk << device->CreateVertexShader(vertexShaderBytecode->GetBufferPointer(), vertexShaderBytecode->GetBufferSize(), nullptr, &vs);
    return vs;
}

ComPtr<ID3D11PixelShader> Device::CreatePixelShader(const ComPtr<ID3DBlob>& pixelShaderBytecode)
{
    ComPtr<ID3D11PixelShader> ps;
    chk << device->CreatePixelShader(pixelShaderBytecode->GetBufferPointer(), pixelShaderBytecode->GetBufferSize(), nullptr, &ps);
    return ps;
}

ComPtr<ID3D11ComputeShader> Device::CreateComputeShader(const ComPtr<ID3DBlob>& computeShaderBytecode)
{
    ComPtr<ID3D11ComputeShader> ps;
    chk << device->CreateComputeShader(computeShaderBytecode->GetBufferPointer(), computeShaderBytecode->GetBufferSize(), nullptr, &ps);
    return ps;
}

ComPtr<ID3D11GeometryShader> Device::CreateGeometryShader(const ComPtr<ID3DBlob>& geometryShaderBytecode)
{
    ComPtr<ID3D11GeometryShader> ps;
    chk << device->CreateGeometryShader(geometryShaderBytecode->GetBufferPointer(), geometryShaderBytecode->GetBufferSize(), nullptr, &ps);
    return ps;
}

ComPtr<ID3D11Texture2D> Device::CreateTexture2D(const D3D11_TEXTURE2D_DESC& textureDesc, D3D11_SUBRESOURCE_DATA* initialData)
{
    UINT support;
    chk << device->CheckFormatSupport(textureDesc.Format, &support);
    Assert((support & D3D11_FORMAT_SUPPORT_TEXTURE2D) != 0, "Format not supported for texture 2d");
    
    ComPtr<ID3D11Texture2D> texture;
    chk << device->CreateTexture2D(&textureDesc, initialData, &texture);
    return texture;
}

ComPtr<ID3D11Texture3D> Device::CreateTexture3D(const D3D11_TEXTURE3D_DESC& textureDesc, D3D11_SUBRESOURCE_DATA* initialData)
{
    UINT support;
    chk << device->CheckFormatSupport(textureDesc.Format, &support);
    Assert((support & D3D11_FORMAT_SUPPORT_TEXTURE3D) != 0, "Format not supported for texture 3d");
    
    ComPtr<ID3D11Texture3D> texture;
    chk << device->CreateTexture3D(&textureDesc, initialData, &texture);
    return texture;
}

ComPtr<ID3D11DepthStencilState> Device::CreateDepthStencilState(const D3D11_DEPTH_STENCIL_DESC& desc)
{
    ComPtr<ID3D11DepthStencilState> depthStencilState;
    chk << device->CreateDepthStencilState(&desc, &depthStencilState);
    return depthStencilState;
}

ComPtr<ID3D11InputLayout> Device::CreateInputLayout(const VertexShader& shader, const InputLayoutDescription& input)
{
    ComPtr<ID3D11InputLayout> inputLayout;
    chk << device->CreateInputLayout(input.GetInputElementDesc(), input.GetElementCount(),
                              shader.shaderBytecode->GetBufferPointer(), shader.shaderBytecode->GetBufferSize(),
                              inputLayout.GetAddressOf());
    return inputLayout;
}

ComPtr<ID3D11RasterizerState> Device::CreateRasterizerState(const D3D11_RASTERIZER_DESC& desc)
{
    ComPtr<ID3D11RasterizerState> rasterizerState;
    chk << device->CreateRasterizerState(&desc, rasterizerState.GetAddressOf());
    return rasterizerState;
}

ComPtr<ID3D11SamplerState> Device::CreateSamplerState(const D3D11_SAMPLER_DESC& desc)
{
    ComPtr<ID3D11SamplerState> samplerState;
    chk << device->CreateSamplerState(&desc, samplerState.GetAddressOf());
    return samplerState;
}

ComPtr<ID3D11BlendState> Device::CreateBlendState(const D3D11_BLEND_DESC& desc)
{
    ComPtr<ID3D11BlendState> blendState;
    chk << device->CreateBlendState(&desc, blendState.GetAddressOf());
    return blendState;
}

void Device::SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY primitiveTopology)
{
    deviceContext->IASetPrimitiveTopology(primitiveTopology);
}

void Device::SetVertexShader(VertexShader& vertexShader)
{
    deviceContext->VSSetShader(vertexShader.vertexShader.Get(), nullptr, 0);
    SetInputLayout(vertexShader.inputLayout);
}

void Device::UnsetVertexShader()
{
    deviceContext->VSSetShader(nullptr, nullptr, 0);
    deviceContext->IASetInputLayout(nullptr);
}

void Device::SetPixelShader(PixelShader& pixelShader)
{
    deviceContext->PSSetShader(pixelShader.pixelShader.Get(), nullptr, 0);
}

void Device::UnsetPixelShader()
{
    deviceContext->PSSetShader(nullptr, nullptr, 0);
}

void Device::SetGeometryShader(GeometryShader& geometryShader)
{
    deviceContext->GSSetShader(geometryShader.geometryShader.Get(), nullptr, 0);
}

void Device::UnsetGeometryShader()
{
    deviceContext->GSSetShader(nullptr, nullptr, 0);   
}

void Device::SetComputeShader(ComputeShader& computeShader)
{
    deviceContext->CSSetShader(computeShader.computeShader.Get(), nullptr, 0);
}

void Device::CopyTexture(Texture& destTexture, const Texture& sourceTexture)
{
    deviceContext->CopyResource(destTexture.GetRawTexture(), sourceTexture.GetRawTexture());
}

void Device::SetFrameBuffer(FrameBuffer& framebuffer)
{
    SetRenderTargetsAndDepthStencilView(framebuffer.GetRTVs(), framebuffer.depthStencil.GetDSV());
    SetViewports({ framebuffer.viewport });
}

void Device::SetFrameBuffer(FrameBuffer& framebuffer, const DepthStencil& depthOverride)
{
    SetRenderTargetsAndDepthStencilView(framebuffer.GetRTVs(), depthOverride.GetDSV());
    SetViewports({ framebuffer.viewport });
}

void Device::SetFrameBufferNoDepth(FrameBuffer& framebuffer)
{
    SetRenderTargetsAndDepthStencilView(framebuffer.GetRTVs(), {});
    SetViewports({ framebuffer.viewport });
}

void Device::SetShadingModel(const ShadingModel& shaderModel)
{
    SetVertexShader(*shaderModel.vertexShader);
    SetPixelShader(*shaderModel.pixelShader);
}

void Device::CleanupDraw()
{
    // Bind an array of nullpointing ID3D11ShaderResourceView* to unbind ALL srv slots
    ID3D11ShaderResourceView* ptrs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = { nullptr };
    deviceContext->VSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, ptrs);
    deviceContext->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, ptrs);
    deviceContext->GSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, ptrs);
}

void Device::UnsetVertexBuffer(uint32_t startSlot, uint32_t count)
{
    std::vector<ID3D11Buffer*> buffs{ count, nullptr };
    std::vector<uint32_t> strides{ count, 0 };
    std::vector<uint32_t> offsets{ count, 0 };
    deviceContext->IASetVertexBuffers(startSlot, count, buffs.data(), strides.data(), offsets.data());
}

void Device::UnsetIndexBuffer()
{
    deviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
}

void Device::Draw(uint32_t vertexCount, uint32_t startVertexLocation)
{
    deviceContext->Draw(vertexCount, startVertexLocation);
}

void Device::DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation)
{
    deviceContext->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

void Device::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, uint32_t baseVertexLocation)
{
    deviceContext->DrawIndexed(indexCount, startIndexLocation, baseVertexLocation);
}

ComPtr<ID3D11Texture2D> Device::GetBackBuffer() const
{
    ComPtr<ID3D11Texture2D> backBuffer = nullptr;
    chk << swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    return backBuffer;
}

void Device::UpdateSubresource(const ComPtr<ID3D11Resource>& destResource, const D3D11_SUBRESOURCE_DATA& srcData)
{
    deviceContext->UpdateSubresource(destResource.Get(), 0, nullptr, srcData.pSysMem, srcData.SysMemPitch,
                                     srcData.SysMemSlicePitch);
}

void Device::CopySubresource(const ComPtr<ID3D11Resource>& destResource, uint32_t dstMipLevel, const ComPtr<ID3D11Resource>& srcResource, uint32_t srcMipLevel)
{
    deviceContext->CopySubresourceRegion(destResource.Get(), dstMipLevel, 0, 0, 0, srcResource.Get(), srcMipLevel, nullptr);
}

void Device::CopyResource(const ComPtr<ID3D11Resource>& destResource, const ComPtr<ID3D11Resource>& srcResource)
{
    deviceContext->CopyResource(destResource.Get(), srcResource.Get());
}

void Device::Map(const ComPtr<ID3D11Resource>& resource, uint32_t subresourceIndex, D3D11_MAP mapType, D3D11_MAPPED_SUBRESOURCE& mappedSubresource)
{
    chk << deviceContext->Map(resource.Get(), subresourceIndex, mapType, 0, &mappedSubresource);
}

void Device::Unmap(const ComPtr<ID3D11Resource>& resource, uint32_t subresourceIndex)
{
    deviceContext->Unmap(resource.Get(), subresourceIndex);
}

void Device::CreateDevice()
{
    // Enumerate adapters to find the one with most VRAM
    ComPtr<IDXGIAdapter1> bestAdapter;
    SIZE_T bestVRAM = 0;

    for (UINT adapterIndex = 0;; ++adapterIndex)
    {
        ComPtr<IDXGIAdapter1> adapter;
        if (dxgiFactory->EnumAdapters1(adapterIndex, &adapter) == DXGI_ERROR_NOT_FOUND)
            break;

        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        // Skip software adapter (WARP)
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        // Check if this adapter has more VRAM than our current best
        if (desc.DedicatedVideoMemory > bestVRAM)
        {
            bestAdapter = adapter;
            bestVRAM = desc.DedicatedVideoMemory;
        }
    }

    Assert(bestAdapter != nullptr, "No suitable GPU adapter found");

    // Will try until finding a supported feature level (or running out of feature levels to try)
    static constexpr D3D_FEATURE_LEVEL FeatureLevels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };

    UINT creationFlags = 0;

#ifdef SC_DEV_VERSION
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    chk << D3D11CreateDevice(bestAdapter.Get(),
                            D3D_DRIVER_TYPE_UNKNOWN, // Use UNKNOWN since we're providing an adapter
                            nullptr,
                            creationFlags,
                            FeatureLevels,
                            ARRAYSIZE(FeatureLevels),
                            D3D11_SDK_VERSION,
                            &device,
                            nullptr,
                            &deviceContext);

    // Log the selected adapter info
    DXGI_ADAPTER_DESC1 selectedDesc;
    bestAdapter->GetDesc1(&selectedDesc);
    DebugPrint("Selected GPU: {} with {} MB VRAM", 
               WStringToString(std::wstring(selectedDesc.Description)),
               selectedDesc.DedicatedVideoMemory / (1024 * 1024));
}

void Device::CreateSwapchain(HWND window, uint32_t width, uint32_t height)
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDescriptor = {};
    swapChainDescriptor.Width = width;
    swapChainDescriptor.Height = height;
    swapChainDescriptor.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDescriptor.SampleDesc.Count = 1;
    swapChainDescriptor.SampleDesc.Quality = 0;
    swapChainDescriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDescriptor.BufferCount = 2;
    swapChainDescriptor.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDescriptor.Scaling = DXGI_SCALING_STRETCH;
    swapChainDescriptor.Flags = {};

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDescriptor = {};
    swapChainFullscreenDescriptor.Windowed = true;

    chk << dxgiFactory->CreateSwapChainForHwnd(device.Get(), window, &swapChainDescriptor,
                                               &swapChainFullscreenDescriptor, nullptr, &swapChain);
}

void Device::UnsetConstantBuffers(uint32_t startSlot, PipelineStageFlags stageFlags)
{
    ID3D11Buffer* buff[1] { nullptr };
    if (stageFlags & VERTEX_SHADER)
        deviceContext->VSSetConstantBuffers(startSlot, 1, buff);

    if (stageFlags & PIXEL_SHADER)
        deviceContext->PSSetConstantBuffers(startSlot, 1, buff);

    if (stageFlags & GEOMETRY_SHADER)
        deviceContext->PSSetConstantBuffers(startSlot, 1, buff);

    if (stageFlags & COMPUTE_SHADER)
        deviceContext->CSSetConstantBuffers(startSlot, 1, buff);
}

void Device::SetInputLayout(const ComPtr<ID3D11InputLayout>& inputLayout)
{
    deviceContext->IASetInputLayout(inputLayout.Get());
}

void Device::UnsetShaderResource(uint32_t slot, PipelineStageFlags stageFlags)
{
    ID3D11ShaderResourceView* res[] {nullptr};
    if (stageFlags & VERTEX_SHADER)
    {
        deviceContext->VSSetShaderResources(slot, 1, res);
    }

    if (stageFlags & PIXEL_SHADER)
    {
        deviceContext->PSSetShaderResources(slot, 1, res);
    }

    if (stageFlags & COMPUTE_SHADER)
    {
        deviceContext->CSSetShaderResources(slot, 1, res);
    }
}

void Device::SetShaderResource(uint32_t slot, const IReadableResource& resource, PipelineStageFlags stageFlags)
{
    UnsetShaderResource(slot, stageFlags);
    
    if (stageFlags & VERTEX_SHADER)
        deviceContext->VSSetShaderResources(slot, 1, resource.GetSRV().GetAddressOf());

    if (stageFlags & PIXEL_SHADER)
        deviceContext->PSSetShaderResources(slot, 1, resource.GetSRV().GetAddressOf());

    if (stageFlags & COMPUTE_SHADER)
        deviceContext->CSSetShaderResources(slot, 1, resource.GetSRV().GetAddressOf());

    if (stageFlags & GEOMETRY_SHADER)
        deviceContext->GSSetShaderResources(slot, 1, resource.GetSRV().GetAddressOf());
}

void Device::SetShaderResources(uint32_t slot, const ViewSpan<IReadableResource*>& resource, PipelineStageFlags stageFlags)
{
    std::vector<ID3D11ShaderResourceView*> SRVs{resource.count};
    std::ranges::transform(resource, SRVs.begin(), [](auto&& tex) { return tex->GetSRV().Get(); });
    
    if (stageFlags & VERTEX_SHADER)
        deviceContext->VSSetShaderResources(slot, static_cast<UINT>(SRVs.size()), SRVs.data());

    if (stageFlags & PIXEL_SHADER)
        deviceContext->PSSetShaderResources(slot, static_cast<UINT>(SRVs.size()), SRVs.data());
    
    if (stageFlags & COMPUTE_SHADER)
        deviceContext->CSSetShaderResources(slot, static_cast<UINT>(SRVs.size()), SRVs.data());

    if (stageFlags & GEOMETRY_SHADER)
        deviceContext->GSSetShaderResources(slot, static_cast<UINT>(SRVs.size()), SRVs.data());
}

void Device::SetUnorderedAccessView(uint32_t slot, const IReadWriteResource& uav)
{
    deviceContext->CSSetUnorderedAccessViews(slot, 1, uav.GetUAV().GetAddressOf(), nullptr);
}

void Device::UnsetUnorderedAccessView(uint32_t slot)
{
    ID3D11UnorderedAccessView* views[] { nullptr };
    deviceContext->CSSetUnorderedAccessViews(slot, 1, views, nullptr);
}

MappedResource Device::MapTexture(Texture& texture, uint32_t subresourceIndex)
{
    Assert(texture.GetUsage() == Texture::STAGING, "Can only map STAGING textures");
    return {texture.GetRawTexture(), subresourceIndex, deviceContext};
}

ComPtr<ID3D11RenderTargetView> Device::CreateRTV(const ComPtr<ID3D11Resource>& resource)
{
    ComPtr<ID3D11RenderTargetView> renderTarget;
    chk << device->CreateRenderTargetView(resource.Get(), nullptr, &renderTarget);
    return renderTarget;
}

ComPtr<ID3D11RenderTargetView> Device::CreateRTV(const ComPtr<ID3D11Resource>& resource,
                                                 const D3D11_RENDER_TARGET_VIEW_DESC& desc)
{
    ComPtr<ID3D11RenderTargetView> renderTarget;
    chk << device->CreateRenderTargetView(resource.Get(), &desc, &renderTarget);
    return renderTarget;
}

ComPtr<ID3D11ShaderResourceView> Device::CreateSRV(const ComPtr<ID3D11Resource>& resource)
{
    ComPtr<ID3D11ShaderResourceView> shaderResourceView;
    chk << device->CreateShaderResourceView(resource.Get(), nullptr, &shaderResourceView);
    return shaderResourceView;
}

ComPtr<ID3D11ShaderResourceView> Device::CreateSRV(const ComPtr<ID3D11Resource>& resource,
                                                   const D3D11_SHADER_RESOURCE_VIEW_DESC& desc)
{
    ComPtr<ID3D11ShaderResourceView> shaderResourceView;
    chk << device->CreateShaderResourceView(resource.Get(), &desc, &shaderResourceView);
    return shaderResourceView;
}

ComPtr<ID3D11UnorderedAccessView> Device::CreateUAV(const ComPtr<ID3D11Resource>& resource)
{
    ComPtr<ID3D11UnorderedAccessView> unorderedAccessView;
    chk << device->CreateUnorderedAccessView(resource.Get(), nullptr, &unorderedAccessView);
    return unorderedAccessView;
}

ComPtr<ID3D11UnorderedAccessView> Device::CreateUAV(const ComPtr<ID3D11Resource>& resource,
                                                    const D3D11_UNORDERED_ACCESS_VIEW_DESC& desc)
{
    ComPtr<ID3D11UnorderedAccessView> unorderedAccessView;
    chk << device->CreateUnorderedAccessView(resource.Get(), &desc, &unorderedAccessView);
    return unorderedAccessView;
}

ComPtr<ID3D11DepthStencilView> Device::CreateDSV(const ComPtr<ID3D11Resource>& resource)
{
    ComPtr<ID3D11DepthStencilView> depthStencilView;
    chk << device->CreateDepthStencilView(resource.Get(), nullptr, &depthStencilView);
    return depthStencilView;
}

ComPtr<ID3D11DepthStencilView> Device::CreateDSV(const ComPtr<ID3D11Resource>& resource,
                                                 const D3D11_DEPTH_STENCIL_VIEW_DESC& desc)
{
    ComPtr<ID3D11DepthStencilView> depthStencilView;
    chk << device->CreateDepthStencilView(resource.Get(), &desc, &depthStencilView);
    return depthStencilView;
}

template <>
void Device::SetIndexBuffer<uint32_t>(const IndexBuffer<uint32_t>& indexBuffer, uint32_t offsetFromStart)
{
    deviceContext->IASetIndexBuffer(indexBuffer.rawBuffer.Get(), DXGI_FORMAT::DXGI_FORMAT_R32_UINT, offsetFromStart);
}

template <>
void Device::SetIndexBuffer<uint16_t>(const IndexBuffer<uint16_t>& indexBuffer, uint32_t offsetFromStart)
{
    deviceContext->IASetIndexBuffer(indexBuffer.rawBuffer.Get(), DXGI_FORMAT::DXGI_FORMAT_R16_UINT, offsetFromStart);
}