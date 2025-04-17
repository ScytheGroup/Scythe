module;
#include "Common/Macros.hpp"

#include "../PhysicsSystem-Headers.hpp"

#include <Jolt/Physics/Character/CharacterBase.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
export module Systems:CharacterUpdater;

import :System;
export import :CharacterContactListener;

import Generated;

export class CharacterControllerSystem : public System
{
    SCLASS(CharacterControllerSystem, System);

    JPH::PhysicsSystem* physicsSystem;
    Array<Handle> newEntities;

    CharacterContactListener contactListener;
    JPH::CharacterVsCharacterCollisionSimple characterCollisions;

public:
    void Init() override;
    void Update(double delta) override;

    void OnComponentAdded(Component& component) override;
    void OnComponentChanged(Component& component) override;
    void OnComponentRemoval(Component& component) override;

private:
    void CheckCreateComponent(TransformComponent* transform, CharacterControllerComponent* character, CollisionShapeComponent* collisionShape);
};
