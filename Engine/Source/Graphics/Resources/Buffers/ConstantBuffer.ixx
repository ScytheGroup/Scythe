export module Graphics:ConstantBuffer;

import :Device;
import :Buffer;

import Common;

constexpr D3D11_BUFFER_DESC GetConstantBufferDesc(uint32_t cbufferSize, bool isDynamic)
{
    return { .ByteWidth = RaiseToNextMultipleOf(cbufferSize, 16),
             .Usage = isDynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT,
             .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
             .CPUAccessFlags = isDynamic ? D3D11_CPU_ACCESS_WRITE : static_cast<uint32_t>(0),
             .MiscFlags = 0 };
}

export template<typename T>
class ConstantBuffer : public Buffer<T>
{
public:
    ConstantBuffer();
    ConstantBuffer(Device& device, const T& initialData, bool isBufferDynamic = false);
    ConstantBuffer(Device& device, bool isBufferDynamic = false);

    ConstantBuffer(ConstantBuffer&&) noexcept = default;
    ConstantBuffer& operator=(ConstantBuffer&&) noexcept = default;

    ConstantBuffer(const ConstantBuffer&) = delete;
    ConstantBuffer& operator=(const ConstantBuffer&) = delete;

    ~ConstantBuffer() override = default;

    void Update(Device& device, const T& newData);

private:
    friend class Device;
};

template <typename T>
ConstantBuffer<T>::ConstantBuffer()
    : Buffer<T>(nullptr, true)
{}

template <typename T>
ConstantBuffer<T>::ConstantBuffer(Device& device, const T& initialData, bool isBufferDynamic)
    : Buffer<T>(device.CreateBuffer(GetConstantBufferDesc(sizeof(T), isBufferDynamic), &initialData), isBufferDynamic)
{}

template <typename T>
ConstantBuffer<T>::ConstantBuffer(Device& device, bool isBufferDynamic)
    : Buffer<T>(device.CreateBuffer(GetConstantBufferDesc(sizeof(T), isBufferDynamic), nullptr), isBufferDynamic)
{}

template <typename T>
void ConstantBuffer<T>::Update(Device& device, const T& newData)
{
    Buffer<T>::UpdateInternal(device, ViewSpan(&newData), GetConstantBufferDesc(sizeof(T), Buffer<T>::isDynamic));
}