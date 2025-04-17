export module Graphics:DepthStencil;

import Common;
import :Device;
import :Interfaces;

// TODO: add stencil support in this class if/when the need arises!
export class DepthStencil : public IReadableResource
{
public:
    DepthStencil(Device& device, uint32_t width, uint32_t height, uint32_t arraySize = 1, bool cubeMap = false);

    uint32_t GetArraySize() const noexcept { return arraySize; };
    const ComPtr<ID3D11DepthStencilView>& GetDSV(uint32_t index = 0) const
    {
        Assert(index < dsvs.size());
        return dsvs[index];
    };
    const ComPtr<ID3D11ShaderResourceView>& GetSRV() const override { return srv; };
protected:
    void GenerateViews(Device& device);
    
private:
    uint32_t arraySize = 1;
    bool cubeMap = false;
    
    ComPtr<ID3D11Texture2D> rawTexture;
    std::vector<ComPtr<ID3D11DepthStencilView>> dsvs{};
    ComPtr<ID3D11ShaderResourceView> srv{};
    ComPtr<ID3D11DepthStencilState> depthStencilState;

    friend class Graphics;
};
