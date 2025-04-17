module;
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>

#include "Reflection/TypeBase.h"

export module Systems.Physics:Shapes;
import Components;
import Common;
import Graphics;
import Tools;
import :Conversion;

export class Mesh;

export class Shape : public Base
{
    SCLASS(Shape, Base);
public:
    virtual ~Shape() = default;
    virtual JPH::ShapeSettings::ShapeResult Create() = 0;
    virtual bool CanCreateShape() { return true; };
    virtual JPH::ShapeSettings* GetSettings() = 0;

#if EDITOR
    virtual bool ImGuiDraw() = 0;
#endif
    // used internally. Do not use it.
    uint64_t storedHandle;
};

export template <class T>
requires std::is_base_of_v<JPH::ShapeSettings, T>
class ShapeImpl : public Shape
{
public:
    T settings;

    virtual JPH::ShapeSettings* GetSettings() { return &settings; }

    ShapeImpl()
    {
        settings.SetEmbedded();
    }
    
    template <class... TArgs>
    ShapeImpl(TArgs... args)
        : settings(args...)
    {
        settings.SetEmbedded();
    }

    ShapeImpl(const ShapeImpl& impl) requires std::is_copy_constructible_v<T>
        : settings{ impl.settings }
    {
    }

    JPH::ShapeSettings::ShapeResult Create() override
    {
        return settings.Create();
    }
};


export class SphereShape : public ShapeImpl<JPH::SphereShapeSettings>
{
    SCLASS(SphereShape, ShapeImpl);

public:
    SphereShape() : SphereShape(0.5) {};
    SphereShape(const SphereShape& sphere);

    template <typename... TArgs>
    SphereShape(float radius, TArgs&&... otherArgs)
        : Super(radius, std::forward<TArgs>(otherArgs)...)
    {
    }

#if EDITOR
    virtual bool ImGuiDraw() override;
#endif
};

export class BoxShape : public ShapeImpl<JPH::BoxShapeSettings>
{
    SCLASS(BoxShape, ShapeImpl);
    
public:
    BoxShape() : BoxShape(Vector3::One * 0.5f) {}
    BoxShape(const BoxShape& other);

    template <typename... TArgs>
    BoxShape(const Vector3& halfExtent, TArgs&&... otherArgs)
        : Super(JPH::Vec3Arg{ halfExtent.x, halfExtent.y, halfExtent.z }, std::forward<TArgs>(otherArgs)...)
    {
    }

#if EDITOR
    virtual bool ImGuiDraw() override;
#endif
};

export class CapsuleShape : public ShapeImpl<JPH::CapsuleShapeSettings>
{
    SCLASS(CapsuleShape, ShapeImpl);

public:
    CapsuleShape() : CapsuleShape(0.5, 0.5) {};
    CapsuleShape(const CapsuleShape&);
    
    template <typename... TArgs>
    CapsuleShape(float halfHeight, float radius, TArgs&&... otherArgs)
        : Super(halfHeight, radius, std::forward<TArgs>(otherArgs)...)
    {
    }

#if EDITOR
    virtual bool ImGuiDraw() override;
#endif
};

export class ConvexHullShape : public ShapeImpl<JPH::ConvexHullShapeSettings>
{
    SCLASS(ConvexHullShape, ShapeImpl);

public:
    ConvexHullShape() = default;
    template <typename... TArgs>
    ConvexHullShape(const std::span<JPH::Vec3> points, TArgs&&... otherArgs)
        : Super(points, points.size(), std::forward<TArgs>(otherArgs)...)
    {
    }
};

export class MeshShape : public ShapeImpl<JPH::MeshShapeSettings>
{
    SCLASS(MeshShape, ShapeImpl);

public:
    AssetRef<Mesh> mesh;

    JPH::ShapeSettings::ShapeResult Create() override;
    bool CanCreateShape() override;
    
    MeshShape() = default;
    MeshShape(const MeshShape& other);
    
    void SetMesh(AssetRef<Mesh> mesh);

#if EDITOR
    virtual bool ImGuiDraw() override;
#endif
};
