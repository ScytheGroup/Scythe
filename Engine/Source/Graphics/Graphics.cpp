module;

#include <d3d11.h>

#include "Common/Macros.hpp"
#include "Utils/EventScopeMacros.h"
module Graphics;

import :GraphicsDefs;
import :Resources;
import :InputLayout;
import :Material;
import :TextureHelpers;
import :ComputeShaderHelper;

import :DepthStencilState;
import :RasterizerState;
import :BlendState;

import Common;
import Core;
import Components;

#ifdef IMGUI_ENABLED
import ImGui;
#ifdef EDITOR
import Editor;
#endif
#endif

Graphics::Graphics(HWND window, uint32_t width, uint32_t height)
    : device(window, width, height)
    , resourceManager{device}
    , window(window)
    , scene{
        .device = device,
        .resourceManager = resourceManager,
        .viewportSize = Vector2{static_cast<float>(width), static_cast<float>(height)},
#ifdef EDITOR
        .editorBuffer = FrameBuffer{"EditorViewport", { Texture::Description::CreateTexture2D("EditorViewport", width, height, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, Texture::TEXTURE_SRV | Texture::TEXTURE_RTV, false), device }, device, ClearColor},
#endif
        .backBuffer = FrameBuffer{BackBufferName, Texture("Backbuffer", device.GetBackBuffer(),
#ifdef IMGUI_ENABLED
        DXGI_FORMAT_R8G8B8A8_UNORM, 
#else
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
#endif
        Texture::TEXTURE_RTV, device), device, ClearColor},
        .gBuffer = CreateGBuffer(width, height, device),
        .sceneColorBuffer = FrameBuffer("PostProcess", { Texture::Description::CreateTexture2D("PostProcess", width, height, DXGI_FORMAT_R16G16B16A16_FLOAT, Texture::TEXTURE_SRV | Texture::TEXTURE_RTV | Texture::TEXTURE_UAV, false), device }, device, ClearColor)  
    }
#ifdef PROFILING
    , profiler(device)
#endif
{
#ifdef PROFILING
    // Allows for seeing initial startup times
    // TODO: could allow for a more granular view by adding same scope logic as the general profiler. This requires a refactor of scope logic though.
    SC_INIT_START();
#endif
#ifdef IMGUI_ENABLED
    ImGuiManager::Get().InitFromGraphics(*this);
#endif
    device.SetFrameBuffer(scene.backBuffer);
    
    CreateSamplerStates();
    
    device.SetRasterizerState(RasterizerStates::Get<RasterizerPresets::BackfaceCulling>(device));
    device.SetDepthStencilState(DepthStencilStates::Get<DepthStencilPresets::LessEqualWriteAll>(device));
    device.SetBlendState(BlendStates::Get<BlendPresets::Opaque>(device));
    
    IMGUI_REGISTER_WINDOW("Graphics", EDIT, Graphics::DrawImGuiWindow);

#ifdef EDITOR
    IMGUI_REGISTER_COMMAND("Generate Reflections", SCENE, Graphics::GenerateReflections);

    playInEditorDelegate = EditorDelegates::PlayInEditorDelegate.Subscribe([this](bool isEntering) 
		{
			if (scene.invalidateIBL) 
			{ 
				GenerateReflections();
			}
		});
#endif
}

Graphics::~Graphics()
{
#ifdef EDITOR
    EditorDelegates::PlayInEditorDelegate.Remove(playInEditorDelegate);
#endif
}

void Graphics::DrawMesh(Device& device, const Transform& transform, Mesh& mesh, const SubMesh& subMesh, std::unordered_map<uint32_t, AssetRef<Material>>& materialOverrides)
{
    Material* material = mesh.GetMaterial(subMesh.materialIndex, materialOverrides);
    if (!material || !material->IsLoaded())
        return;
    
    DrawMesh(device, transform, mesh, subMesh, *material);
}

void Graphics::DrawMesh(Device& device, const Transform& transform, Mesh& mesh, const SubMesh& subMesh, const Material& material)
{
    if (material.shadingModelName != ShadingModelStore::NONE)
    {
        const ShadingModel& shadingModel = *ResourceManager::Get().shadingModelStore.GetShadingModel(material.shadingModelName, &material);
        DrawMesh(device, transform, mesh, subMesh, material, shadingModel);
        return;
    }
    
    material.constantBuffer.Update(device, material.data);
    device.SetConstantBuffers(MaterialCBufferIndex, ViewSpan{ &material.constantBuffer }, PIXEL_SHADER);

    DrawGeometry(device, transform, *mesh.geometry, subMesh.startIndex, subMesh.indexCount);
}

