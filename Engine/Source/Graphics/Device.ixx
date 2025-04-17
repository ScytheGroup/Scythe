export module Graphics:Device;

import :GraphicsDefs;
import :MappedResource;

import Common;

struct InputLayoutDescription;
class GeometryShader;
struct IReadableResource;
struct IReadWriteResource;

export class DepthStencil;
export class DepthStencilState;
export class FrameBuffer;
export class Texture;
export struct InputLayout;
export class ComputeShader;
export class VertexShader;
export class PixelShader;
export struct ShadingModel;
export class BlendState;
export class RasterizerState;

template <typename T> class VertexBuffer;
template <typename T> requires (std::is_same_v<T, uint16_t> or std::is_same_v<T, uint32_t>) class IndexBuffer;
template <typename T> class Buffer;
template <typename T> class ConstantBuffer;

export class Device
{
public:
    Device(HWND window, uint32_t width, uint32_t height);

    void ClearRTV(const Color& clearColor, const ComPtr<ID3D11RenderTargetView>& renderTarget);
    void ClearUAV(const ComPtr<ID3D11UnorderedAccessView>& uav, uint32_t value);
    void ClearDepthStencilView(const ComPtr<ID3D11DepthStencilView>& depthStencilView);

    void UnsetRenderTargets();
    void SetRenderTargets(const std::vector<ComPtr<ID3D11RenderTargetView>>& renderTargets);
    void SetRenderTargetsAndDepthStencilView(const std::vector<ComPtr<ID3D11RenderTargetView>>& renderTargets, const ComPtr<ID3D11DepthStencilView>& depthStencilTarget);
    void SetRenderTargetsAndUnorderedAccessViews(const std::vector<ComPtr<ID3D11RenderTargetView>>& renderTargets, const ComPtr<ID3D11DepthStencilView>& depthStencilTarget, const std::vector<IReadWriteResource*>& readWriteResources);
    void SetDepthStencilState(const DepthStencilState&  depthStencilState);
    void UnsetDepthStencilState();
    
    void SetRasterizerState(const RasterizerState& rasterizerState);
    void SetRasterizerState(const ComPtr<ID3D11RasterizerState>& rasterizerState);
    
    void SetSamplers(uint32_t startSlot, ViewSpan<ComPtr<ID3D11SamplerState>> samplersViewSpan, PipelineStageFlags stageFlags);
    void SetViewports(const std::vector<D3D11_VIEWPORT>& viewportSpan);
    
    void SetBlendState(const BlendState& blendState);
    void DisableBlending();
    
public:
    void Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ);
    void Present(bool activateVsync);
    
    void Draw(uint32_t vertexCount, uint32_t startVertexLocation);
    void DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation = 0, uint32_t startInstanceLocation = 0);
    void DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, uint32_t baseVertexLocation);

    ComPtr<ID3D11Texture2D> GetBackBuffer() const;

    void Flush();
    void OnResize(uint32_t width, uint32_t height);

public:
    void SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY primitiveTopology);
    void SetVertexShader(VertexShader& vertexShader);
    void UnsetVertexShader();
    
    void SetPixelShader(PixelShader& pixelShader);
    void UnsetPixelShader();
    
    void SetGeometryShader(GeometryShader& geometryShader);
    void UnsetGeometryShader();
    
    void SetComputeShader(ComputeShader& computeShader);
    void CopyTexture(Texture& destTexture, const Texture& sourceTexture);
    void SetFrameBuffer(FrameBuffer& framebuffer);
    void SetFrameBuffer(FrameBuffer& framebuffer, const DepthStencil& depthOverride);
    void SetFrameBufferNoDepth(FrameBuffer& framebuffer);

    void SetShadingModel(const ShadingModel& shaderModel);
    void CleanupDraw();

    template <typename T>
    void SetVertexBuffer(uint32_t startSlot, ViewSpan<VertexBuffer<T>> vertexBuffersViewSpan, uint32_t offset = 0);

    void UnsetVertexBuffer(uint32_t startSlot = 0, uint32_t count = 1);
    void UnsetIndexBuffer();

    // Templated function only has two implementations: uint16_t and uint32_t.
    // This matches the two only formats that IASetIndexBuffer accepts: https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-iasetindexbuffer
    template <typename T>
    void SetIndexBuffer(const IndexBuffer<T>& indexBuffer, uint32_t offsetFromStart = 0);

    template <typename T>
    void SetConstantBuffers(uint32_t startSlot, ViewSpan<ConstantBuffer<T>> constantBuffersViewSpan, PipelineStageFlags stageFlags);
    void UnsetConstantBuffers(uint32_t startSlot, PipelineStageFlags stageFlags);

    void SetInputLayout(const ComPtr<ID3D11InputLayout>& inputLayout);

    void UnsetShaderResource(uint32_t slot, PipelineStageFlags stageFlags);
    void SetShaderResource(uint32_t slot, const IReadableResource& texture, PipelineStageFlags stageFlags);
    void SetShaderResources(uint32_t slot, const ViewSpan<IReadableResource*>& texture, PipelineStageFlags stageFlags);
    
    void SetUnorderedAccessView(uint32_t slot, const IReadWriteResource& uav);
    void UnsetUnorderedAccessView(uint32_t slot);
    
    MappedResource MapTexture(Texture& texture, uint32_t subresourceIndex = 0);

