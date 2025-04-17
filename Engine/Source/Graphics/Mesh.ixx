module;

#include <filesystem>
#include <assimp/mesh.h>

export module Graphics:Mesh;

import :Resources;
import :InputLayout;
import :Material;

import Common;

export class SubMesh
{
public:
    std::string name;

    uint32_t startIndex, indexCount, materialIndex;

#ifdef IMGUI_ENABLED
    bool ImGuiDraw();
#endif
};

export struct Geometry : public ILazyLoadedResource
{
    std::vector<GenericVertex> vertices;
    std::vector<uint32_t> indices;

    DirectX::BoundingSphere bounds;
    DirectX::BoundingBox box;

    uint32_t vertexCount{}, indexCount{};

    IndexBuffer<uint32_t> indexBuffer;
    VertexBuffer<GenericVertex> vertexBuffer;

    static ConstantBuffer<ObjectData> objectDataBuffer;

    Geometry() = default;
    Geometry(const std::vector<GenericVertex>& vertices, const std::vector<uint32_t>& indices)
        : vertices{vertices}
        , indices{indices}
    {}

    void LoadResource(Device& device);
    void CalculateBounds();

#ifdef IMGUI_ENABLED
    bool ImGuiDraw();
#endif
};

export class Mesh : public ILazyLoadedResource
{
public:
    std::shared_ptr<Geometry> geometry;
    std::vector<SubMesh> subMeshes;
    std::vector<AssetRef<Material>> materials;

    Mesh() = default;
    Mesh(const std::filesystem::path& filepath);
    Mesh(std::shared_ptr<Geometry> sourceGeometry);

    Material* GetMaterial(int materialIndex, const std::unordered_map<uint32_t, AssetRef<Material>>& materialOverrides = {}) const;

#ifdef IMGUI_ENABLED
    bool ImGuiDraw();
#endif

    void LoadResource(Device& device) override;
};

export struct MeshPresets
{
    static std::shared_ptr<Geometry> GetCubeGeometry();
    template <float Radius, uint32_t NumSlices, uint32_t NumStacks>
    static std::shared_ptr<Geometry> GetSphereGeometry();
    static std::shared_ptr<Geometry> GetPlaneGeometry();

private:
    // This cannot be constexpr because of the fact that maths function are not constexpr :((((
    static std::vector<GenericVertex> GetSphereVerts(float radius, uint32_t numSlices, uint32_t numStacks);
    static std::vector<uint32_t> GetSphereIndices(uint32_t numSlices, uint32_t numStacks);
};

template <float Radius, uint32_t NumSlices, uint32_t NumStacks>
std::shared_ptr<Geometry> MeshPresets::GetSphereGeometry()
{
    static std::shared_ptr<Geometry> sphereGeo = []()
    {
        auto ptr = std::make_shared<Geometry>(GetSphereVerts(Radius, NumSlices, NumStacks), GetSphereIndices(NumSlices, NumStacks));
        return ptr;
    }();
    return sphereGeo;
}

export struct MeshLoaderUtils
{
    std::filesystem::path GetProjectRelativeStoreFolder() const noexcept { return "Resources/Imported/Meshes"; };
    std::string_view GetAssetDisplayName() const noexcept { return "Mesh"; };
    
    void LoadPresets(Device& device, ResourceLoader<Mesh>::CacheType& cache);
    std::unique_ptr<Mesh> LoadAsset(ArchiveStream& readStream);
    void SaveAsset(const Mesh& asset, ArchiveStream& saveStream, EntityComponentSystem& ecs);
    bool HandlesExtension(const std::string& extension);
    bool CanLoadAsync() const noexcept { return false; };
};
