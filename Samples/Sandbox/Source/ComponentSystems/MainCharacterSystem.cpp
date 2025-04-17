module Sandbox;

import SandboxGenerated;
import : MainCharacterSystem;

static constexpr float Speed = 10;

void MainCharacterSystem::Init()
{
    System::Init();
    auto& contactListener = GetEcs().GetSystem<Physics>().contactListener;
    contactListener.OnContactAddedDelegate.Subscribe([]
        (auto& ecs, Handle entity1, Handle entity2, JPH::ContactManifold manifold, JPH::ContactSettings)
    {
        if (ecs.GetComponent<CharacterControllerComponent>(entity1))
        {
            DebugPrint("Entity {} : character controller detected", entity1.id);    
        }
        if (ecs.GetComponent<CharacterControllerComponent>(entity2))
        {
            DebugPrint("Entity {} : character controller detected", entity2);    
        }
    });
}

void MainCharacterSystem::Update(double delta)
{
    auto& ecs = GetEcs();

    auto characters = GetEcs().GetArchetype<TransformComponent, CharacterControllerComponent>();

    auto& inputSystem = InputSystem::Get();
    
    for (auto [tc, character] : characters)
    {
        auto& t = tc->transform;
        Vector3 velocity;
        const Vector3& gravity = GetEcs().GetSceneSettings().physics.gravity;
        
        // Linear Velocity
        Vector3 upDirection = t.GetUpVector();

        // Calculate horizontal and vertical components
        Vector3 horizontalVelocity = Vector3::Zero;
        Vector3 verticalVelocity = Vector3::Zero;
        
        if (inputSystem.IsKeyHeld(Inputs::Keys::S))
        {
            horizontalVelocity += -t.GetForwardVector() * Speed;
        }
        if (inputSystem.IsKeyHeld(Inputs::Keys::W))
        {
            horizontalVelocity += t.GetForwardVector() * Speed;
        }
        if (inputSystem.IsKeyHeld(Inputs::Keys::A))
        {
            horizontalVelocity += -t.GetRightVector() * Speed;
        }
        if (inputSystem.IsKeyHeld(Inputs::Keys::D))
        {
            horizontalVelocity += t.GetRightVector() * Speed;
        }
        if (inputSystem.IsKeyPressed(Inputs::Keys::Space))
        {
            verticalVelocity += upDirection * Speed;
        }
        
        if (character->GetGroundState() == GroundState::OnGround)
        {
            velocity = character->GetGroundVelocity() + horizontalVelocity + verticalVelocity + gravity * delta;
        }
        else
        {
            Vector3 currentVerticalVelocity = character->linearVelocity.Dot(upDirection) * upDirection;
            velocity = horizontalVelocity + currentVerticalVelocity + gravity * delta;
        }
        character->linearVelocity = velocity;

        // innerCharacter.SetRotation(Convert(transform->rotation));
        // innerCharacter.SetPosition(Convert(transform->position));
    }
}