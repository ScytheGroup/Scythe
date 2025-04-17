module;
#include "Common/Macros.hpp"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>

#include "Serialization/SerializationMacros.h"

module Systems.Physics:Shapes;

import Systems.Physics;
import Graphics;
import Core;
import Common;

inline JPH::TriangleList CreateTriangleList(Geometry& geometry)
{
    JPH::TriangleList triangles;
    triangles.reserve(geometry.indices.size() / 3);
    for (size_t i = 0; i < geometry.indices.size(); i += 3)
    {
        JPH::Triangle triangle{
            Convert(geometry.vertices[geometry.indices[i]].position),
            Convert(geometry.vertices[geometry.indices[i + 1]].position),
            Convert(geometry.vertices[geometry.indices[i + 2]].position)
        };
        triangles.push_back(triangle);
    }
    return triangles;
}

inline JPH::VertexList ExtractVertices(Geometry& geometry)
{
    JPH::VertexList vertices;
    vertices.reserve(geometry.vertices.size());
    for (auto& vertex : geometry.vertices)
    {
        Vector3 newPos = vertex.position;
        vertices.push_back({ newPos.x, newPos.y, newPos.z });
    }
    return vertices;
}

inline JPH::IndexedTriangleList ExtractIndices(Geometry& geometry)
{
    JPH::IndexedTriangleList triangles;
    triangles.reserve(geometry.indices.size() / 3);
    for (size_t i = 0; i < geometry.indices.size(); i += 3)
    {
        JPH::IndexedTriangle triangle{
            geometry.indices[i],
            geometry.indices[i + 1],
            geometry.indices[i + 2]
        };
        triangles.push_back(triangle);
    }
    return triangles;
}

void MeshShape::SetMesh(AssetRef<Mesh> meshRef)
{
    mesh = meshRef;
}

SphereShape::SphereShape(const SphereShape& sphere)
{
    settings.mRadius = sphere.settings.mRadius;
}

BoxShape::BoxShape(const BoxShape& other)
{
    settings.mHalfExtent = other.settings.mHalfExtent;
    settings.mConvexRadius = other.settings.mConvexRadius;
}

CapsuleShape::CapsuleShape(const CapsuleShape& other)
{
    settings.mRadius = other.settings.mRadius;
    settings.mHalfHeightOfCylinder = other.settings.mHalfHeightOfCylinder;
}

bool MeshShape::CanCreateShape()
{
    ResourceManager::Get().meshLoader.FillAsset(mesh);
    return mesh;
};

JPH::ShapeSettings::ShapeResult MeshShape::Create()
{
    ResourceManager::Get().meshLoader.FillAsset(mesh);
    if (!mesh)
    {
        JPH::Shape::ShapeResult result;
        result.SetError("Mesh was not set");
        return result;
    }
    
    settings.mTriangleVertices = ExtractVertices(*mesh->geometry);
    settings.mIndexedTriangles = ExtractIndices(*mesh->geometry);
    settings.Sanitize();

    return ShapeImpl::Create();
}

MeshShape::MeshShape(const MeshShape& other)
    : mesh{other.mesh}
{
    settings.mIndexedTriangles = other.settings.mIndexedTriangles;
    settings.mTriangleVertices = other.settings.mTriangleVertices;
}

#if EDITOR

import Editor;

bool BoxShape::ImGuiDraw()
{
    bool result = false;
    result |= ImGui::DragFloat3("Half Extents", settings.mHalfExtent.mF32, 0.1f, 0.1f, std::numeric_limits<float>::max(), "%.3f", ImGuiSliderFlags_AlwaysClamp);
    return result;
}

bool SphereShape::ImGuiDraw()
{
    bool result = false;
    result |= ImGui::DragFloat("Radius", &settings.mRadius, 0.1f, 0.1f, std::numeric_limits<float>::max(), "%.3f", ImGuiSliderFlags_AlwaysClamp);
    return result;
}

bool CapsuleShape::ImGuiDraw()
{
    bool result = false;
    result |= ImGui::DragFloat("Radius", &settings.mRadius, 0.1f, 0.1f, std::numeric_limits<float>::max(), "%.3f", ImGuiSliderFlags_AlwaysClamp);
    result |= ImGui::DragFloat("Half Height", &settings.mHalfHeightOfCylinder, 0.1f, 0.1f, std::numeric_limits<float>::max(), "%.3f", ImGuiSliderFlags_AlwaysClamp);
    return result;
}

bool MeshShape::ImGuiDraw()
{
    bool result = false;
    result |= DrawAssetRefDragDrop(mesh);
    return result;
}

#endif

BEGIN_SERIALIZATION(BoxShape)
    WRITE_KEY_FIELD("halfExtents", settings.mHalfExtent);
    WRITE_KEY_FIELD("convexRadius", settings.mConvexRadius);
END_SERIALIZATION()

BEGIN_DESERIALIZATION(BoxShape)
    PARSE_KEY_FIELD("halfExtents", settings.mHalfExtent);
    PARSE_KEY_FIELD("convexRadius", settings.mConvexRadius);
END_DESERIALIZATION()

BEGIN_SERIALIZATION(SphereShape)
    WRITE_KEY_FIELD("radius", settings.mRadius);
END_SERIALIZATION()

BEGIN_DESERIALIZATION(SphereShape)
    PARSE_KEY_FIELD("radius", settings.mRadius);
END_DESERIALIZATION()

BEGIN_SERIALIZATION(CapsuleShape)
    WRITE_KEY_FIELD("radius", settings.mRadius);
    WRITE_KEY_FIELD("halfHeight", settings.mHalfHeightOfCylinder);
END_SERIALIZATION()

BEGIN_DESERIALIZATION(CapsuleShape)
    PARSE_KEY_FIELD("radius", settings.mRadius);
    PARSE_KEY_FIELD("halfHeight", settings.mHalfHeightOfCylinder);
END_DESERIALIZATION()

BEGIN_SERIALIZATION(MeshShape)
    WRITE_FIELD(mesh)
END_SERIALIZATION()

BEGIN_DESERIALIZATION(MeshShape)
    PARSE_FIELD(mesh)
END_DESERIALIZATION()