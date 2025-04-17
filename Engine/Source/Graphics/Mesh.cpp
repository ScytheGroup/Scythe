module;

#include <assimp/Importer.hpp> // C++ importer interface
#include <assimp/postprocess.h> // Post processing flags
#include <assimp/scene.h> // Output data structure
#include <assimp/mesh.h> // Output data structure

#include <cstring> // Module interface doesn't exist yet for using import...

module Graphics:Mesh;
import :Mesh;
import :InputLayout;
import :ResourceManager;
import :AssimpImportHelpers;

import Common;

ConstantBuffer<ObjectData> Geometry::objectDataBuffer{};

Mesh::Mesh(const std::filesystem::path& filepath)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filepath.string(),
         aiProcessPreset_TargetRealtime_Fast |
         aiProcess_OptimizeMeshes |
         aiProcess_PreTransformVertices |
         aiProcess_FlipUVs
    );

    if (!scene)
        DebugPrintError("Assimp importation error: {}", importer.GetErrorString());

    std::filesystem::path meshFolder = filepath.parent_path();

    auto name = filepath.stem().string();

    materials.reserve(scene->mNumMaterials);
    for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
    {
        AssetRef<Material> material;
        if (aiMaterial* aiMaterial = scene->mMaterials[i])
        {
            std::string materialName;
            if (std::strlen(aiMaterial->GetName().C_Str()) > 0)
            {
                materialName = name + std::format("_MAT_{}", aiMaterial->GetName().C_Str());
            }
            else
            {
                materialName = name + std::format("_MAT_{}", i);
            }
            
            material = ResourceManager::Get().materialLoader.ImportFromMemory(AssetMetadata{ materialName });
            ImportMaterialData(material->data, *aiMaterial);
            ImportMaterialTexturesData(*material, *aiMaterial, meshFolder);

            material->shadingModelName = ShadingModelStore::LIT;
            material->RegisterLazyLoadedResource(true);
        }
        else
        {
            material = AssetRef<Material>{Guid{"DefaultUnlit"}};
            ResourceManager::Get().materialLoader.FillAsset(material);
        }
        materials.push_back(std::move(material));
    }
    
    geometry = std::make_shared<Geometry>();
    for (unsigned meshI = 0; meshI < scene->mNumMeshes; ++meshI) 
    {
        auto* mesh = scene->mMeshes[meshI];
        subMeshes.emplace_back(
            std::string(mesh->mName.data, mesh->mName.length),
            static_cast<uint32_t>(geometry->indices.size()),
            mesh->mNumFaces * 3,
            mesh->mMaterialIndex
        );
        uint32_t verticesCount = static_cast<uint32_t>(geometry->vertices.size());
        geometry->vertices.reserve(verticesCount + mesh->mNumVertices);
        for (unsigned i = 0; i < mesh->mNumVertices; ++i)
        {
            GenericVertex vert;
            vert.normal = AIVectorToVector(mesh->mNormals[i]);
            vert.position = AIVectorToVector(mesh->mVertices[i]);
            
            if (mesh->mTextureCoords && mesh->mTextureCoords[0])
                vert.texCoord = Vector2{ mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };

            geometry->vertices.push_back(std::move(vert));
        }

        geometry->indices.reserve(geometry->indices.size() + mesh->mNumFaces * 3);
        for (unsigned faceI = 0; faceI < mesh->mNumFaces; ++faceI)
        {
            std::transform(mesh->mFaces[faceI].mIndices, mesh->mFaces[faceI].mIndices + mesh->mFaces[faceI].mNumIndices, std::back_inserter(geometry->indices), [&](int index)
            {
                return verticesCount + index;
            });
        }
    }
}

Mesh::Mesh(std::shared_ptr<Geometry> sourceGeometry)
    : geometry(std::move(sourceGeometry))
{
    subMeshes.emplace_back("", 0, std::max(static_cast<uint32_t>(geometry->indices.size()), geometry->indexCount), 0);
    AssetRef<Material> matRef{Guid("Default")};
    ResourceManager::Get().materialLoader.FillAsset(matRef);
    materials.push_back(matRef);
}

