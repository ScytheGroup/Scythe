export module Editor:Icons;

import <memory>;
import Graphics;

export namespace EditorIcons
{
    std::unique_ptr<Texture> CameraIcon;
    std::unique_ptr<Texture> PointLightIcon;
    std::unique_ptr<Texture> SpotLightIcon;
    std::unique_ptr<Texture> AssetBrowserMeshIcon;
    std::unique_ptr<Texture> AssetBrowserTextureIcon;
    std::unique_ptr<Texture> AssetBrowserMaterialsIcon;

    inline bool AreSpriteIconsLoaded()
    {
        return CameraIcon && CameraIcon->IsLoaded() && PointLightIcon && PointLightIcon->IsLoaded() && SpotLightIcon && SpotLightIcon->IsLoaded();
    }
}