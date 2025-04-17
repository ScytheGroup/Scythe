export module Editor:AssetBrowser;

import Common;
import Tools;
import Reflection;
import Graphics;

import :AssetItem;

export class AssetBrowser
{
    Array<AssetItem<Material>> materials;
    Array<AssetItem<Mesh>> meshes;
    Array<AssetItem<Texture>> textures;
    bool needsRefresh = false;

    struct AssetBrowserStyle
    {
        float cellPadding = 10.0f;
        float itemWidth = 100.0f;
        float itemHeight = 150.0f;
    } style;

    IAssetRef* selectedAsset = nullptr;
public:
    AssetBrowser();
    void ReloadAssets();
    void ImGuiDraw(bool*);
    void DrawAssetDetails(bool*);
};