void Mesh::LoadResource(Device& device)
{
    ILazyLoadedResource::LoadResource(device);

    geometry->LoadResource(device);

    for (auto& mat : materials)
    {
        ResourceManager::Get().materialLoader.FillAsset(mat);
    }
}

Material* Mesh::GetMaterial(int materialIndex, const std::unordered_map<uint32_t, AssetRef<Material>>& materialOverrides) const
{
    if (materialOverrides.contains(materialIndex))
        return materialOverrides.at(materialIndex).GetLoadedAsync();
    
    return materials[materialIndex].GetLoadedAsync();
}

std::shared_ptr<Geometry> MeshPresets::GetCubeGeometry()
{
    static constexpr std::array<uint32_t, 36> Indices = {
        0,  1,  2, // front
        0,  2,  3, // front

        5,  6,  7, // back
        5,  7,  4, // back

        8,  9,  10, // down
        8,  10, 11, // down

        13, 14, 15, // up
        13, 15, 12, // up

        19, 16, 17, // left
        19, 17, 18, // left

        20, 21, 22, // right
        20, 22, 23 // right
    };

    static constexpr float DX = 1;
    static constexpr float DY = 1;
    static constexpr float DZ = 1;

    static constexpr Vector3 Point[8] = { Vector3(-DX / 2, DY / 2, -DZ / 2), Vector3(DX / 2, DY / 2, -DZ / 2),
                                          Vector3(DX / 2, -DY / 2, -DZ / 2), Vector3(-DX / 2, -DY / 2, -DZ / 2),
                                          Vector3(-DX / 2, DY / 2, DZ / 2),  Vector3(-DX / 2, -DY / 2, DZ / 2),
                                          Vector3(DX / 2, -DY / 2, DZ / 2),  Vector3(DX / 2, DY / 2, DZ / 2) };

    // Normals
    static constexpr Vector3 N0(0.0f, 0.0f, -1.0f); // front
    static constexpr Vector3 N1(0.0f, 0.0f, 1.0f); // back
    static constexpr Vector3 N2(0.0f, -1.0f, 0.0f); // down
    static constexpr Vector3 N3(0.0f, 1.0f, 0.0f); // up
    static constexpr Vector3 N4(-1.0f, 0.0f, 0.0f); // left
    static constexpr Vector3 N5(1.0f, 0.0f, 0.0f); // right

    static constexpr std::array<GenericVertex, 24> Vertices{
        GenericVertex{ { { .position = Point[0] }, { .normal = N0 }, { .texCoord = Vector2{ 0, 0 } } } },
        GenericVertex{ { { .position = Point[1] }, { .normal = N0 }, { .texCoord = Vector2{ 1, 0 } } } },
        GenericVertex{ { { .position = Point[2] }, { .normal = N0 }, { .texCoord = Vector2{ 1, 1 } } } },
        GenericVertex{ { { .position = Point[3] }, { .normal = N0 }, { .texCoord = Vector2{ 0, 1 } } } },
        GenericVertex{ { { .position = Point[4] }, { .normal = N1 }, { .texCoord = Vector2{ 0, 1 } } } },
        GenericVertex{ { { .position = Point[5] }, { .normal = N1 }, { .texCoord = Vector2{ 0, 0 } } } },
        GenericVertex{ { { .position = Point[6] }, { .normal = N1 }, { .texCoord = Vector2{ 1, 0 } } } },
        GenericVertex{ { { .position = Point[7] }, { .normal = N1 }, { .texCoord = Vector2{ 1, 1 } } } },
        GenericVertex{ { { .position = Point[3] }, { .normal = N2 }, { .texCoord = Vector2{ 0, 0 } } } },
        GenericVertex{ { { .position = Point[2] }, { .normal = N2 }, { .texCoord = Vector2{ 1, 0 } } } },
        GenericVertex{ { { .position = Point[6] }, { .normal = N2 }, { .texCoord = Vector2{ 1, 1 } } } },
        GenericVertex{ { { .position = Point[5] }, { .normal = N2 }, { .texCoord = Vector2{ 0, 1 } } } },
        GenericVertex{ { { .position = Point[0] }, { .normal = N3 }, { .texCoord = Vector2{ 0, 1 } } } },
        GenericVertex{ { { .position = Point[4] }, { .normal = N3 }, { .texCoord = Vector2{ 0, 0 } } } },
        GenericVertex{ { { .position = Point[7] }, { .normal = N3 }, { .texCoord = Vector2{ 1, 0 } } } },
        GenericVertex{ { { .position = Point[1] }, { .normal = N3 }, { .texCoord = Vector2{ 1, 1 } } } },
        GenericVertex{ { { .position = Point[0] }, { .normal = N4 }, { .texCoord = Vector2{ 0, 0 } } } },
        GenericVertex{ { { .position = Point[3] }, { .normal = N4 }, { .texCoord = Vector2{ 1, 0 } } } },
        GenericVertex{ { { .position = Point[5] }, { .normal = N4 }, { .texCoord = Vector2{ 1, 1 } } } },
        GenericVertex{ { { .position = Point[4] }, { .normal = N4 }, { .texCoord = Vector2{ 0, 1 } } } },
        GenericVertex{ { { .position = Point[1] }, { .normal = N5 }, { .texCoord = Vector2{ 0, 0 } } } },
        GenericVertex{ { { .position = Point[7] }, { .normal = N5 }, { .texCoord = Vector2{ 1, 0 } } } },
        GenericVertex{ { { .position = Point[6] }, { .normal = N5 }, { .texCoord = Vector2{ 1, 1 } } } },
        GenericVertex{ { { .position = Point[2] }, { .normal = N5 }, { .texCoord = Vector2{ 0, 1 } } } }
    };

    static std::shared_ptr<Geometry> cubeGeo = []()
    {
        auto cubeGeo = std::make_shared<Geometry>();
        cubeGeo->indices.reserve(Indices.size());
        std::ranges::copy(Indices, std::back_inserter(cubeGeo->indices));
        cubeGeo->vertices.reserve(Vertices.size());
        std::ranges::copy(Vertices, std::back_inserter(cubeGeo->vertices));
        return cubeGeo;
    }();

    return cubeGeo;
}

