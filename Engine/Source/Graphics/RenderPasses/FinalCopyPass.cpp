module;
#include "../Utils/EventScopeMacros.h"
module Graphics:RenderPass;

import Graphics;
import :RasterizerState;
import :Scene;
import :FrameBuffer;

void FinalCopyPass::Render(GraphicsScene& scene)
{
    SC_SCOPED_GPU_EVENT("Final Copy");
    
    FrameBuffer* finalCopyBuffer = &scene.backBuffer;
#ifdef EDITOR
    finalCopyBuffer = &scene.editorBuffer;
#endif
    
    ShaderKey copyKey;
    copyKey.filepath = "CopyPass.ps.hlsl";

    copyKey.defines = 
    {
#ifdef IMGUI_ENABLED
        // This makes it so we do a manual conversion to SRGB to allow
        // for proper tone mapped render on back buffer
        // This is due to ImGui not correctly handling srgb buffers.
        { "COPY_TO_IMGUI", "1" },
#endif
        { "FXAA", useFXAA ? "1" : "0" }
    };

    scene.device.SetRasterizerState(RasterizerStates::Get<RasterizerPresets::BackfaceCulling>(scene.device));
    scene.device.SetFrameBuffer(*finalCopyBuffer);
    
    PixelShader* copyShader = ResourceManager::Get().shaderLoader.Load<PixelShader>(std::move(copyKey), scene.device);

    scene.device.SetConstantBuffers(Graphics::FrameCBufferIndex, ViewSpan{ &scene.frameDataBuffer }, PIXEL_SHADER);

    scene.device.SetShaderResource(0, scene.sceneColorBuffer.renderTextures[0], PIXEL_SHADER);
    
    Graphics::DrawFullscreenTriangle(scene.device, *copyShader);
}

#ifdef IMGUI_ENABLED

bool FinalCopyPass::ImGuiDraw()
{
    ImGui::Checkbox("FXAA", &useFXAA);
    return true;
}

#endif