void Graphics::DrawMesh(Device& device, const Transform& transform, Mesh& mesh, const SubMesh& subMesh, const Material& material, const ShadingModel& shadingModel)
{
    material.constantBuffer.Update(device, material.data);
    device.SetConstantBuffers(MaterialCBufferIndex, ViewSpan{ &material.constantBuffer }, PIXEL_SHADER);

    device.SetShadingModel(shadingModel);

    if (material.shadingModelName != ShadingModelStore::CUSTOM)
    {
        static std::unique_ptr<Texture> defaultARM = std::make_unique<Texture>(CreateUniformColorTextureData(Color(1, 0.5, 0), 1, 1, Texture::TEXTURE_2D, Texture::DEFAULT, Texture::TEXTURE_SRV));
        static std::unique_ptr<Texture> defaultNormal = std::make_unique<Texture>(CreateUniformColorTextureData(Color(0.5, 0.5, 1), 1, 1, Texture::TEXTURE_2D, Texture::DEFAULT, Texture::TEXTURE_SRV));
        
        Texture* defaultDiffuse = AssetRef<Texture>{ "Missing"s }.GetLoadedAsync();
        auto getTextureOrFallback = [](const AssetRef<Texture>& refTexture, Texture* fallback) -> Texture*
        {
            if (Texture* tex = refTexture.GetLoadedAsync())
                return tex;
            return fallback;
        };

        if (material.diffuseTexture.HasId())
            if (Texture* tex = getTextureOrFallback(material.diffuseTexture, defaultDiffuse))
                device.SetShaderResource(shadingModel.pixelShader->GetTextureSlot("DiffuseTexture"), *tex, PIXEL_SHADER);    

        if (material.normalTexture.HasId())
            if (Texture* tex = getTextureOrFallback(material.normalTexture, defaultNormal.get()))
                device.SetShaderResource(shadingModel.pixelShader->GetTextureSlot("NormalTexture"), *tex, PIXEL_SHADER);    

        if (material.armTexture.HasId())
            if (Texture* tex = getTextureOrFallback(material.armTexture, defaultARM.get()))
                device.SetShaderResource(shadingModel.pixelShader->GetTextureSlot("ARMTexture"), *tex, PIXEL_SHADER);    
    }

    DrawGeometry(device, transform, *mesh.geometry, subMesh.startIndex, subMesh.indexCount);
}

void Graphics::DrawGeometry(Device& device, const Transform& transform, Geometry& geometry, int startIndex, int count)
{
    if (!geometry.IsLoaded())
        return;
    
    device.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    Matrix modelMatrix = transform.ToMatrixWithScale();
    Geometry::objectDataBuffer.Update(device, { modelMatrix, modelMatrix.Invert().Transpose() });
    device.SetConstantBuffers(ModelCBufferIndex, ViewSpan{ &Geometry::objectDataBuffer }, VERTEX_SHADER);
    
    device.SetVertexBuffer<GenericVertex>(0, &geometry.vertexBuffer);
    device.SetIndexBuffer(geometry.indexBuffer);
    device.DrawIndexed(count, startIndex, 0);
    device.CleanupDraw();
}

void Graphics::DrawGeometry(Device& device, const Transform& transform, Geometry& geometry)
{
    DrawGeometry(device, transform, geometry, 0, geometry.indexCount);
}

void Graphics::DrawFullscreenTriangle(Device& device, PixelShader& pixelShader)
{
    device.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    ShaderKey vsKey;
    vsKey.filepath = "FullscreenPass.vs.hlsl";
    device.SetVertexShader(*ResourceManager::Get().shaderLoader.Load<VertexShader>(std::move(vsKey), device));
    device.SetPixelShader(pixelShader);
    device.Draw(3, 0);
}

void Graphics::DrawCopyPass(Texture& renderTexture)
{
    ShaderKey copyKey;
    copyKey.filepath = "CopyPass.ps.hlsl";
    PixelShader* copyShader = resourceManager.shaderLoader.Load<PixelShader>(std::move(copyKey), device);

    device.SetShaderResource(0, renderTexture, PIXEL_SHADER);
    
    DrawFullscreenTriangle(device, *copyShader);
}

void Graphics::DrawSprite(Device& device, Texture& spriteTexture, Transform& transform)
{
    SC_SCOPED_GPU_EVENT("Graphics::DrawSprite");
    device.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ResourceManager& resourceManager = ResourceManager::Get();

    Matrix modelMatrix = transform.ToMatrixWithScale();
    ObjectData data{ modelMatrix };
    ConstantBuffer constantBuffer{ device, data };
    device.SetConstantBuffers(ModelCBufferIndex, ViewSpan{ &constantBuffer }, VERTEX_SHADER);

    ShaderKey vsKey;
    vsKey.filepath = "Sprite.vs.hlsl";
    device.SetVertexShader(*resourceManager.shaderLoader.Load<VertexShader>(std::move(vsKey), device));

    ShaderKey psKey;
    psKey.filepath = "Sprite.ps.hlsl";
    PixelShader* pixelShader = resourceManager.shaderLoader.Load<PixelShader>(std::move(psKey), device);
    device.SetPixelShader(*pixelShader);

    device.SetShaderResource(pixelShader->GetTextureSlot("SpriteTexture"), spriteTexture, PIXEL_SHADER);
    device.Draw(6, 0);
}