std::shared_ptr<Geometry> MeshPresets::GetPlaneGeometry()
{
    static constexpr std::array<uint32_t, 6> Indices = {
        0, 1, 2,
        0, 2, 3
    };

    static constexpr Vector3 Forward{ 0, 0, -1 };

    static constexpr std::array<GenericVertex, 4> Vertices{
        GenericVertex{ { { .position = { -0.5, 0.5, 0 } }, { .normal = Forward }, { .texCoord = Vector2{ 0, 0 } } } },
        GenericVertex{ { { .position = { 0.5, 0.5, 0 } }, { .normal = Forward }, { .texCoord = Vector2{ 1, 0 } } } },
        GenericVertex{ { { .position = { 0.5, -0.5, 0 } }, { .normal = Forward }, { .texCoord = Vector2{ 1, 1 } } } },
        GenericVertex{ { { .position = { -0.5, -0.5, 0 } }, { .normal = Forward }, { .texCoord = Vector2{ 0, 1 } } } },
    };

    static std::shared_ptr<Geometry> planeGeo = []()
    {
        auto planeGeo = std::make_shared<Geometry>();
        planeGeo->indices.reserve(Indices.size());
        std::ranges::copy(Indices, std::back_inserter(planeGeo->indices));
        planeGeo->vertices.reserve(Vertices.size());
        std::ranges::copy(Vertices, std::back_inserter(planeGeo->vertices));
        return planeGeo;
    }();

    return planeGeo;
}