public:
    ComPtr<ID3D11RenderTargetView> CreateRTV(const ComPtr<ID3D11Resource>& resource);
    ComPtr<ID3D11RenderTargetView> CreateRTV(const ComPtr<ID3D11Resource>& resource,
                                             const D3D11_RENDER_TARGET_VIEW_DESC& desc);

    ComPtr<ID3D11ShaderResourceView> CreateSRV(const ComPtr<ID3D11Resource>& resource);
    ComPtr<ID3D11ShaderResourceView> CreateSRV(const ComPtr<ID3D11Resource>& resource,
                                               const D3D11_SHADER_RESOURCE_VIEW_DESC& desc);

    ComPtr<ID3D11UnorderedAccessView> CreateUAV(const ComPtr<ID3D11Resource>& resource);
    ComPtr<ID3D11UnorderedAccessView> CreateUAV(const ComPtr<ID3D11Resource>& resource,
                                                const D3D11_UNORDERED_ACCESS_VIEW_DESC& desc);

    ComPtr<ID3D11DepthStencilView> CreateDSV(const ComPtr<ID3D11Resource>& resource);
    ComPtr<ID3D11DepthStencilView> CreateDSV(const ComPtr<ID3D11Resource>& resource,
                                             const D3D11_DEPTH_STENCIL_VIEW_DESC& desc);

    ComPtr<ID3D11Buffer> CreateBuffer(const D3D11_BUFFER_DESC& bufferDesc, const void* initialData = nullptr);
    ComPtr<ID3D11VertexShader> CreateVertexShader(const ComPtr<ID3DBlob>& vertexShaderBytecode);
    ComPtr<ID3D11PixelShader> CreatePixelShader(const ComPtr<ID3DBlob>& pixelShaderBytecode);
    ComPtr<ID3D11ComputeShader> CreateComputeShader(const ComPtr<ID3DBlob>& computeShaderBytecode);
    ComPtr<ID3D11GeometryShader> CreateGeometryShader(const ComPtr<ID3DBlob>& geometryShaderBytecode);

    ComPtr<ID3D11Texture2D> CreateTexture2D(const D3D11_TEXTURE2D_DESC& textureDesc,
                                            D3D11_SUBRESOURCE_DATA* initialData = nullptr);
    ComPtr<ID3D11Texture3D> CreateTexture3D(const D3D11_TEXTURE3D_DESC& textureDesc,
                                            D3D11_SUBRESOURCE_DATA* initialData = nullptr);

    ComPtr<ID3D11DepthStencilState> CreateDepthStencilState(const D3D11_DEPTH_STENCIL_DESC& desc);

    ComPtr<ID3D11InputLayout> CreateInputLayout(const VertexShader& shader, const InputLayoutDescription& input);
    ComPtr<ID3D11RasterizerState> CreateRasterizerState(const D3D11_RASTERIZER_DESC& desc);

    ComPtr<ID3D11SamplerState> CreateSamplerState(const D3D11_SAMPLER_DESC& desc);
    ComPtr<ID3D11BlendState> CreateBlendState(const D3D11_BLEND_DESC& desc);

public:
    template <typename T>
    void UploadDataToBuffer(const Buffer<T>& buffer, const ViewSpan<T>& dataViewSpan);

    void UpdateSubresource(const ComPtr<ID3D11Resource>& destResource, const D3D11_SUBRESOURCE_DATA& srcData);

    void CopySubresource(const ComPtr<ID3D11Resource>& destResource, uint32_t dstMipLevel, const ComPtr<ID3D11Resource>& srcResource, uint32_t srcMipLevel);
    void CopyResource(const ComPtr<ID3D11Resource>& destResource, const ComPtr<ID3D11Resource>& srcResource);

    void Map(const ComPtr<ID3D11Resource>& resource, uint32_t subresourceIndex, D3D11_MAP mapType, D3D11_MAPPED_SUBRESOURCE& mappedSubresource);
    void Unmap(const ComPtr<ID3D11Resource>& resource, uint32_t subresourceIndex);

    void GenerateMips(const ComPtr<ID3D11ShaderResourceView>& srv)
    {
        deviceContext->GenerateMips(srv.Get());
    }