void Graphics::DownloadResource(const Texture& tex)
{
    tex.data.data = TextureHelpers::DownloadTexture2DMip0(tex, device);
}

void Graphics::ReloadTexture(const Texture& tex, const Texture::TextureData& data)
{
    tex.~Texture();
    new (const_cast<Texture*>(&tex)) Texture(data, device);
}

std::optional<Handle> Graphics::GetSelectedEntityFromScreen(const Vector2& mousePosition, EntityComponentSystem& ecsRef)
{
    if (mousePosition.y < 0 || mousePosition.y > scene.viewportSize.y || mousePosition.x < 0 || mousePosition.x > scene.viewportSize.x)
        return {};
    
    DirectX::XMUINT2 intSize = {
        static_cast<uint32_t>(scene.viewportSize.x),
        static_cast<uint32_t>(scene.viewportSize.y)
    };

    static ConstantBuffer<uint32_t> objectIdBuffer;

    Texture::Description desc{
        .name = "Staging Click Detection",
        .width = intSize.x,
        .height = intSize.y,
        .format = DXGI_FORMAT_R32_UINT,
        .type = Texture::TEXTURE_2D,
        .usage = Texture::STAGING
    };
    Texture stagingClickDetectionTexture{ desc, device };

    Texture::Description clickDesc = desc;
    clickDesc.usage = Texture::DEFAULT;
    clickDesc.bindFlags = Texture::TEXTURE_SRV | Texture::TEXTURE_RTV | Texture::TEXTURE_UAV;
    FrameBuffer clickDetectionBuffer = FrameBuffer{"Object Id Framebuffer", { clickDesc, device }, device };
    
    ShaderKey vsKey;
    vsKey.filepath = "Mesh.vs.hlsl";
    VertexShader* vertexShader = resourceManager.shaderLoader.Load<VertexShader>(std::move(vsKey), device);

    ShaderKey psKey;
    psKey.filepath = "ObjectIdDetector.ps.hlsl";
    PixelShader* pixelShader = resourceManager.shaderLoader.Load<PixelShader>(std::move(psKey), device);

    device.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    clickDetectionBuffer.Clear(device);
    device.UnsetDepthStencilState();
    device.SetRenderTargets(clickDetectionBuffer.GetRTVs());
    
    device.SetViewports({ clickDetectionBuffer.viewport });

    device.DisableBlending();

    ShaderKey fullscreenPsKey;
    fullscreenPsKey.filepath = "ObjectIdDetector.ps.hlsl";
    fullscreenPsKey.defines = { { "FULLSCREEN_PASS", "1" } };
    PixelShader* fullscreenPixelShader = resourceManager.shaderLoader.Load<PixelShader>(std::move(fullscreenPsKey), device);
    
    objectIdBuffer.Update(device, std::numeric_limits<uint32_t>::max());
    device.SetConstantBuffers(0, ViewSpan{ &objectIdBuffer }, PIXEL_SHADER);
    DrawFullscreenTriangle(device, *fullscreenPixelShader);

    device.SetRenderTargetsAndDepthStencilView(clickDetectionBuffer.GetRTVs(), clickDetectionBuffer.depthStencil.GetDSV());
    
    device.SetVertexShader(*vertexShader);
    device.SetPixelShader(*pixelShader);

    const Array<std::tuple<GlobalTransform*, MeshComponent*>>& drawnObjects = ecsRef.GetArchetype<GlobalTransform, MeshComponent>();
    for (auto [transform, meshInstance] : drawnObjects)
    {
        if (!meshInstance->mesh)
            continue;
        
        const Transform& meshTransform = transform->transform;
        Mesh* mesh = meshInstance->mesh;

        objectIdBuffer.Update(device, transform->GetOwningEntity());
        device.SetConstantBuffers(0, ViewSpan{ &objectIdBuffer }, PIXEL_SHADER);

        DrawGeometry(device, meshTransform, *mesh->geometry);
    }
    
#ifdef EDITOR
    // Draw editor sprites but for selection logic
    ShaderKey spriteVsKey;
    spriteVsKey.filepath = "Sprite.vs.hlsl";
    VertexShader* spriteVertexShader = resourceManager.shaderLoader.Load<VertexShader>(std::move(spriteVsKey), device);
    device.SetVertexShader(*spriteVertexShader);

    ShaderKey spritePsKey;
    spritePsKey.filepath = "ObjectIdDetector.ps.hlsl";
    spritePsKey.defines = { { "SPRITE", "1" } };
    PixelShader* spritePixelShader = resourceManager.shaderLoader.Load<PixelShader>(std::move(spritePsKey), device);
    device.SetPixelShader(*spritePixelShader);

    if (EditorIcons::AreSpriteIconsLoaded())
    {
        static ConstantBuffer constantBuffer{ device, ObjectData{} };

        // Lights
        {
            Array<std::tuple<PunctualLightComponent*, GlobalTransform*>> punctualLightComponents = ecsRef.GetArchetype<PunctualLightComponent, GlobalTransform>();
            CameraComponent* mainCamera = ecsRef.GetSystem<CameraSystem>().GetActiveCamera();
            GlobalTransform* cameraTransform = ecsRef.GetComponent<GlobalTransform>(mainCamera->GetOwningEntity());

            auto orderedLights = SortLightSpritesBackToFront(CameraSystem::GetViewMatrix(*cameraTransform), punctualLightComponents);

            for (auto [punctualLightComponent, globalTransform] : orderedLights)
            {
                Transform spriteTransform = *globalTransform;
                if (mainCamera->projection.type == CameraProjection::ProjectionType::PERSPECTIVE)
                {
                    spriteTransform =
                      MakeTransformScreenSpaceSizedBillboard(spriteTransform, cameraTransform->position, mainCamera->projection.FOV, scene.frameData.screenSize / 40.0f, scene.frameData.screenSize);
                }
                else { spriteTransform = MakeTransformBillboard(spriteTransform, cameraTransform->position); }

                auto id = globalTransform->GetOwningEntity().id;
                objectIdBuffer.Update(device, id);
                device.SetConstantBuffers(spritePixelShader->GetConstantBufferSlot("ObjectId"), ViewSpan{ &objectIdBuffer }, PIXEL_SHADER);
                if (punctualLightComponent->type == PunctualLightComponent::LightType::POINT)
                {
                    device.SetShaderResource(spritePixelShader->GetTextureSlot("SpriteTexture"), *EditorIcons::PointLightIcon, PIXEL_SHADER);
                }
                else if (punctualLightComponent->type == PunctualLightComponent::LightType::SPOT)
                {
                    device.SetShaderResource(spritePixelShader->GetTextureSlot("SpriteTexture"), *EditorIcons::SpotLightIcon, PIXEL_SHADER);
                }
                else { continue; }

                device.SetConstantBuffers(FrameCBufferIndex, ViewSpan{ &scene.frameDataBuffer }, VERTEX_SHADER);

                Matrix modelMatrix = spriteTransform.ToMatrixWithScale();
                ObjectData data{ modelMatrix };
                constantBuffer.Update(device, data);
                device.SetConstantBuffers(ModelCBufferIndex, ViewSpan{ &constantBuffer }, VERTEX_SHADER);

                device.Draw(6, 0);
            }
        }

        // Cameras
        {
            Array<std::tuple<CameraComponent*, GlobalTransform*>> cameras = ecsRef.GetArchetype<CameraComponent, GlobalTransform>();
            CameraComponent* mainCamera = ecsRef.GetSystem<CameraSystem>().GetActiveCamera();
            GlobalTransform* cameraTransform = ecsRef.GetComponent<GlobalTransform>(mainCamera->GetOwningEntity());

            auto orderedCameras = SortCameraSpritesBackToFront(CameraSystem::GetViewMatrix(*cameraTransform), cameras);

            for (auto [camera, globalTransform] : orderedCameras)
            {
                if (camera == mainCamera) continue;

                Transform spriteTransform = *globalTransform;
                if (mainCamera->projection.type == CameraProjection::ProjectionType::PERSPECTIVE)
                {
                    spriteTransform =
                      MakeTransformScreenSpaceSizedBillboard(spriteTransform, cameraTransform->position, mainCamera->projection.FOV, scene.frameData.screenSize / 40.0f, scene.frameData.screenSize);
                }
                else { spriteTransform = MakeTransformBillboard(spriteTransform, cameraTransform->position); }

                auto id = globalTransform->GetOwningEntity().id;
                objectIdBuffer.Update(device, id);
                device.SetConstantBuffers(spritePixelShader->GetConstantBufferSlot("ObjectId"), ViewSpan{ &objectIdBuffer }, PIXEL_SHADER);
                device.SetShaderResource(spritePixelShader->GetTextureSlot("SpriteTexture"), *EditorIcons::CameraIcon, PIXEL_SHADER);

                device.SetConstantBuffers(FrameCBufferIndex, ViewSpan{ &scene.frameDataBuffer }, VERTEX_SHADER);

                Matrix modelMatrix = spriteTransform.ToMatrixWithScale();
                ObjectData data{ modelMatrix };
                constantBuffer.Update(device, data);
                device.SetConstantBuffers(ModelCBufferIndex, ViewSpan{ &constantBuffer }, VERTEX_SHADER);

                device.Draw(6, 0);
            }
        }
    }
#endif

    device.CopyTexture(stagingClickDetectionTexture, clickDetectionBuffer.renderTextures[0]);

    MappedResource textureMapping = device.MapTexture(stagingClickDetectionTexture);

    uint8_t* data = static_cast<uint8_t*>(textureMapping.GetData().pData);

    int pos = static_cast<int>(textureMapping.GetData().RowPitch * mousePosition.y + mousePosition.x * sizeof(uint32_t));
    uint32_t objectId = *reinterpret_cast<uint32_t*>(data + pos);
    if (objectId == std::numeric_limits<uint32_t>::max())
        return {};
    
    return Handle(objectId);
}

