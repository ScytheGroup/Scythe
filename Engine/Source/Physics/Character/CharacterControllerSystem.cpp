module;
#include "Physics/PhysicsSystem-Headers.hpp"

#include <Jolt/Physics/Character/CharacterVirtual.h>
#include "Jolt/Physics/Collision/Shape/EmptyShape.h"
module Systems;

import :CharacterUpdater;
import Core;
import Components;
import Systems.Physics;

void CharacterControllerSystem::CheckCreateComponent(TransformComponent* transform, CharacterControllerComponent* character, CollisionShapeComponent* collisionShape)
{
    if (!character->innerCharacter)
    {
        auto tempSettings = character->settings.GetInnerSettings();
        tempSettings.mInnerBodyLayer = std::to_underlying(Layers::MOVING);

        if (collisionShape->shapeSettings && !character->innerShape /*|| shapeChanged*/)
        {
            JPH::ShapeRefC shapeRef;
            JPH::Shape::ShapeResult shapeResult;

            shapeResult = collisionShape->shapeSettings->Create();
            if (shapeResult.IsValid())
            {
                shapeRef = shapeResult.Get();
                shapeResult = shapeRef->ScaleShape(Convert(transform->transform.scale));
                if (shapeResult.IsValid())
                {
                    character->innerShape = shapeResult.Get();
                }
                else
                {
                    GetEcs().RemoveComponent(collisionShape);
                    return;
                }
            }
        }

        tempSettings.mInnerBodyShape = character->innerShape->ScaleShape(JPH::Vec3::sOne() * 1.1).Get();
        character->innerCharacter = std::make_unique<JPH::CharacterVirtual>(&tempSettings, Convert(transform->position), Convert(transform->rotation), transform->GetOwningEntity(), physicsSystem);
#ifdef EDITOR
        character->innerCharacter->sDrawConstraints = true;
#endif

        character->innerCharacter->SetCharacterVsCharacterCollision(&characterCollisions);
        // character->innerCharacter->SetListener(&contactListener);
        character->innerCharacter->SetUp(JPH::Vec3::sAxisY());
        std::unique_ptr<JPH::TempAllocator> tempAllocator = Physics::CreateTempAllocator();
        bool shapeCreated = character->innerCharacter->SetShape(character->innerShape, std::numeric_limits<float>::max(),
                                    physicsSystem->GetDefaultBroadPhaseLayerFilter(std::to_underlying(Layers::MOVING)),
                                    physicsSystem->GetDefaultLayerFilter(std::to_underlying(Layers::MOVING)), {}, {}, *tempAllocator);
        Assert(shapeCreated);
    }
}

void CharacterControllerSystem::Init()
{
    Super::Init();
    physicsSystem = &GetEcs().GetSystem<Physics>().physicsSystem;

    auto characters = GetEcs().GetArchetype<TransformComponent, CharacterControllerComponent, CollisionShapeComponent>();

    for (auto [transform, character, collisionShape] : characters)
    {
        CheckCreateComponent(transform, character, collisionShape);
        auto allocator = Physics::CreateTempAllocator();
        character->innerCharacter->RefreshContacts(physicsSystem->GetDefaultBroadPhaseLayerFilter(std::to_underlying(Layers::MOVING)),
                                                   physicsSystem->GetDefaultLayerFilter(std::to_underlying(Layers::MOVING)), {}, {}, *allocator);
    }
}

void CharacterControllerSystem::Update(double delta)
{
    auto characters = GetEcs().GetArchetype<TransformComponent, CharacterControllerComponent, CollisionShapeComponent>();
    const auto gravity = Convert(GetEcs().GetSceneSettings().physics.gravity);
    for (auto [transform, character, collisionShape] : characters)
    {        
        auto tempAllocator = Physics::CreateTempAllocator();
        CheckCreateComponent(transform, character, collisionShape);
        JPH::CharacterVirtual& innerCharacter = *character->innerCharacter;
#ifdef IMGUI_ENABLED
        innerCharacter.sDrawConstraints = character->showDebug;
        innerCharacter.sDrawWalkStairs = character->showDebug;
        innerCharacter.sDrawStickToFloor = character->showDebug;
#endif
        innerCharacter.SetLinearVelocity(Convert(character->linearVelocity));

        innerCharacter.SetRotation(Convert(transform->rotation));
        innerCharacter.SetPosition(Convert(transform->position));
        innerCharacter.SetMass(character->settings.mass);
        innerCharacter.SetMaxStrength(character->settings.maxStrength);
        innerCharacter.SetShapeOffset(Convert(character->settings.shapeOffset));

        innerCharacter.ExtendedUpdate(delta, JPH::Vec3::sAxisY() * Convert(GetEcs().GetSceneSettings().physics.gravity).Length(), {},
                                      physicsSystem->GetDefaultBroadPhaseLayerFilter(std::to_underlying(Layers::MOVING)),
                                      physicsSystem->GetDefaultLayerFilter(std::to_underlying(Layers::MOVING)), {}, {}, *tempAllocator);

        transform->transform.position = Convert(innerCharacter.GetPosition());
        transform->transform.rotation = Convert(innerCharacter.GetRotation());
    }
}

void CharacterControllerSystem::OnComponentAdded(Component& component)
{}

void CharacterControllerSystem::OnComponentChanged(Component& component)
{
    auto result = GetEcs().GetComponents<TransformComponent, CharacterControllerComponent, CollisionShapeComponent>(component.GetOwningEntity());
    if (!result.has_value())
        return;

    auto [transform, character, collisionShape] = result.value();

    if (!character->innerCharacter)
    {
        CheckCreateComponent(transform, character, collisionShape);
        return;
    }

    character->UpdateInnerSettings();

    if (collisionShape->shapeSettings)
    {
        JPH::ShapeRefC shapeRef;
        JPH::Shape::ShapeResult shapeResult;

        shapeResult = collisionShape->shapeSettings->Create();
        if (shapeResult.IsValid())
        {
            shapeRef = shapeResult.Get();
            shapeResult = shapeRef->ScaleShape(Convert(transform->transform.scale));
            if (shapeResult.IsValid())
            {
                character->innerShape = shapeResult.Get();
                std::unique_ptr<JPH::TempAllocator> tempAllocator = Physics::CreateTempAllocator();
                character->innerCharacter->SetShape(character->innerShape.GetPtr(), std::numeric_limits<float>::max(), {}, {}, {}, {}, *tempAllocator);
                character->innerCharacter->SetInnerBodyShape(character->innerShape.GetPtr());
            }
            else
                GetEcs().RemoveComponent(collisionShape);
        }
    }
}

void CharacterControllerSystem::OnComponentRemoval(Component& component)
{
    Super::OnComponentRemoval(component);
}