private:
    void CreateDevice();
    void CreateSwapchain(HWND window, uint32_t width, uint32_t height);

    ComPtr<ID3D11Device> device = nullptr;
    ComPtr<IDXGIFactory7> dxgiFactory = nullptr;
    ComPtr<ID3D11DeviceContext> deviceContext = nullptr;
    ComPtr<IDXGISwapChain1> swapChain = nullptr;

    static constexpr int MaxVertexBufferSlots = 32;
    static constexpr int MaxConstantBufferSlots = 14;
    static constexpr int MaxSamplerSlots = 16;

    // Used to use WIC texture import library
    friend class Texture;
    friend class Editor;
#ifdef IMGUI_ENABLED
    friend class ImGuiManager;
#endif

#ifdef RENDERDOC
    friend class RenderDoc;
#endif

#ifdef PROFILING
    friend class Profiler;
    friend class GPUScope;
#endif
};

template <typename T>
void Device::SetVertexBuffer(uint32_t startSlot, ViewSpan<VertexBuffer<T>> vertexBuffersViewSpan,
                             uint32_t offset)
{
    uint32_t numElements = vertexBuffersViewSpan.count;

    if (numElements > MaxVertexBufferSlots - startSlot)
    {
        DebugPrintError(
            "Too many vertex buffers passed to SetVertexBuffer (startSlot: {}), max amount authorized is {}.",
            startSlot, MaxVertexBufferSlots);
    }

    using VertexBufferT = std::array<ID3D11Buffer*, MaxVertexBufferSlots>;
    VertexBufferT rawBuffers{ nullptr };
    std::ranges::transform(vertexBuffersViewSpan, std::begin(rawBuffers),
                           [](VertexBuffer<T>& buffer) -> ID3D11Buffer* { return buffer.rawBuffer.Get(); });

    deviceContext->IASetVertexBuffers(startSlot, numElements, rawBuffers.data(), &VertexBuffer<T>::Stride, &offset);
}

template <typename T>
void Device::SetConstantBuffers(uint32_t startSlot, ViewSpan<ConstantBuffer<T>> constantBuffersViewSpan,
                                PipelineStageFlags stageFlags)
{
    uint32_t numElements = constantBuffersViewSpan.count;

    if (numElements > MaxConstantBufferSlots - startSlot)
    {
        DebugPrintError(
            "Too many constant buffers passed to SetConstantBuffer (startSlot: {}), max amount authorized is {}.",
            startSlot, MaxConstantBufferSlots);
    }

    using ConstantBufferT = std::array<ID3D11Buffer*, MaxConstantBufferSlots>;
    ConstantBufferT rawBuffers{ nullptr };
    std::ranges::transform(constantBuffersViewSpan, std::begin(rawBuffers),
                           [](ConstantBuffer<T>& buffer) -> ID3D11Buffer* { return buffer.rawBuffer.Get(); });

    if (stageFlags & VERTEX_SHADER)
        deviceContext->VSSetConstantBuffers(startSlot, numElements, rawBuffers.data());

    if (stageFlags & PIXEL_SHADER)
        deviceContext->PSSetConstantBuffers(startSlot, numElements, rawBuffers.data());

    if (stageFlags & COMPUTE_SHADER)
        deviceContext->CSSetConstantBuffers(startSlot, numElements, rawBuffers.data());

    if (stageFlags & GEOMETRY_SHADER)
        deviceContext->GSSetConstantBuffers(startSlot, numElements, rawBuffers.data());
}

ComPtr<ID3D11Buffer> Device::CreateBuffer(const D3D11_BUFFER_DESC& bufferDesc, const void* initialData)
{
    D3D11_SUBRESOURCE_DATA data;
    if (initialData != nullptr)
    {
        data.pSysMem = initialData;
        data.SysMemPitch = 0;
        data.SysMemSlicePitch = 0;
    }
    ComPtr<ID3D11Buffer> buffer;
    chk << device->CreateBuffer(&bufferDesc, initialData == nullptr ? nullptr : &data, buffer.GetAddressOf());
    return buffer;
}

template <typename T>
void Device::UploadDataToBuffer(const Buffer<T>& buffer, const ViewSpan<T>& dataViewSpan)
{
    if (buffer.IsDynamic())
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource{};
        chk << deviceContext->Map(buffer.rawBuffer.Get(), 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        memcpy(mappedResource.pData, dataViewSpan.data, sizeof(T) * dataViewSpan.count);
        deviceContext->Unmap(buffer.rawBuffer.Get(), 0);
    }
    else
    {
        deviceContext->UpdateSubresource(buffer.rawBuffer.Get(), 0, nullptr, dataViewSpan.data, sizeof(T) * dataViewSpan.count, 0);
    }
}