void Graphics::ResizeViewport(uint32_t width, uint32_t height)
{
    width = std::max(static_cast<uint32_t>(MinViewportSize.x), width);
    height = std::max(static_cast<uint32_t>(MinViewportSize.y), height);

    scene.viewportSize = Vector2{ static_cast<float>(width), static_cast<float>(height) };
    
    scene.gBuffer.Resize(width, height, device);
    scene.sceneColorBuffer.Resize(width, height, device);
#ifdef EDITOR
    scene.editorBuffer.Resize(width, height, device);
#endif
}

void Graphics::InitializeComponent(MeshComponent& meshComp)
{
    if (meshComp.isInitialized)
        return;

    meshComp.isInitialized = true;
    ResourceManager::Get().meshLoader.FillAsset(meshComp.mesh);
    if (meshComp.mesh)
        meshComp.mesh->RegisterLazyLoadedResource(false);
    for (auto& mat : meshComp.materialOverrides | std::views::values)
    {
        if (Material* m = mat.GetLoadedAsync())
        {
            m->RegisterLazyLoadedResource(true);
        }
    }
}

void Graphics::GenerateReflections()
{
    auto& ecsRef = GetEcs();
    deferredLightingPass.RegenerateIBL(scene, skyboxPass.GetSkybox() ? &skyboxPass.GetSkybox()->texture : nullptr, atmospherePass, ecsRef.GetSceneSettings());
}

