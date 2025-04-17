export module Graphics:DebugShapes;

import Common;

export struct GraphicsScene;

export struct DebugLine
{
    Vector3 start;
    Vector3 end;
    Color color;
};

std::vector<DebugLine> debugLines;

export namespace DebugShapes
{
    void AddDebugCubeHalfExtents(const Vector3& center, const Vector3& halfExtents, const Color& color);
    void AddDebugCubeHalfExtents(const Matrix& transformationMatrix, const Vector3& halfExtents, const Color& color);
    void AddDebugCubeFullExtents(const Vector3& center, const Vector3& extents, const Color& color);
    void AddDebugCubeFullExtents(const Matrix& transformationMatrix, const Vector3& extents, const Color& color);
    void AddDebugSphereRadius(const Vector3 position, float radius, const Color& color);
    void AddDebugArrow(const Vector3& start, const Vector3& end, const Color& color);
    void AddDebugCone(const Vector3& position, const Quaternion& rotation, float range, float innerAngle, float outerAngle, const Color& color);
    void AddDebugFrustumPerspective(const Matrix& inverseViewProjection, const Color& color);
    void AddDebugFrustumOrthographic(const Matrix& inverseViewProjection, const Color& color);
    void AddDebugLine(const Vector3& start, const Vector3& end, const Color& color);
}