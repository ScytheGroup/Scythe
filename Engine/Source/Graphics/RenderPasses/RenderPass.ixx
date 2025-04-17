export module Graphics:RenderPass;

import :ConstantBuffer;
import :GraphicsDefs;
import :Texture;
import :Skybox;
import :Shadows;
import :TextureHelpers;

class PhysicsDebugRenderer;
export struct GraphicsScene;
export class DepthStencil;
export class FrameBuffer;
export class EntityComponentSystem;
export struct SceneSettings;

struct MeshPass
{
    void Render(GraphicsScene& scene, EntityComponentSystem& ecs);
};

struct SceneShadows
{
    ConstantBuffer<DirectionalCascadedShadowMap::CascadesData>* shadowMapCascades{};
    IReadableResource* directionalShadowMap{};
    IReadableResource* spotShadowMaps{};
    StructuredBuffer<Matrix>* spotLightViewMatrices{};
    IReadableResource* pointShadowMaps{};
    StructuredBuffer<PointShadowMaps::PointLightData>* pointLightData{};
};

struct ShadowPass
{
    DirectionalCascadedShadowMap dirShadowMap;
    SpotShadowMaps spotShadowMaps;
    PointShadowMaps pointShadowMaps;
    
public:
    void Init(Device& device);
    void Render(GraphicsScene& scene, EntityComponentSystem& ecs);

    SceneShadows sceneShadowData;
#ifdef IMGUI_ENABLED
    bool ImGuiDraw();
#endif
    
};

struct IBLData
{
    bool useComputeForIrradianceData = true;
    std::optional<Texture> diffuseIrradianceMap;
    std::optional<Texture> prefilteredEnvironmentMap;
    std::optional<Texture> brdfIntegrationLUT;
    ColorSH9 diffuseIrradianceSH9;
    ConstantBuffer<std::array<Vector4, 7>> diffuseIrradianceSH9PackedConstantBuffer;

    void RecreateDiffuseIrradiance(Device& device, const Texture& mippedSkybox, const Texture& skybox);
    void RecreateEnvironmentMap(Device& device, const Texture& mippedSkybox);
    void RecreateBRDFLUT(Device& device);

#ifdef IMGUI_ENABLED
    void RecreateDiffuseIrradiance(Device& device, const Skybox& skybox);
    void RecreateEnvironmentMap(Device& device, const Skybox& skybox);
#endif
};

class AtmospherePass;

class DeferredLightingPass
{
public:
#if VXGI_ENABLED 
    struct VoxelGIConfig
    {
        float indirectBoost = 4.0f;
    } giConfig;
#endif
    struct Settings
    {
        bool enableIBL = true;
        bool useDiffuseIrradianceSH = true;
        
        bool dirShadows = true;
        bool pointShadows = true;
        bool spotShadows = true;
        bool shadowPCF = true;
    } settings;

    IBLData iblData;
private:
#if VXGI_ENABLED
    ConstantBuffer<VoxelGIConfig> voxelGIConfigBuffer;
#endif
public:
    void Init(Device& device, const Skybox* skybox, const AtmospherePass& atmospherePass, const SceneSettings& sceneSettings);
    void Render(
        GraphicsScene& scene,
        SceneShadows& shadowsData,
#if VXGI_ENABLED
        Texture* illuminationVolume,
#endif
        SceneSettings& sceneSettings, const AtmospherePass& atmospherePass, const Texture* skybox = nullptr);

    void RegenerateIBL(Device& device, const Texture* skybox, const AtmospherePass& atmospherePass, const SceneSettings& sceneSettings);
    void RegenerateIBL(GraphicsScene& scene, const Texture* skybox, const AtmospherePass& atmospherePass, const SceneSettings& sceneSettings);

#ifdef IMGUI_ENABLED
    bool ImGuiDraw();
#endif
};

#if VXGI_ENABLED
class VoxelizationPass
{
    static constexpr uint32_t VoxelizationDataCBuffer = 10;
    
    struct VoxelizationData
    {
        uint32_t resolution = 512;
        Vector3 minBounds = Vector3{ -128, -128, -128 };
        Vector3 maxBounds = Vector3{  128,  128,  128 };

        float coneTracingStep = 0.5f;
        float coneTracingAngle = 0.628319f;
        float HDRPackingFactor = 100.0f;
        float maxConeTraceDistance = 100.0f;
        float temporalUpdateAlpha = 0.85f;
    } data;

    // This propagates the light across the scene
    int numAdditionnalBounces = 0;

    DirectionalProjectedShadowMap dirShadowMap{};
    
    GeometryShader* voxelisationGeomShader{};

    GeometryShader* voxelisationDebugGeomShader{};
    PixelShader* voxelisationDebugPixelShader{};
    VertexShader* voxelisationDebugVertexShader{};

    ComputeShader* clearVoxelsShader{};
    ComputeShader* illumVolumeShader{};
    ComputeShader* bouncesShader{};

    // These could be replaced by textures3ds
    StructuredBuffer<uint32_t> voxelData;
    //StructuredBuffer<Vector2> normalData;
    
    ConstantBuffer<VoxelizationData> voxelizationBuffer;
    ConstantBuffer<int> bounceInfoBuffer;
    
    std::optional<Texture> illuminationVolume;

    Vector3 GetVoxelBoundsCenter() const;
    Vector3 GetVoxelBoundsSize() const;

