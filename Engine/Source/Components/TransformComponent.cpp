module;

#include "Serialization/SerializationMacros.h"

module Components:Transform;
import :Transform;
import Core;

DEFINE_SERIALIZATION_AND_DESERIALIZATION(TransformComponent, transform)
DEFINE_SERIALIZATION_AND_DESERIALIZATION(GlobalTransform)

TransformComponent::TransformComponent()
    : transform{}
{
}

TransformComponent::TransformComponent(const Matrix& initialMat)
    : transform{ initialMat }
{
}

TransformComponent::TransformComponent(const Transform& initialTransform)
    : transform{ initialTransform }
{
}

TransformComponent::operator Transform()
{
    return transform;
}

void TransformComponent::operator=(Transform transform)
{
    this->transform = transform;
}

GlobalTransform::GlobalTransform()
{
}

GlobalTransform::operator Transform()
{
    return transform;
}
