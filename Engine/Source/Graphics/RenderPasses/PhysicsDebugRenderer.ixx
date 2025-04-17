export module Graphics:PhysicsDebugRenderer;
#ifdef SC_DEV_VERSION

import Systems.Physics;

export class PhysicsDebugRenderer : public JPH::DebugRendererSimple
{
public:
    void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;
    void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) override;
};
#endif