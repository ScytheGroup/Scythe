module;
#include "Common/Macros.hpp"
#include "Serialization/SerializationMacros.h"

export module Systems.Physics:ShapeComponent;

import Common;
import Components;
import :Shapes;

export class CollisionShapeComponent : public Component
{
    SCLASS(CollisionShapeComponent, Component);
    REQUIRES(TransformComponent);
    MANUALLY_DEFINED_EDITOR();
    friend class Physics;
public:
    std::unique_ptr<Shape> shapeSettings = std::make_unique<BoxShape>();

    CollisionShapeComponent();
    CollisionShapeComponent(const CollisionShapeComponent&);
};

DECLARE_SERIALIZABLE(CollisionShapeComponent)