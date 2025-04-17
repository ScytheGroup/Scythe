export module Graphics:Buffer;

import :Device;
import Common;

export template<typename T>
class Buffer
{
public:
    Buffer(Buffer&&) noexcept = default;
    Buffer& operator=(Buffer&&) noexcept = default;

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    virtual ~Buffer() = default;

    bool IsSet() const noexcept { return rawBuffer != nullptr; }
    bool IsDynamic() const noexcept { return isDynamic; }

    const ComPtr<ID3D11Buffer>& GetRawBuffer() const noexcept { return rawBuffer; }
    
protected:
    Buffer(const ComPtr<ID3D11Buffer>& rawBuffer, bool isDynamic)
        : rawBuffer(rawBuffer)
        , isDynamic(isDynamic)
    {}

    void UpdateInternal(Device& device, const ViewSpan<T>& dataViewSpan, D3D11_BUFFER_DESC&& bufferDesc);

    ComPtr<ID3D11Buffer> rawBuffer;
    bool isDynamic;

    friend class Device;
};

template <typename T>
void Buffer<T>::UpdateInternal(Device& device, const ViewSpan<T>& dataViewSpan, D3D11_BUFFER_DESC&& bufferDesc)
{
    if (!isDynamic)
    {
        DebugPrint("Updating the data of a non-dynamic buffer is not recommended, only do this if the update is a rare operation. If you want to update data each frame, use a dynamic buffer instead.");
    }

    if (!IsSet())
    {
        rawBuffer = device.CreateBuffer(bufferDesc, dataViewSpan.data);
    }
    else
    {
        device.UploadDataToBuffer(*this, dataViewSpan);
    }
}