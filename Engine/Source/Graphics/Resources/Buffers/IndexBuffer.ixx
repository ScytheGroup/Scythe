export module Graphics:IndexBuffer;

import :Device;

import Common;

D3D11_BUFFER_DESC GetIndexBufferDesc(uint32_t indexCount, uint32_t stride)
{
    return { .ByteWidth = indexCount * stride,
             .Usage = D3D11_USAGE_IMMUTABLE,
             .BindFlags = D3D11_BIND_INDEX_BUFFER,
             .CPUAccessFlags = 0,
             .MiscFlags = 0,
             .StructureByteStride = stride };
}

export template <typename T>
    requires (std::is_same_v<T, uint16_t> or std::is_same_v<T, uint32_t>) // This requires currently doesn't compile with modules :( sad
class IndexBuffer
{
public:
    IndexBuffer() = default;
    IndexBuffer(Device& device, const ViewSpan<T>& initialData)
        : rawBuffer(device.CreateBuffer(GetIndexBufferDesc(initialData.count, sizeof(T)), initialData.data))
    {}
    IndexBuffer(IndexBuffer&&) noexcept = default;
    IndexBuffer& operator=(IndexBuffer&&) noexcept = default;

    IndexBuffer(const IndexBuffer&) = delete;
    IndexBuffer& operator=(const IndexBuffer&) = delete;
    ~IndexBuffer() = default;

    bool IsSet() const noexcept { return rawBuffer != nullptr; }

private:
    ComPtr<ID3D11Buffer> rawBuffer;

    friend class Device;
};