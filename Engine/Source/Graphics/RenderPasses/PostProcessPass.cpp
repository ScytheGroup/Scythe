module;
#include "../Utils/EventScopeMacros.h"
module Graphics:RenderPass;

import :BlendState;
import :RasterizerState;
import :ComputeShaderHelper;
import :Scene;

import Graphics;
import Core;

void PostProcessPass::ApplyVignette(GraphicsScene& scene)
{
    SC_SCOPED_GPU_EVENT_FUNC();

    scene.device.SetFrameBuffer(scene.sceneColorBuffer);
    scene.device.SetRasterizerState(RasterizerStates::Get<RasterizerPresets::BackfaceCulling>(scene.device));
    
    ShaderKey psKey;
    psKey.filepath = "DummyVignette.ps.hlsl";
    PixelShader ps = *scene.resourceManager.shaderLoader.Load<PixelShader>(std::move(psKey), scene.device);
    
    scene.device.SetBlendState(BlendStates::Get<BlendPresets::Alpha>(scene.device));
    Graphics::DrawFullscreenTriangle(scene.device, ps);
    scene.device.SetBlendState(BlendStates::Get<BlendPresets::Opaque>(scene.device));
}

void PostProcessPass::ApplyBlur(GraphicsScene& scene)
{
    static ConstantBuffer<int> blurConstantBuffer;
    static Texture blurSourceTexture = scene.sceneColorBuffer.renderTextures[0].CreateCopy(scene.device);
    
    SC_SCOPED_GPU_EVENT_FUNC();

    Texture& resultTexture = scene.sceneColorBuffer.renderTextures[0];
    
    if (blurSourceTexture.GetWidth() != resultTexture.GetWidth() || blurSourceTexture.GetWidth() != resultTexture.GetHeight())
    {
        blurSourceTexture.Resize(resultTexture.GetWidth(), resultTexture.GetHeight(), scene.device);   
    }

    scene.device.CopyTexture(blurSourceTexture, resultTexture);
    
    ShaderKey key;
    key.filepath = "Compute/Blur.cs.hlsl";
    ComputeShader* computeShader = ResourceManager::Get().shaderLoader.Load<ComputeShader>(std::move(key), scene.device);
    
    scene.device.SetComputeShader(*computeShader);

    scene.device.SetShaderResource(0, blurSourceTexture, COMPUTE_SHADER);
    scene.device.SetUnorderedAccessView(0, scene.sceneColorBuffer.renderTextures[0]);
    
    blurConstantBuffer.Update(scene.device, settings.blurKernelSize);
    scene.device.SetConstantBuffers(0, ViewSpan{ &blurConstantBuffer }, COMPUTE_SHADER);
    
    auto [x, y, z] = GetThreadCountsForComputeSize(scene.sceneColorBuffer.GetSize(), { 8, 8 });

    scene.device.Dispatch(x, y, z);

    scene.device.UnsetShaderResource(0, COMPUTE_SHADER);
    scene.device.UnsetUnorderedAccessView(0);
}

void PostProcessPass::ApplyToneMapping(GraphicsScene& scene)
{
    SC_SCOPED_GPU_EVENT_FUNC();

    ShaderKey acesToneMapKey;
    acesToneMapKey.filepath = "Compute/ToneMapping.cs.hlsl";
    acesToneMapKey.defines = { { "TONE_MAPPER", "ACES"} };
    ComputeShader* toneMappingShader = scene.resourceManager.shaderLoader.Load<ComputeShader>(std::move(acesToneMapKey), scene.device);

    ShaderKey rToneMapKey;
    rToneMapKey.filepath = "Compute/ToneMapping.cs.hlsl";
    rToneMapKey.defines = { { "TONE_MAPPER", "REINHARD" } };
    ComputeShader* reinhardToneMapper = scene.resourceManager.shaderLoader.Load<ComputeShader>(std::move(rToneMapKey), scene.device);

    ShaderKey rjToneMapKey;
    rjToneMapKey.filepath = "Compute/ToneMapping.cs.hlsl";
    rjToneMapKey.defines = { { "TONE_MAPPER", "REINHARD_JODIE" } };
    ComputeShader* reinhardJodieToneMapper = scene.resourceManager.shaderLoader.Load<ComputeShader>(std::move(rjToneMapKey), scene.device);

    scene.device.UnsetRenderTargets();
    scene.device.SetUnorderedAccessView(0, scene.sceneColorBuffer.renderTextures[0]);

    auto [x,y,z] = GetThreadCountsForComputeSize(scene.sceneColorBuffer.GetSize(), { 8,8 });

    ComputeShader* toneMappers[] { toneMappingShader, reinhardToneMapper, reinhardJodieToneMapper };

    scene.device.SetComputeShader(*toneMappers[static_cast<int>(settings.toneMappingType)]);
    scene.device.Dispatch(x, y, z);

    scene.device.UnsetUnorderedAccessView(0);
}

