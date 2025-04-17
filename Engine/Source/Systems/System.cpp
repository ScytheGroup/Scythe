module Systems;
import Common;
import Core;

namespace
{
    void AssertEcs(const EntityComponentSystem* ecs)
    {
        Assert(ecs, "Attempt to fetch ecs out of system initialization scope. This may be caused by an attempt to fetch it at construction");
    }
}

EntityComponentSystem& System::GetEcs()
{
    AssertEcs(ecs);
    return *ecs;
}

const EntityComponentSystem& System::GetEcs() const
{
    AssertEcs(ecs);
    return *ecs;
}