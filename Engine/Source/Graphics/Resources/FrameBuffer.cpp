module Graphics:FrameBuffer;

import :FrameBuffer;

FrameBuffer::FrameBuffer(const std::string& viewportName, Texture&& texture, Device& device, const Color& clearColor)
    : name{ viewportName }
    , depthStencil{ CreateDepthBufferFromTexture2D(texture, device) }
    , clearColor{clearColor}
{
    renderTextures.push_back(std::move(texture));
    Texture& firstTex = renderTextures[0];
    viewport.Width = static_cast<FLOAT>(firstTex.GetWidth());
    viewport.Height = static_cast<FLOAT>(firstTex.GetHeight());
    viewport.MaxDepth = 1.0f;
}

FrameBuffer::FrameBuffer(const std::string& viewportName, std::vector<Texture>&& textures, Device& device, const Color& clearColor)
    : name{ viewportName }
    , renderTextures(std::move(textures))
    , depthStencil{ CreateDepthBufferFromTexture2D(renderTextures[0], device) }
    , clearColor{clearColor}
{
    Texture& firstTex = renderTextures[0];
    viewport.Width = static_cast<FLOAT>(firstTex.GetWidth());
    viewport.Height = static_cast<FLOAT>(firstTex.GetHeight());
    viewport.MaxDepth = 1.0f;
}

Vector2 FrameBuffer::GetSize() const noexcept { return { viewport.Width, viewport.Height }; }

std::vector<ComPtr<ID3D11RenderTargetView>> FrameBuffer::GetRTVs() const
{
    std::vector<ComPtr<ID3D11RenderTargetView>> RenderTargetViews{renderTextures.size()};
    std::ranges::transform(renderTextures, RenderTargetViews.begin(), [](auto&& texture){ return texture.GetRTV(); });
    return RenderTargetViews;
}

std::vector<ComPtr<ID3D11ShaderResourceView>> FrameBuffer::GetSRVs() const
{
    std::vector<ComPtr<ID3D11ShaderResourceView>> RenderTargetViews{renderTextures.size()};
    std::ranges::transform(renderTextures, RenderTargetViews.begin(), [](auto&& texture){ return texture.GetSRV(); });
    return RenderTargetViews;
}


void FrameBuffer::Resize(uint32_t width, uint32_t height, Device& device)
{
    for (auto&& texture : renderTextures)
        texture.Resize(width, height, device);

    depthStencil = DepthStencil(device, width, height);
    viewport.Width = static_cast<FLOAT>(width);
    viewport.Height = static_cast<FLOAT>(height);
    viewport.MaxDepth = 1.0f;
}

void FrameBuffer::Clear(Device& device)
{
    for (auto&& texture : renderTextures)
    {
        device.ClearRTV(clearColor, texture.GetRTV());    
    }
    device.ClearDepthStencilView(depthStencil.GetDSV());
}

DepthStencil FrameBuffer::CreateDepthBufferFromTexture2D(const Texture& texture, Device& device)
{
    return { device, texture.GetWidth(), texture.GetHeight() };
}

#if defined IMGUI_ENABLED

import ImGui;

bool FrameBuffer::ImGuiDraw()
{
    bool result = false;
    if (masks.size() != renderTextures.size())
        masks.resize(renderTextures.size(), { true, true, true, true });
    
    ScytheGui::Header(name.c_str(), ImGui::headerFont);

    float width = ImGui::GetColumnWidth();

    float imageMaxWidth = 250.0f;
    
    ImGui::BeginTable("table", static_cast<int>(std::max(1.0f, width/imageMaxWidth)));
    ImGui::TableNextRow();
    
    float currentWidth = 0;
    for (int i = 0; i < renderTextures.size(); ++i)
    {
        ImGui::TableNextColumn();

        ImGui::PushID(i);
    
        ImGui::Text(renderTextures[i].GetName().data());
        ImGui::Text("Mask: ");
        ImGui::SameLine();
        result |= ImGui::Checkbox("R", &masks[i][0]);
        ImGui::SameLine();
        result |= ImGui::Checkbox("G", &masks[i][1]);
        ImGui::SameLine();
        result |= ImGui::Checkbox("B", &masks[i][2]);
        ImGui::SameLine();
        result |= ImGui::Checkbox("A", &masks[i][3]);
       
        ScytheGui::MaskedImage(renderTextures[i], ImVec2{ imageMaxWidth, imageMaxWidth }, masks[i].data());
        
        ImGui::PopID();
        
        if (currentWidth > width)
        {
            ImGui::TableNextRow();
            currentWidth = 0;
        }
        currentWidth += imageMaxWidth;
    }
    ImGui::EndTable();
    return result;
}

#endif