export module Graphics:Interfaces;

import Common;

export struct IReadableResource
{
    virtual const ComPtr<ID3D11ShaderResourceView>& GetSRV() const = 0;
    virtual ~IReadableResource() {}
};

export struct IReadWriteResource
{
    virtual const ComPtr<ID3D11UnorderedAccessView>& GetUAV() const = 0;
    virtual ~IReadWriteResource() {}
};