void Graphics::BindSamplers()
{
    device.SetSamplers(0, { samplerStates.data(), static_cast<uint32_t>(samplerStates.size()) }, VERTEX_SHADER | PIXEL_SHADER | COMPUTE_SHADER);
}

void Graphics::UpdateGraphicsScene(double delta, EntityComponentSystem& ecs)
{
    CameraComponent* mainCamera = ecs.GetSystem<CameraSystem>().GetActiveCamera();
    GlobalTransform* cameraTransform = ecs.GetComponent<GlobalTransform>(mainCamera->GetOwningEntity());

    if (mainCamera)
    {
        GlobalTransform* cameraTransform = ecs.GetComponent<GlobalTransform>(mainCamera->GetOwningEntity());
        mainCamera->projection.SetAspectRatio(scene.viewportSize.x, scene.viewportSize.y);
        scene.frameData.UpdateView(cameraTransform->transform, mainCamera->projection);
    }
    
    scene.frameData.screenSize = scene.viewportSize;
    scene.frameData.frameTime = static_cast<float>(delta);
    scene.frameData.elapsedTime += static_cast<float>(delta);
    
    scene.frameDataBuffer.Update(device, scene.frameData);
    device.SetConstantBuffers(FrameCBufferIndex, ViewSpan{ &scene.frameDataBuffer }, VERTEX_SHADER | PIXEL_SHADER | COMPUTE_SHADER | GEOMETRY_SHADER);

    // This could be moved to GraphicsScene to allow for use elsewhere in the engine
    std::vector<PunctualLightData> lightShaderData;
    Array<std::tuple<PunctualLightComponent*, GlobalTransform*>> punctualLightComponents = ecs.GetArchetype<PunctualLightComponent, GlobalTransform>();
    for (auto [punctualLightComponent, GlobalTransform] : punctualLightComponents)
    {
        Assert(punctualLightComponent && GlobalTransform);
        
        PunctualLightData lightData;
        lightData.type = static_cast<PunctualLightData::PunctualLightType>(std::to_underlying(punctualLightComponent->type));
        lightData.position = GlobalTransform->position;
        lightData.color = punctualLightComponent->color;
        lightData.intensity = punctualLightComponent->intensity;
        // The sign of the falloff determines if we use frostbite's inverse sq attenuation or khronos attenuation.
        lightData.falloff = 1 / std::max(FloatSmallNumber, punctualLightComponent->attenuationRadius * punctualLightComponent->attenuationRadius);
        if (punctualLightComponent->useInverseSquaredFalloff)
        {
            lightData.falloff *= -1;
        }

        if (lightData.type == PunctualLightData::PunctualLightType::SPOT) 
        {
            lightData.direction = Vector3::Transform(Vector3::Forward, GlobalTransform->rotation);
            
            // Convert from logical editor values to optimized shader values
            float cosOuter = std::cosf(punctualLightComponent->outerAngle * 0.5f);
            float cosInner = std::cosf(punctualLightComponent->innerAngle * 0.5f);
            float lightAngleScale = 1.0f / std::max(FloatSmallNumber , cosInner - cosOuter);
            float lightAngleOffset = -cosOuter * lightAngleScale;
            lightData.angleScaleOffset = Vector2{ lightAngleScale, lightAngleOffset };
        }

        lightShaderData.emplace_back(lightData);
    }

    if (!lightShaderData.empty())
    {
        scene.lightDataBuffer.Update(device, lightShaderData);
        device.SetShaderResource(PunctualLightSRVIndex, scene.lightDataBuffer, VERTEX_SHADER | PIXEL_SHADER | COMPUTE_SHADER | GEOMETRY_SHADER);    
    }
    
    // The frame data can be bound only once per frame!
    auto& graphicsSettings = ecs.GetSceneSettings().graphics;
    scene.frameData.directionalLight.color = graphicsSettings.directionalLight.color;
    scene.frameData.directionalLight.intensity = graphicsSettings.directionalLight.intensity;
    scene.frameData.directionalLight.direction = graphicsSettings.directionalLight.direction;

    scene.frameData.punctualLightsCount = static_cast<uint32_t>(lightShaderData.size());
}

