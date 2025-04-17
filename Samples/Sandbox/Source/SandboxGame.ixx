export module Sandbox;

export import :TestComponent;
export import :SpinSystem;
export import :SpinComponent;
export import :RocketComponent;
export import :RocketSystem;
export import :MainCharacterSystem;

export import Scythe;
export import SandboxGenerated;

export class SandboxGame final : public Game
{
    void DrawDebug(bool* opened);
public:
    SandboxGame();

    void Init() override;
    void Update(double dt) override;

    JobHandle jobHandle;
};
