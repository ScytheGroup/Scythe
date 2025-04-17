module;

#include "Common/Macros.hpp"
#include "Serialization/SerializationMacros.h"

export module Components:Transform;

import Common;
import Generated;
import :Component;
import Serialization;

export class TransformComponent : public Component
{
    SCLASS(TransformComponent, Component);
    REQUIRES(GlobalTransform);

public:
    TransformComponent();
    TransformComponent(const Matrix& initialMat);
    TransformComponent(const Transform& initialMat);

    union
    {
        Transform transform;

        struct
        {
            Vector3 position;    // X, Y, Z
            Quaternion rotation; // Quaternion
            Vector3 scale;       // X, Y, Z
        };
    };

    operator Transform();
    void operator=(Transform transform);
};

export class GlobalTransform : public Component
{
    SCLASS(GlobalTransform, Component);
    REQUIRES(TransformComponent);

public:
    explicit GlobalTransform();

    union
    {
        const Transform transform = {};

        struct
        {
            const Vector3 position;    // X, Y, Z
            const Quaternion rotation; // Quaternion
            const Vector3 scale;       // X, Y, Z
        };
    };
    
    operator Transform();
};


DECLARE_SERIALIZABLE(TransformComponent);
DECLARE_SERIALIZABLE(GlobalTransform);