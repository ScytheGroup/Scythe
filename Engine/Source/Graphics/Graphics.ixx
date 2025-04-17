module;
#include "Common/Macros.hpp"
export module Graphics;

export import :Device;
export import :Mesh;
export import :ResourceManager;
export import :Material;
export import :Texture;
export import :DebugShapes;

#ifdef RENDERDOC
export import :RenderDoc;
#endif

import :Lights;
import :Resources;
import :Skybox;
import :FrameBuffer;
import :Texture;
import :DepthStencil;
import :CubemapHelpers;
import :ShadingModel;
import :Shader;
import :Scene;
import :RenderPass;
import :ResourceRegister;

#ifdef PROFILING
import Profiling;
#endif

import Common;
import Systems;

export class PunctualLightComponent;
export class MeshComponent;
export class GlobalTransform;
export struct CameraProjection;
export struct CameraData;
export class EntityComponentSystem;

export std::vector<std::tuple<PunctualLightComponent*, GlobalTransform*>> SortLightSpritesBackToFront(const Matrix& viewMatrix,
                                                                                                      const Array<std::tuple<PunctualLightComponent*, GlobalTransform*>>& punctualLightComponents);
export std::vector<std::tuple<CameraComponent*, GlobalTransform*>> SortCameraSpritesBackToFront(const Matrix& viewMatrix,
                                                                                                      const Array<std::tuple<CameraComponent*, GlobalTransform*>>& cameras);

#define VXGI_ENABLED 0

export class Graphics : public System
{
public:
    Graphics(HWND window, uint32_t width, uint32_t height);
    Graphics(const Graphics&) = delete;
    Graphics& operator=(const Graphics&) = delete;
    Graphics(Graphics&&) = default;
    Graphics& operator=(Graphics&&) = default;
    ~Graphics() override;

    void Init() override;
    void Update(double deltaTime) override;

    void OnResize(uint32_t width, uint32_t height);

    void SetEditor(Editor& newEditor);

#ifdef IMGUI_ENABLED
    void DrawImGuiWindow(bool* opened);
#endif

    static void DrawMesh(Device& device, GlobalTransform& transform, MeshComponent& meshComp);
    static void DrawMesh(Device& device, const Transform& transform, Mesh& mesh);
    static void DrawMesh(Device& device, const Transform& transform, Mesh& mesh, std::unordered_map<uint32_t, AssetRef<Material>>& materialOverrides);
    static void DrawMesh(Device& device, const Transform& transform, Mesh& mesh, const SubMesh& subMesh, std::unordered_map<uint32_t, AssetRef<Material>>& materialOverrides);
    static void DrawMesh(Device& device, const Transform& transform, Mesh& mesh, const SubMesh& subMesh, const Material& materialOverrides);
    static void DrawMesh(Device& device, const Transform& transform, Mesh& mesh, const SubMesh& subMesh, const Material& material, const ShadingModel& shadingModel);
    static void DrawGeometry(Device& device, const Transform& transform, Geometry& geometry);
    static void DrawGeometry(Device& device, const Transform& transform, Geometry& geometry, int startIndex, int count);
    static void DrawFullscreenTriangle(Device& device, PixelShader& pixelShader);
    static void DrawSprite(Device& device, Texture& spriteTexture, Transform& transform);

    void DownloadResource(const Texture& tex);
    
    // TEMPORARY
    void ReloadTexture(const Texture& tex, const Texture::TextureData& data);
private:
    void BeginRender();
    void EndRender();
 
    void CreateSamplerStates();
    void BindSamplers();

    FrameBuffer CreateGBuffer(uint32_t width, uint32_t height, Device& device);
    
    void DrawCopyPass(Texture& renderTexture);

    void UpdateGraphicsScene(double delta, EntityComponentSystem& ecs);

    std::optional<Handle> GetSelectedEntityFromScreen(const Vector2& mousePosition, EntityComponentSystem& ecsRef);

    void ResizeViewport(uint32_t width, uint32_t height);

    void InitializeComponent(MeshComponent& mesh);

    void GenerateReflections();

public:
    static constexpr int FrameCBufferIndex = 0;
    static constexpr int ModelCBufferIndex = 1;
    static constexpr int MaterialCBufferIndex = 2;
    static constexpr int PunctualLightSRVIndex = 98;
private:
    static constexpr Color ClearColor = { 0.1f, 0.1f, 1.0f, 1.0f };
    static constexpr const char* BackBufferName = "GameViewport";
    static constexpr Vector2 MinViewportSize{ 100.0f, 100.0f };
    
    Device device;

    ResourceManager resourceManager;

    HWND window;
    
    Editor* editor{};

    GraphicsScene scene;

    bool activateVsync = false;

    /// Render passes
    MeshPass meshPass;
#if VXGI_ENABLED
    VoxelizationPass voxelizationPass;
#endif
    DeferredLightingPass deferredLightingPass;
    PostProcessPass postProcessPass;
    SkyboxPass skyboxPass;
    AtmospherePass atmospherePass;
    ShadowPass shadowPass;
    FinalCopyPass finalCopyPass;
#ifdef EDITOR
    DebugPass debugLinesPass;
#endif
#ifdef IMGUI_ENABLED
    ImGuiPass imguiPass;
#endif
    /// 

    std::vector<ComPtr<ID3D11SamplerState>> samplerStates;

    ResourceRegister graphicsResources;
    
#ifdef EDITOR
    Handle playInEditorDelegate;
#endif

#ifdef PROFILING
    Profiler profiler;
#endif

#ifdef IMGUI_ENABLED
    friend ImGuiManager;
#ifdef EDITOR
    friend Editor;
#endif
#endif
    friend CubemapHelpers;
};