    void DrawMesh(Device& device, Transform& transform, Mesh& mesh, SubMesh& subMesh, const Material& material);
#ifdef EDITOR
    void ShowDebugVoxels(GraphicsScene& scene);
#endif
    void VoxelizeScene(GraphicsScene& scene, EntityComponentSystem& ecs, SceneShadows& shadowsData);
    void BuildIlluminationVolume(GraphicsScene& scene, StructuredBuffer<uint32_t>& source);
    void ClearVoxels(GraphicsScene& scene);
    void ComputeLightBounce(GraphicsScene& scene, StructuredBuffer<uint32_t>& destBuffer);
    void ComputeLightBounces(GraphicsScene& scene);
public:
    void Init(Device& device);
    void Render(GraphicsScene& scene, EntityComponentSystem& ecs, SceneShadows& shadowData);
    Texture* GetIlluminationVolume();

#ifdef EDITOR
    bool showVoxels = false;
    bool showBounds = false;
    bool shouldRecomputeVoxels = true;
#endif
    
#ifdef IMGUI_ENABLED
    bool ImGuiDraw();
#endif
};
#endif

class PostProcessPass
{
    bool enabled = true;
    struct Settings
    {
        bool blurEnabled = false;
        int blurKernelSize = 9;
        
        bool vignetteEnabled  = true;
        
        bool toneMappingEnabled = true;
        
        ToneMappingType toneMappingType = ToneMappingType::ACES;
    } settings;
    
    void ApplyVignette(GraphicsScene& scene);
    void ApplyBlur(GraphicsScene& scene);
    void ApplyToneMapping(GraphicsScene& scene);
    void ApplyCustomPostProcessPasses(GraphicsScene& scene, const SceneSettings& sceneSettings);

public:
    void Render(GraphicsScene& scene, const SceneSettings& sceneSettings);

#ifdef IMGUI_ENABLED
    bool ImGuiDraw();
#endif
};

class SkyboxPass
{
    std::filesystem::path skyboxTexturePath;
    std::optional<Skybox> skybox{};

#ifdef IMGUI_ENABLED
public:
#endif
    void Recreate(Device& device, SceneSettings& sceneSettings);
    
public:
    void Init(Device& device, SceneSettings& sceneSettings);
    void Render(GraphicsScene& scene, SceneSettings& sceneSettings);

    const Skybox* GetSkybox() const;

#ifdef IMGUI_ENABLED
    bool ImGuiDraw();
#endif
};

struct AtmosphereSettings
{
    float planetRadius = 6371.0f;
    float atmosphereHeight = 80.0f;
    float densityScaleHeightRayleigh = 7.994f;
    float densityScaleHeightMie = 1.200f;
    Vector3 planetCenter = Vector3(0.0f, -6371.0f, 0.0f);
    float atmosphereIntensity = 4.0f;
    bool useStaticSkyView = true; // Not passed to shader
};

class AtmospherePass
{
public:
    void Init(Device& device, SceneSettings& sceneSettings);
    void Render(GraphicsScene& scene, SceneSettings& sceneSettings);

    const AtmosphereSettings& GetSettings() const { return atmosphereSettings; }
    const ConstantBuffer<AtmosphereSettings>& GetSettingsBuffer() const { return atmosphereSettingsCBuffer; }
    const Texture& GetTransmittanceLUT() const { return *transmittanceLUT; }
    const Texture& GetMultipleScatteringLUT() const { return *multipleScatteringLUT; }

#ifdef IMGUI_ENABLED
    void ImGuiDraw(Device& device);
#endif
private:
    void GenerateTransmittanceLUT(Device& device);
    void GenerateMultipleScatteringLUT(Device& device);
    void GenerateSkyLUT(Device& device, Vector3 cameraPosition, Vector3 sunDirection);

    struct DirectionalLight
    {
        Vector3 color = { 1, 1, 1 };
        float intensity = 5;
        Vector3 direction = { -1, -1, -1 };
    } directionalLightSettings;
    AtmosphereSettings atmosphereSettings;

    // Only used for MSLUT generation, which is why this is apart from the atmosphere settings.
    Vector3 groundAlbedo = {0.1f, 0.1f, 0.1f};

    ConstantBuffer<AtmosphereSettings> atmosphereSettingsCBuffer;

    static constexpr UIntVector2 TransmittanceLUTSize = {256, 64}; 
    static constexpr UIntVector2 MultipleScatteringLUTSize = {32, 32};
    static constexpr UIntVector2 SkyLUTSize = {640, 360};
    std::optional<Texture> transmittanceLUT;
    std::optional<Texture> multipleScatteringLUT;
    std::optional<Texture> skyLUT;
};

struct FinalCopyPass
{
    void Render(GraphicsScene& scene);
#ifdef IMGUI_ENABLED
    bool ImGuiDraw();
#endif

private:
    bool useFXAA = true;
};

#ifdef EDITOR

class DebugPass
{
    // Would use unique ptr here but forward declare doesnt work for some reason
    PhysicsDebugRenderer* renderer{};
    
    ComPtr<ID3D11InputLayout> inputLayout;
    VertexShader* debugLinesVS{};
    PixelShader* debugLinesPS{};

    void DrawDebugLines(GraphicsScene& scene);

    bool enableDebugDisplay = true;
    bool showAll = false;
public:
    DebugPass();
    void Init(Device& device);
    void Render(GraphicsScene& scene, EntityComponentSystem& ecs);
    ~DebugPass();
};

#endif

#ifdef IMGUI_ENABLED

export class ShaderResourceLoader;

class ImGuiPass
{
    Editor* editor{};

#ifdef EDITOR
    void DrawEditorViewport(GraphicsScene& scene);
    void DrawEditorImGui(GraphicsScene& scene);
    void DrawEditorSprites(GraphicsScene& scene, EntityComponentSystem& ecsRef);
    void DrawEditorGizmos(GraphicsScene& scene, EntityComponentSystem& ecsRef);
#endif
    
public:
    void Init(Editor* editor);
    void Render(GraphicsScene& scene, ShaderResourceLoader& shaderLoader);
};

#endif