void Graphics::BeginRender()
{
    SC_SCOPED_EVENT("Graphics::BeginRender");
    SC_SCOPED_GPU_EVENT("Graphics::BeginRender");
    device.UnsetRenderTargets();

#ifdef EDITOR
    auto viewportSize = editor->viewportRect.GetSize();
    if (Convert(viewportSize) != scene.viewportSize)
    {
        ResizeViewport(
            static_cast<uint32_t>(viewportSize.x),
            static_cast<uint32_t>(viewportSize.y)
        );
    }

    editor->UpdateSelection();
#endif

    resourceManager.shaderLoader.RecompileDirty(device);
    
    if (!resourceManager.resourcesToLazyLoad.empty())
    {
        resourceManager.LazyLoadResources(device);
    }

    scene.gBuffer.Clear(device);
    scene.sceneColorBuffer.Clear(device);
#ifdef EDITOR
    scene.editorBuffer.Clear(device);
#endif 
    scene.backBuffer.Clear(device);
    
    device.SetBlendState(BlendStates::Get<BlendPresets::Opaque>(device));
    device.SetRasterizerState(RasterizerStates::Get<RasterizerPresets::BackfaceCulling>(device));
    device.SetDepthStencilState(DepthStencilStates::Get<DepthStencilPresets::LessEqualWriteAll>(device));
    BindSamplers();
}

void Graphics::Init()
{
    auto& ecs = GetEcs();

    for (MeshComponent* meshComp : ecs.GetComponentsOfType<MeshComponent>())
    {
        InitializeComponent(*meshComp);
    }
    
    BindSamplers();

    skyboxPass.Init(device, ecs.GetSceneSettings());
    shadowPass.Init(device);
#if EDITOR
    debugLinesPass.Init(device);
#endif
    atmospherePass.Init(device, ecs.GetSceneSettings());
    deferredLightingPass.Init(device, skyboxPass.GetSkybox(), atmospherePass, ecs.GetSceneSettings());
#if VXGI_ENABLED
    voxelizationPass.Init(device);
#endif
}

void Graphics::Update(double deltaTime)
{
    auto& ecsRef = GetEcs();

    SC_SCOPED_EVENT("Graphics::Update");

#if PROFILING
    profiler.Update(static_cast<float>(deltaTime));
#endif
    
#ifdef RENDERDOC
    auto tracker = ecsRef.GetSystem<InputSystem>().GetKeyboardTracker();

    // Useful hotkeys for capturing frames:
    // Ctrl + R to open RenderDoc while closing all already opened instances
    // Ctrl + Shift + R to open RenderDoc without touching other opened instances
    if (tracker.GetLastState().LeftControl && tracker.IsKeyPressed(DirectX::Keyboard::R))
    {
        if (renderDoc)
            renderDoc->PerformCapture(device, !tracker.GetLastState().LeftShift);
    }
#endif

    BeginRender();

    UpdateGraphicsScene(deltaTime, ecsRef);

    shadowPass.Render(scene, ecsRef);

#if VXGI_ENABLED
    if (ecsRef.GetSceneSettings().graphics.enableVoxelGI)
    {
        voxelizationPass.Render(scene, ecsRef, shadowPass.sceneShadowData);    
    }
#ifdef EDITOR 
    if (!voxelizationPass.showVoxels)
#endif
#endif
    {
        meshPass.Render(scene, ecsRef);

        deferredLightingPass.Render(scene, shadowPass.sceneShadowData, 
#if VXGI_ENABLED
            voxelizationPass.GetIlluminationVolume(), 
#endif
        ecsRef.GetSceneSettings(), atmospherePass, skyboxPass.GetSkybox() ? &skyboxPass.GetSkybox()->texture : nullptr);    
    }
    
    skyboxPass.Render(scene, ecsRef.GetSceneSettings());

    atmospherePass.Render(scene, ecsRef.GetSceneSettings());
    
    postProcessPass.Render(scene, ecsRef.GetSceneSettings());

#ifdef EDITOR
    debugLinesPass.Render(scene, ecsRef);
#endif
    
    finalCopyPass.Render(scene);

#ifdef IMGUI_ENABLED
    imguiPass.Render(scene, resourceManager.shaderLoader);
#endif

    EndRender();
}