std::vector<GenericVertex> MeshPresets::GetSphereVerts(float radius, uint32_t numSlices, uint32_t numStacks)
{
    std::vector<GenericVertex> verts;

    auto generateVertexFromPosUV = [&verts](Vector3 pos, Vector2 uv) {
        Vector3 normal = pos;
        normal.Normalize();
        verts.emplace_back(GenericVertex{ { { .position = pos }, { .normal = normal }, { .texCoord = uv } } });
    };

    auto generateVertexFromPos = [&verts](Vector3 pos) {
        Vector3 normal = pos;
        normal.Normalize();
        Vector2 uv = 
        {
            static_cast<float>(0.5 + std::atan2(pos.z, pos.x) / (2 * std::numbers::pi)),
            static_cast<float>(0.5 - asin(pos.y) / std::numbers::pi)
        };
        verts.emplace_back(GenericVertex{ { { .position = pos }, { .normal = normal }, { .texCoord = uv } } });
    };

    float deltaLatitude = static_cast<float>(std::numbers::pi / numSlices);
    float deltaLongitude = static_cast<float>(2 * std::numbers::pi / numStacks);

    // Compute all vertices first except normals
    for (uint32_t i = 0; i <= numSlices; ++i)
    {
        float latitudeAngle = static_cast<float>(std::numbers::pi / 2 - i * deltaLatitude);
        float xz = radius * cosf(latitudeAngle);
        float y = radius * sinf(latitudeAngle);

        for (uint32_t j = 0; j <= numStacks; ++j)
        {
            float longitudeAngle = j * deltaLongitude;

            generateVertexFromPosUV({ xz * cosf(longitudeAngle), y, xz * sinf(longitudeAngle) }
                , { static_cast<float>(j) / numStacks, static_cast<float>(i) / numSlices }
            );
        }
    }

    return verts;
}

std::vector<uint32_t> MeshPresets::GetSphereIndices(uint32_t numSlices, uint32_t numStacks)
{
    std::vector<uint32_t> indices;

    for (uint32_t i = 0; i < numSlices; ++i)
    {
        uint32_t k1 = i * (numStacks + 1);
        uint32_t k2 = k1 + numStacks + 1;
        // 2 Triangles per latitude block excluding the first and last longitudes blocks
        for (uint32_t j = 0; j < numStacks; ++j, ++k1, ++k2)
        {
            if (i != 0)
            {
                indices.push_back(k1);
                indices.push_back(k1 + 1);
                indices.push_back(k2);
            }

            if (i != numSlices - 1)
            {
                indices.push_back(k2);
                indices.push_back(k1 + 1);
                indices.push_back(k2 + 1);
            }
        }
    }

    return indices;
}

void Geometry::LoadResource(Device& device)
{
    ILazyLoadedResource::LoadResource(device);

    if (!vertexBuffer.IsSet())
    {
        vertexBuffer = VertexBuffer{ device, vertices };
        vertexCount = static_cast<uint32_t>(vertices.size());
    }
    
    if (!indexBuffer.IsSet())
    {
        indexBuffer = IndexBuffer<uint32_t>{ device, { indices.begin(), indices.end() }};
        indexCount = static_cast<uint32_t>(indices.size());
    }
    
    CalculateBounds();

    if (shouldFlushDataOnLoad)
    {
        vertices.clear();
        indices.clear();
    }
}

void Geometry::CalculateBounds()
{
    if (vertices.empty()) return;

    std::vector<Vector3> positions;
    positions.reserve(vertices.size());

    for (const auto& vertex : vertices)
    {
        positions.push_back(vertex.position);
    }

    DirectX::BoundingSphere::CreateFromPoints(bounds, vertices.size(), positions.data(), sizeof(Vector3));
    DirectX::BoundingBox::CreateFromPoints(box, vertices.size(), positions.data(), sizeof(Vector3));
}

#ifdef IMGUI_ENABLED

import ImGui;
import Systems;

bool Mesh::ImGuiDraw()
{
    using namespace std::literals;

    bool result = false;
    
    ScytheGui::Header("Geometry");
    
    ImGui::Indent();
    geometry->ImGuiDraw();
    ImGui::Unindent();

    ScytheGui::Header("Materials");
    
    ImGui::Indent();
    
    int i = 0;
    for (auto& material : materials)
    {
        ImGui::Text(std::format("{}", i++).c_str());
        ImGui::SameLine();
        if (ImGui::TreeNode(ResourceLoader<Material>::Get().GetName(material)))
        {
            result |= material.ImGuiDrawEditContent();
            ImGui::TreePop();
        }
    }
    
    ImGui::Unindent();

    return result;
}

