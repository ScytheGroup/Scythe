module;
#include "Common/Macros.hpp"
#include "Physics/PhysicsSystem-Headers.hpp"

#include "Jolt/Physics/Character/CharacterBase.h"
#include "Jolt/Physics/Character/CharacterVirtual.h"
#include "ThirdParty/DirectXTex/DirectXTex.h"
export module Components:Character;

export import :CharacterSettings;
import :Component;

import Serialization;

import Generated;

export using GroundState = JPH::CharacterBase::EGroundState;

export class CharacterControllerComponent : public Component
{
    SCLASS(CharacterControllerComponent, Component);
    REQUIRES(TransformComponent, CollisionShapeComponent);
    MANUALLY_DEFINED_EDITOR();

    JPH::ShapeRefC innerShape;
    std::unique_ptr<JPH::CharacterVirtual> innerCharacter = nullptr;
    void UpdateInnerSettings();


    bool showDebug = false;
public:
    friend class CharacterControllerSystem;
    CharacterSettings settings;

    // The linear velocity of the controller. You need to update this yourself in order to get a velocity effect.
    // Remember that the gravity is not applied automatically so you need to include it in you calculations.
    Vector3 linearVelocity;

    Vector3 GetGroundVelocity();
    GroundState GetGroundState() const;

    CharacterControllerComponent() = default;
    CharacterControllerComponent(const CharacterControllerComponent& component) { settings = component.settings; }
};

DECLARE_SERIALIZABLE(CharacterControllerComponent);
