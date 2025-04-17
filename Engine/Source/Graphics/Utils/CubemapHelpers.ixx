export module Graphics:CubemapHelpers;

import :Texture;
import :Mesh;
import :FrameBuffer;
import :DepthStencil;
import :RenderPass;

class Graphics;

struct DrawCubemapInfo
{
    std::shared_ptr<Geometry> geometry;
    DepthStencil depthStencil;
    D3D11_VIEWPORT viewport;
};

export struct CubemapHelpers
{
    static Texture ConvertEquirectangularTextureToCubeMap(const Texture& equirectangularTexture, Device& device);
    static Texture CreateDiffuseIrradianceMap(const Texture& environmentMap, Device& device);
    static Matrix GetViewMatrixForFace(int faceIndex, const Vector3& position = Vector3::Zero);
    static Texture CreatePrefilteredEnvironmentMap(const Texture& environmentMap, Device& device);
    static Vector3 ConvertCubemapFaceUVToVec3(const int face, Vector2 uv);

    static Texture CreateCubemapFromAtmosphere(Device& device,
        const DirectionalLightData& directionalLight,
        const ConstantBuffer<AtmosphereSettings>& atmosphereSettingsCBuffer,
        const Texture& transmittanceLUT, const Texture& multipleScatteringLUT);
    
private:
    static DrawCubemapInfo SetupDrawCubemap(ShadingModel& shadingModel, uint32_t cubemapResolutionSize, Device& device);
    static void ExecuteDrawCubemap(Texture& renderTexture, uint32_t mip, uint32_t face, DrawCubemapInfo& drawCubemapInfo, Device& device);
    static void CleanupDrawCubemap(Device& device);
};
