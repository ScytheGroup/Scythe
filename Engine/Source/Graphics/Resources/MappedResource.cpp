module;
#include <d3d11.h>

module Graphics:MappedResource;

import :MappedResource;

MappedResource::MappedResource(const ComPtr<ID3D11Resource>& resource, uint32_t subresourceIndex, const ComPtr<ID3D11DeviceContext>& deviceContext)
    : resource{resource}
    , subresourceIndex{subresourceIndex}
    , deviceContext{deviceContext}
{
    chk << deviceContext->Map(resource.Get(), subresourceIndex, D3D11_MAP_READ, 0, &mappedSubresource);
}

MappedResource::MappedResource(MappedResource&& other) noexcept
{
    mappedSubresource = std::move(other.mappedSubresource);
    resource = std::exchange(other.resource, nullptr);
    deviceContext = std::exchange(other.deviceContext, nullptr);
    subresourceIndex = other.subresourceIndex;
}

MappedResource::~MappedResource()
{
    if (resource && deviceContext)
        deviceContext->Unmap(resource.Get(), subresourceIndex);
}
