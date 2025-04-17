export module Graphics:VertexBuffer;

import :Device;

import Common;

constexpr D3D11_BUFFER_DESC GetVertexBufferDesc(uint32_t vertexCount, uint32_t stride)
{
    return { .ByteWidth = vertexCount * stride,
             .Usage = D3D11_USAGE_IMMUTABLE,
             .BindFlags = D3D11_BIND_VERTEX_BUFFER,
             .CPUAccessFlags = 0,
             .MiscFlags = 0,    
             .StructureByteStride = stride };
}

export template <typename T>
class VertexBuffer
{
public:
    static constexpr uint32_t Stride = sizeof(T);

    VertexBuffer() = default;
    VertexBuffer(VertexBuffer&&) noexcept = default;
    VertexBuffer& operator=(VertexBuffer&&) noexcept = default;
    VertexBuffer(Device& device, const std::vector<T>& initialData) :
        rawBuffer(device.CreateBuffer(GetVertexBufferDesc(static_cast<uint32_t>(initialData.size()), sizeof(T)), initialData.data()))
    {}
    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;
    ~VertexBuffer() = default;

    bool IsSet() const noexcept { return rawBuffer != nullptr; }
    
private:
    ComPtr<ID3D11Buffer> rawBuffer;

    friend class Device;
};