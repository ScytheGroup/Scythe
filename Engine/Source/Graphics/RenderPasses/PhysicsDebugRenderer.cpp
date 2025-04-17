module Graphics:PhysicsDebugRenderer;
#ifdef SC_DEV_VERSION

import :PhysicsDebugRenderer;
import :DebugShapes;

import Common;

void PhysicsDebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor)
{
    DebugShapes::AddDebugLine(Convert(inFrom), Convert(inTo), Convert(inColor));
}

void PhysicsDebugRenderer::DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight)
{
    // TODO: Add this if we ever have text in our engine
}
#endif