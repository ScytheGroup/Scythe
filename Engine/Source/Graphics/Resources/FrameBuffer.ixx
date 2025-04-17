export module Graphics:FrameBuffer;

import Common;
import :Texture;
import :DepthStencil;

export class FrameBuffer
{
public:
    FrameBuffer(const std::string& viewportName, Texture&& texture, Device& device, const Color& clearColor = Colors::Black);
    FrameBuffer(const std::string& viewportName, std::vector<Texture>&& textures, Device& device, const Color& clearColor = Colors::Black);

    std::vector<ComPtr<ID3D11RenderTargetView>> GetRTVs() const;
    std::vector<ComPtr<ID3D11ShaderResourceView>> GetSRVs() const;
    bool ImGuiDraw();

    FrameBuffer(const FrameBuffer& other) = delete;
    FrameBuffer& operator=(const FrameBuffer& other) = delete;
    FrameBuffer(FrameBuffer&& other) = default;
    FrameBuffer& operator=(FrameBuffer&& other) = default;

    void Resize(uint32_t width, uint32_t height, Device& device);
    void Clear(Device& device);
    Vector2 GetSize() const noexcept;
    
    std::string name;
    
    D3D11_VIEWPORT viewport{};
    std::vector<Texture> renderTextures;
    DepthStencil depthStencil;
    Color clearColor;
private:

    static DepthStencil CreateDepthBufferFromTexture2D(const Texture& texture, Device& device);

#ifdef IMGUI_ENABLED
    std::vector<std::array<bool, 4>> masks;
#endif
};