void Graphics::EndRender()
{
    SC_SCOPED_EVENT("Graphics::EndRender");
    SC_SCOPED_GPU_EVENT("Graphics::EndRender");

#if RENDERDOC
    if (renderDoc)
        renderDoc->UpdateEndFrame(device);
#endif

    device.Present(activateVsync);
}

void Graphics::OnResize(uint32_t width, uint32_t height)
{
    device.Flush();
    
    auto format = scene.backBuffer.renderTextures[0].GetFormat();
    
    scene.backBuffer.~FrameBuffer();

    device.OnResize(width, height);
    scene.viewportSize = Vector2{ static_cast<float>(width), static_cast<float>(height) };

    new (&scene.backBuffer) FrameBuffer("GameViewport", Texture("Backbuffer", device.GetBackBuffer(), format, Texture::TEXTURE_RTV, device), device);

    if (!editor) {
        scene.gBuffer.Resize(width, height, device);
        scene.sceneColorBuffer.Resize(width, height, device);
    }
}

void Graphics::SetEditor(Editor& newEditor)
{
    editor = &newEditor;
#ifdef IMGUI_ENABLED
    imguiPass.Init(editor);
#endif
}

void Graphics::CreateSamplerStates()
{
    // Point sampler
    {
        D3D11_SAMPLER_DESC pointSamplerDesc;
        pointSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        pointSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        pointSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        pointSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        pointSamplerDesc.MipLODBias = 0;
        pointSamplerDesc.MinLOD = 0;
        pointSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        pointSamplerDesc.MaxAnisotropy = 0;
        samplerStates.push_back(device.CreateSamplerState(pointSamplerDesc));
    }

    // Linear sampler
    {
        D3D11_SAMPLER_DESC linearSamplerDesc;
        linearSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        linearSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        linearSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        linearSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        linearSamplerDesc.MipLODBias = 0;
        linearSamplerDesc.MinLOD = 0;
        linearSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        linearSamplerDesc.MaxAnisotropy = 0;
        samplerStates.push_back(device.CreateSamplerState(linearSamplerDesc));
    }

    // Anisotropic sampler
    {
        D3D11_SAMPLER_DESC anisotropicSamplerDesc;
        anisotropicSamplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
        anisotropicSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        anisotropicSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        anisotropicSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        anisotropicSamplerDesc.MipLODBias = 0;
        anisotropicSamplerDesc.MinLOD = 0;
        anisotropicSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        anisotropicSamplerDesc.MaxAnisotropy = 16;
        samplerStates.push_back(device.CreateSamplerState(anisotropicSamplerDesc));
    }

    // Shadow map sampler
    {
        D3D11_SAMPLER_DESC shadowMapSampler{};
        shadowMapSampler.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        shadowMapSampler.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        shadowMapSampler.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        shadowMapSampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        shadowMapSampler.ComparisonFunc = D3D11_COMPARISON_LESS;
        shadowMapSampler.MaxLOD = D3D11_FLOAT32_MAX;
        samplerStates.push_back(device.CreateSamplerState(shadowMapSampler));
    }

    // Wrap linear sampler
    {
        D3D11_SAMPLER_DESC wrapLinearSampler{};
        wrapLinearSampler.Filter = D3D11_FILTER_ANISOTROPIC;
        wrapLinearSampler.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        wrapLinearSampler.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        wrapLinearSampler.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        wrapLinearSampler.MipLODBias = 0;
        wrapLinearSampler.MinLOD = 0;
        wrapLinearSampler.MaxLOD = D3D11_FLOAT32_MAX;
        wrapLinearSampler.MaxAnisotropy = 16;
        samplerStates.push_back(device.CreateSamplerState(wrapLinearSampler));
    }
}

FrameBuffer Graphics::CreateGBuffer(uint32_t width, uint32_t height, Device& device)
{
    std::vector<Texture> gbufferTextures;
    
    Texture::Description diffuseDesc = Texture::Description::CreateTexture2D("Diffuse Color", width, height, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, Texture::TEXTURE_SRV | Texture::TEXTURE_RTV, false);
    gbufferTextures.emplace_back(diffuseDesc, device);
    
    Texture::Description normalsDesc = Texture::Description::CreateTexture2D("Surface Normal / Shading Model", width, height, DXGI_FORMAT_R16G16B16A16_FLOAT, Texture::TEXTURE_SRV | Texture::TEXTURE_RTV, false);
    gbufferTextures.emplace_back(normalsDesc, device);
    
    Texture::Description ARMDesc = Texture::Description::CreateTexture2D("AO / Roughness / Metallic", width, height, DXGI_FORMAT_R11G11B10_FLOAT, Texture::TEXTURE_SRV | Texture::TEXTURE_RTV, false);
    gbufferTextures.emplace_back(ARMDesc, device);
    
    return FrameBuffer("GBuffer", std::move(gbufferTextures), device);
}

