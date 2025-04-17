export module Graphics:Scene;

import :FrameBuffer;
import :ConstantBuffer;
import :ResourceManager;
import :Material;
import :DebugShapes;

export struct GraphicsScene
{
    Device& device;
    ResourceManager& resourceManager;

    Vector2 viewportSize;

#ifdef IMGUI_ENABLED
#ifdef EDITOR
    FrameBuffer editorBuffer;
#endif // EDITOR
#endif // IMGUI_ENABLED

    FrameBuffer backBuffer;
    FrameBuffer gBuffer;
    FrameBuffer sceneColorBuffer;
    FrameData frameData;

    ConstantBuffer<Material::MaterialData> materialCBuffer;
    ConstantBuffer<FrameData> frameDataBuffer;
    StructuredBuffer<PunctualLightData> lightDataBuffer;
    
    // Defines that the current IBL textures are invalid, due to changing the skybox texture, or changing settings for the atmosphere.
    // If this is true, we will skip all IBL related rendering features.
    bool invalidateIBL = false;
};