bool SubMesh::ImGuiDraw()
{
    ImGui::Text(std::format("indices: {} -> {} mat: {}", startIndex, startIndex + indexCount, materialIndex).c_str());
    return false;
}

bool Geometry::ImGuiDraw()
{
    ImGui::LabelText("Vertices", std::format("{}", vertices.size()).c_str());
    ImGui::LabelText("Indices", std::format("{}", indices.size()).c_str());
    return false;
}

#endif

template<>
ArchiveStream& operator<<(ArchiveStream& ar, const GenericVertex& m)
{
    ar << m.normal;
    ar << m.position;
    ar << m.texCoord;
    return ar;
}

template<>
ArchiveStream& operator>>(ArchiveStream& ar, GenericVertex& m)
{
    ar >> m.normal;
    ar >> m.position;
    ar >> m.texCoord;
    return ar;
}

template<>
ArchiveStream& operator<<(ArchiveStream& ar, const SubMesh& m)
{
    ar << m.name;
    ar << m.indexCount;
    ar << m.materialIndex;
    ar << m.startIndex;
    return ar;
}

template<>
ArchiveStream& operator>>(ArchiveStream& ar, SubMesh& m)
{
    ar >> m.name;
    ar >> m.indexCount;
    ar >> m.materialIndex;
    ar >> m.startIndex;
    return ar;
}

template<>
ArchiveStream& operator<<(ArchiveStream& ar, const Geometry& m)
{
    ar << m.indices;
    ar << m.vertices;
    return ar;
}

template<>
ArchiveStream& operator>>(ArchiveStream& ar, Geometry& m)
{
    ar >> m.indices;
    ar >> m.vertices;
    return ar;
}

template<>
ArchiveStream& operator<<(ArchiveStream& ar, const Mesh& m)
{
    ar << *m.geometry;
    ar << m.materials;
    ar << m.subMeshes;
    return ar;
}

template<>
ArchiveStream& operator>>(ArchiveStream& ar, Mesh& m)
{
    m.geometry = std::make_shared<Geometry>();
    ar >> *m.geometry;
    ar >> m.materials;
    ar >> m.subMeshes;
    return ar;
}

std::unique_ptr<Mesh> MeshLoaderUtils::LoadAsset(ArchiveStream& readStream)
{
    auto m = std::make_unique<Mesh>();
    readStream >> *m;
    m->RegisterLazyLoadedResource(false);
    return m;
}

void MeshLoaderUtils::SaveAsset(const Mesh& asset, ArchiveStream& saveStream, EntityComponentSystem& ecs)
{
    saveStream << asset;
}

bool MeshLoaderUtils::HandlesExtension(const std::string& extension)
{
    constexpr const char* supportedAssets[] { "obj", "gltf", "glb" }; // Add extensions as needed
    return std::ranges::find(supportedAssets, extension) != std::end(supportedAssets);
}

void MeshLoaderUtils::LoadPresets(Device& device, ResourceLoader<Mesh>::CacheType& cache)
{
    auto cubeMesh = std::make_unique<Mesh>(MeshPresets::GetCubeGeometry());
    cubeMesh->RegisterLazyLoadedResource(false);
    cache[Guid("Cube")] = {
        AssetMetadata{
            .name = "Cube",
            .isLoaded = true,
            .isTransient = true,
        },
        std::move(cubeMesh)
    };

    auto sphereMesh = std::make_unique<Mesh>(MeshPresets::GetSphereGeometry<0.5f, 32, 32>());
    sphereMesh->RegisterLazyLoadedResource(false);
    cache[Guid("Sphere")] = {
        AssetMetadata{
            .name = "Sphere",
            .isLoaded = true,
            .isTransient = true,
        },
        std::move(sphereMesh)
    };

    auto planeMesh = std::make_unique<Mesh>(MeshPresets::GetPlaneGeometry());
    planeMesh->RegisterLazyLoadedResource(false);
    cache[Guid("Plane")] = {
        AssetMetadata{
            .name = "Plane",
            .isLoaded = true,
            .isTransient = true,
        },
        std::move(planeMesh)
    };
}