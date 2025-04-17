module;
#include <Common/Macros.hpp>
module Sandbox;

import :TestComponent;

import Scythe;
import Systems;
import Common;

import :TestComponent;

static auto testFcnt()
{
    auto currentTimestamp = std::chrono::system_clock::now();
    auto endTime = currentTimestamp + std::chrono::milliseconds(300);
    while (currentTimestamp < endTime)
    {
        currentTimestamp = std::chrono::system_clock::now();
    };
};

SandboxGame::SandboxGame()
{
    IMGUI_REGISTER_WINDOW("Sandbox Debug", SCENE, SandboxGame::DrawDebug);
    initialLoadedScene = "Resources/Scenes/Empty.json"_p;
}

void SandboxGame::Init()
{
    GetECS().GetSystem<Physics>().contactListener.OnContactAddedDelegate.Subscribe([this](auto& ecs, Handle b1, Handle b2, const JPH::ContactManifold& manifold, const JPH::ContactSettings& setting)
    {
        DebugPrint("Collision started between {}{} and {}{}",
            b1.id, ecs.GetComponent<CollisionTriggerComponent>(b1) ? " (trigger)" : "",
            b2.id, ecs.GetComponent<CollisionTriggerComponent>(b2) ? " (trigger)" : "");

        Physics& PhysicsSystem = GetECS().GetSystem<Physics>();
        BodyManipulationInterface& bodyInterface = PhysicsSystem.GetBodyManipulationInterface();

        if (ecs.GetComponent<CollisionTriggerComponent>(b2))
        {
            if (TestComponent* testComponent = GetECS().GetComponent<TestComponent>(b1))
            {
                GetECS().RemoveEntity(b2);
            }
            if (auto* body = ecs.GetComponent<RigidBodyComponent>(b1))
                bodyInterface.AddImpulse(*body, Vector3::Up * 100000000.0f);
        }
    });
    GetECS().GetSystem<Physics>().contactListener.OnContactRemovedDelegate.Subscribe([this](auto& ecs, Handle b1, Handle b2)
    {
        DebugPrint("Collision ended between {} and {}", b1, b2);
    });

    InputSystem::SetMouseModeRelative();
}

void SandboxGame::Update(double dt)
{
}

#ifdef IMGUI_ENABLED

void SandboxGame::DrawDebug(bool* opened)
{
    if (!ImGui::Begin("Sandbox Debug", opened))
        return;

    ScytheGui::Header("Job Scheduler");
    ImGui::Text("Current held thread id is %d", jobHandle.IsValid() ? jobHandle->GetId() : -1);
    if (ImGui::Button("Add test thread"))
    {
        jobHandle = JobScheduler::Get().CreateJob([] { testFcnt(); });
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete current handle") && jobHandle.IsValid())
    {
        jobHandle = JobHandle();
    }

    if (ImGui::Button("Test with deps and barriers"))
    {
        auto& scheduler = JobScheduler::Get();

        Barrier testBarrier;

        for (uint32_t i = 0; i < JobScheduler::GetMaxConcurrency(); ++i)
        {
            auto job1 = scheduler.CreateJob(testFcnt, 0);
            testBarrier.AddJob(job1);
            scheduler.CreateJob([job1]()
            {
                std::thread::id this_id = std::this_thread::get_id();
                DebugPrint("Creating a master job");
                auto currentTimestamp = std::chrono::system_clock::now();
                auto endTime = currentTimestamp + std::chrono::seconds(1);
                while (currentTimestamp < endTime)
                {
                    currentTimestamp = std::chrono::system_clock::now();
                };
                job1.GetPtr()->DecrementDependencies();
            });
        }

        DebugPrint("Waiting for first batch of jobs to finish");
        testBarrier.Wait();
    }

    if (ImGui::Button("Run 100 test jobs"))
    {
        auto& scheduler = JobScheduler::Get();

        for (size_t i = 0; i < 100; ++i)
            scheduler.CreateJob(&testFcnt);
    }

    if (GetECS().IsSimulationActive() && ImGui::Button("Add physics entity"))
    {
        auto handle = GetECS().AddEntity();
        GetECS().AddComponentAndRequirements<RigidBodyComponent>(handle);
        auto* shape = GetECS().AddComponent<CollisionShapeComponent>(handle);
        shape->shapeSettings = std::make_unique<BoxShape>();
    }
    if (GetECS().IsSimulationActive() && ImGui::Button("Add a lonely physics body"))
    {
        auto handle = GetECS().AddEntity();
        GetECS().AddComponent<RigidBodyComponent>(handle);

        auto* shape = GetECS().AddComponent<CollisionShapeComponent>(handle);
        shape->shapeSettings = std::make_unique<BoxShape>();
    }
    
    ImGui::End();
}

#endif
