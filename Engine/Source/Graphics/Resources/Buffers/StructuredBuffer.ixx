export module Graphics:StructuredBuffer;

import :Device;
import :Interfaces;
import :Buffer;

import Common;

template <typename T>
constexpr D3D11_BUFFER_DESC GetStructuredBufferDesc(uint32_t numElements, bool isReadWrite, bool isDynamic)
{
    uint32_t cpuAccessFlags = 0;
    uint32_t bindFlags = D3D11_BIND_SHADER_RESOURCE;
    if (isReadWrite)
        bindFlags |= D3D11_BIND_UNORDERED_ACCESS;

    if (isDynamic)
        cpuAccessFlags |= D3D11_CPU_ACCESS_WRITE;
    else if (isReadWrite)
        cpuAccessFlags |= D3D11_CPU_ACCESS_READ;
    
    return
    {
        .ByteWidth = numElements * sizeof(T),
        .Usage = isDynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT,
        .BindFlags = bindFlags,
        .CPUAccessFlags = cpuAccessFlags,
        .MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
        .StructureByteStride = sizeof(T)
    };
}

constexpr D3D11_SHADER_RESOURCE_VIEW_DESC GetSRVDesc(uint32_t numElements)
{
    return {
        .Format = DXGI_FORMAT_UNKNOWN,
        .ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX,
        .BufferEx = { .FirstElement = 0, .NumElements = numElements, .Flags = 0 }
    };
}

// Structured buffers should be used for arrays of resources whose size is unknown at compile-time.
// For more invariant data, a ConstantBuffer should be used instead.
export template<typename T>
class StructuredBuffer : public Buffer<T>, public IReadableResource, public IReadWriteResource
{
    ComPtr<ID3D11UnorderedAccessView> CreateUAV(Device& device, uint32_t nElems);
public:
    StructuredBuffer();
    StructuredBuffer(StructuredBuffer&&) = default;
    StructuredBuffer& operator =(StructuredBuffer&&) = default;
    StructuredBuffer(Device& device, std::vector<T>& initialData, bool isReadWrite = false, bool isBufferDynamic = false);
    StructuredBuffer(Device& device, bool isReadWrite = false, bool isBufferDynamic = false);

    ~StructuredBuffer() override = default;

    void Update(Device& device, std::vector<T>& newData);

public:
    const ComPtr<ID3D11ShaderResourceView>& GetSRV() const override { return srv; }
    const ComPtr<ID3D11UnorderedAccessView>& GetUAV() const override { return uav; }
    
private:
    uint32_t size{};
    ComPtr<ID3D11ShaderResourceView> srv{};
    ComPtr<ID3D11UnorderedAccessView> uav{};
    
    friend class Device;
};

template <typename T>
ComPtr<ID3D11UnorderedAccessView> StructuredBuffer<T>::CreateUAV(Device& device, uint32_t nElems)
{
    D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    desc.Buffer.NumElements = nElems;
    desc.Buffer.FirstElement = 0;
    desc.Buffer.Flags = 0;
    return device.CreateUAV(Buffer<T>::rawBuffer.Get(), desc);
}

template <typename T>
StructuredBuffer<T>::StructuredBuffer()
    : Buffer<T>(nullptr, true)
{}

template <typename T>
StructuredBuffer<T>::StructuredBuffer(Device& device, std::vector<T>& initialData, bool isReadWrite, bool isBufferDynamic)
    : Buffer<T>(device.CreateBuffer(GetStructuredBufferDesc<T>(static_cast<uint32_t>(initialData.size()), isReadWrite, isBufferDynamic), initialData.data()), isBufferDynamic)
    , srv(device.CreateSRV(Buffer<T>::rawBuffer.Get(), GetSRVDesc(static_cast<uint32_t>(initialData.size()))))
    , uav{isReadWrite ? CreateUAV(device, static_cast<uint32_t>(initialData.size())) : nullptr}
{
    size = static_cast<uint32_t>(initialData.size());
}

template <typename T>
StructuredBuffer<T>::StructuredBuffer(Device& device, bool isReadWrite, bool isBufferDynamic)
    : Buffer<T>(device.CreateBuffer(GetStructuredBufferDesc<T>(0, isReadWrite, isBufferDynamic), nullptr), isBufferDynamic)
    , srv(device.CreateSRV(Buffer<T>::rawBuffer.Get(), GetSRVDesc(0)))
    , uav{isReadWrite ? CreateUAV(device, 0) : nullptr}
{}

template <typename T>
void StructuredBuffer<T>::Update(Device& device, std::vector<T>& newData)
{
    uint32_t newSize = static_cast<uint32_t>(newData.size());

    if (size != newSize)
    {
        // Force buffer recreate for resize
        Buffer<T>::rawBuffer = nullptr;
    }
    
    Buffer<T>::UpdateInternal(device, ViewSpan<T>(newData.data(), newSize), GetStructuredBufferDesc<T>(static_cast<uint32_t>(newData.size()), !!uav, Buffer<T>::isDynamic));

    if (size != newSize)
    {
        srv = device.CreateSRV(Buffer<T>::rawBuffer.Get(), GetSRVDesc(newSize));

        if (uav)
            uav = CreateUAV(device, newSize);    
    }

    size = newSize;
}
