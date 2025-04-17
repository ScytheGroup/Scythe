export module Graphics:Skybox;

import Common;
import :Mesh;
import :Texture;

export class Device;

// TODO: add this to world settings as a graphics resource
export class Skybox
{
public:
    Skybox(Texture&& cubemapTexture, Device& device);

    bool ImGuiDraw();
    
    Transform transform;
    std::shared_ptr<Geometry> geometry;
    ShadingModel shadingModel;
    Texture texture;
};