void PostProcessPass::ApplyCustomPostProcessPasses(GraphicsScene& scene, const SceneSettings& sceneSettings)
{
    SC_SCOPED_GPU_EVENT_FUNC();

    static Texture SourceTexture = scene.sceneColorBuffer.renderTextures[0].CreateCopy(scene.device);
    Texture& resultTexture = scene.sceneColorBuffer.renderTextures[0];
    
    if (SourceTexture.GetWidth() != resultTexture.GetWidth() || SourceTexture.GetWidth() != resultTexture.GetHeight())
    {
        SourceTexture.Resize(resultTexture.GetWidth(), resultTexture.GetHeight(), scene.device);
    }

    scene.device.UnsetRenderTargets();
    scene.device.UnsetDepthStencilState();
    scene.device.SetUnorderedAccessView(0, scene.sceneColorBuffer.renderTextures[0]);
    scene.device.SetConstantBuffers(Graphics::FrameCBufferIndex, ViewSpan{ &scene.frameDataBuffer }, COMPUTE_SHADER);

    static constexpr uint32_t SceneColorSlot = 0;
    static constexpr uint32_t DepthSlot = 1;
    static constexpr uint32_t OutputSlot = 0;

    for (auto& [passName, passInfo] : sceneSettings.graphics.GetSortedPostProcessPasses())
    {
        ShaderKey csKey;
        csKey.filepath = passInfo->passPath;
        csKey.absolutePath = true;
        ComputeShader* cs = scene.resourceManager.shaderLoader.Load<ComputeShader>(std::move(csKey), scene.device);
        if (!cs->IsValid())
        { 
            // Force recompile each time. Will make error messages appear immediately (and only require recompiling this singular shader).
            auto errMsg = cs->RecompileShader(scene.device);
            if (errMsg.has_value())
            {
                scene.resourceManager.shaderLoader.SubmitFailedCompilation(cs, errMsg.value_or("Unspecified error."));
                continue;
            }
        }

        if (!passInfo->enabled) continue;

        auto [x, y, z] = GetThreadCountsForComputeSize(scene.sceneColorBuffer.GetSize(), { 8, 8 });

        scene.device.CopyTexture(SourceTexture, resultTexture);

        scene.device.SetComputeShader(*cs);
        scene.device.SetShaderResource(SceneColorSlot, SourceTexture, COMPUTE_SHADER);
        scene.device.SetShaderResource(DepthSlot, scene.gBuffer.depthStencil, COMPUTE_SHADER);
        scene.device.SetUnorderedAccessView(OutputSlot, resultTexture);
        scene.device.Dispatch(x, y, z);
        scene.device.UnsetShaderResource(DepthSlot, COMPUTE_SHADER);
    }

    scene.device.UnsetUnorderedAccessView(OutputSlot);
}

void PostProcessPass::Render(GraphicsScene& scene, const SceneSettings& sceneSettings)
{
    if (!enabled)
        return;
    
    SC_SCOPED_GPU_EVENT("Post-Process");
    
    ApplyCustomPostProcessPasses(scene, sceneSettings);

    if (settings.vignetteEnabled)
        ApplyVignette(scene);

    if (settings.toneMappingEnabled)
        ApplyToneMapping(scene);

    if (settings.blurEnabled)
        ApplyBlur(scene);
}

#ifdef IMGUI_ENABLED

import ImGui;

bool PostProcessPass::ImGuiDraw()
{
    bool result = false;
    ScytheGui::Header("Post process settings");
    
    result |= ImGui::Checkbox("Use post process", &enabled);
    ScytheGui::IfEnabled(enabled, [&]
    {
        result |= ImGui::Checkbox("Blur", &settings.blurEnabled);
        ScytheGui::IfEnabled(settings.blurEnabled, [&]
        {
            result |= ImGui::InputInt("Kernel size", &settings.blurKernelSize);
        });
        ImGui::Separator();
        
        result |= ImGui::Checkbox("Vignette", &settings.vignetteEnabled);
        ScytheGui::IfEnabled(settings.vignetteEnabled, [&]
        {
            
        });
        ImGui::Separator();

        result |= ImGui::Checkbox("Tone mapping", &settings.toneMappingEnabled);
        ScytheGui::IfEnabled(settings.toneMappingEnabled, [&]
        {
            ScytheGui::ComboEnum("Tone mapper type", settings.toneMappingType);
        });
    });

    return result;
}

#endif