void Graphics::DrawMesh(Device& device, GlobalTransform& transform, MeshComponent& meshComp)
{
    Mesh* mesh = meshComp.mesh.GetLoadedAsync();
    if (!mesh || !mesh->IsLoaded() || !meshComp.isVisible)
        return;
    
    DrawMesh(device, transform, *meshComp.mesh, meshComp.materialOverrides);
}

void Graphics::DrawMesh(Device& device, const Transform& transform, Mesh& mesh)
{
    std::unordered_map<uint32_t, AssetRef<Material>> override;
    DrawMesh(device, transform, mesh, override);
}

void Graphics::DrawMesh(Device& device, const Transform& transform, Mesh& mesh, std::unordered_map<uint32_t, AssetRef<Material>>& materialOverrides)
{
    for (SubMesh& subMesh : mesh.subMeshes)
    {
        DrawMesh(device, transform, mesh, subMesh, materialOverrides);
    }
}

#ifdef IMGUI_ENABLED

void Graphics::DrawImGuiWindow(bool* opened)
{
    if (ImGui::Begin("Graphics", opened))
    {
        finalCopyPass.ImGuiDraw();

        if (ImGui::CollapsingHeader("Deferred Lighting Settings"))
        {
            deferredLightingPass.ImGuiDraw();
        }

        if (ImGui::CollapsingHeader("Shadows"))
        {
            shadowPass.ImGuiDraw();
        }

        if (ImGui::CollapsingHeader("Skybox"))
        {
            skyboxPass.ImGuiDraw();
        }

        if (ImGui::CollapsingHeader("Atmosphere"))
        {
            atmospherePass.ImGuiDraw(device);
        }

#if VXGI_ENABLED
        if (ImGui::CollapsingHeader("Voxelization"))
        {
            voxelizationPass.ImGuiDraw();
        }
#endif

        if (ImGui::CollapsingHeader("Post Process Settings"))
        {
            postProcessPass.ImGuiDraw();
        }
        
        if (ImGui::CollapsingHeader("Frame Buffers"))
        {
            scene.gBuffer.ImGuiDraw();
#ifdef EDITOR
            scene.editorBuffer.ImGuiDraw();
#endif
        }
        
        if (ImGui::CollapsingHeader("Loaded Meshes"))
        {
            resourceManager.meshLoader.ImGuiDraw();
        }

        if (ImGui::CollapsingHeader("Loaded Textures"))
        {
            resourceManager.textureLoader.ImGuiDraw();
        }

        if (ImGui::CollapsingHeader("Loaded Material"))
        {
            resourceManager.materialLoader.ImGuiDraw();
        }

        if (ImGui::CollapsingHeader("Loaded Shaders"))
        {
            resourceManager.shaderLoader.ImGuiDraw();
        }
    }
    ImGui::End();
}

#ifdef EDITOR

std::vector<std::tuple<PunctualLightComponent*, GlobalTransform*>> SortLightSpritesBackToFront(const Matrix& viewMatrix, const Array<std::tuple<PunctualLightComponent*, GlobalTransform*>>& punctualLightComponents)
{
    std::vector orderedLights = punctualLightComponents;
    std::ranges::sort(orderedLights, [&viewMatrix](const auto& a, const auto& b)
    {
        GlobalTransform* transformA = std::get<1>(a);
        GlobalTransform* transformB = std::get<1>(b);
        float distanceA = Vector3::Transform(transformA->position, viewMatrix).z;
        float distanceB = Vector3::Transform(transformB->position, viewMatrix).z;
        return distanceA < distanceB;
    });

    return orderedLights;
}

std::vector<std::tuple<CameraComponent*, GlobalTransform*>> SortCameraSpritesBackToFront(const Matrix& viewMatrix, const Array<std::tuple<CameraComponent*, GlobalTransform*>>& cameras)
{
    std::vector orderedCameras = cameras;
    std::ranges::sort(orderedCameras, [&viewMatrix](const auto& a, const auto& b) {
        GlobalTransform* transformA = std::get<1>(a);
        GlobalTransform* transformB = std::get<1>(b);
        float distanceA = Vector3::Transform(transformA->position, viewMatrix).z;
        float distanceB = Vector3::Transform(transformB->position, viewMatrix).z;
        return distanceA < distanceB;
    });

    return orderedCameras;
}

#endif

#endif // IMGUI_ENABLED
