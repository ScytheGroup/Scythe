export module Graphics:MappedResource;
import Common;

export class Device;

export class MappedResource
{
    D3D11_MAPPED_SUBRESOURCE mappedSubresource{};
    ComPtr<ID3D11Resource> resource;
    ComPtr<ID3D11DeviceContext> deviceContext;
    uint32_t subresourceIndex;
public:
    MappedResource(const ComPtr<ID3D11Resource>& resource, uint32_t subresourceIndex, const ComPtr<ID3D11DeviceContext>& deviceContext);
    MappedResource(MappedResource&&) noexcept;
    auto& GetData() { return mappedSubresource; }
    const auto& GetData() const { return mappedSubresource; }
    ~MappedResource();
};