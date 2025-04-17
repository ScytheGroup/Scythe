module Graphics:DebugShapes;

import :Scene;

namespace DebugShapes
{

void AddDebugCubeHalfExtents(const Vector3& center, const Vector3& halfExtents, const Color& color)
{
    Vector3 topBackLeft = center + Vector3(-1,1,-1) * halfExtents;
    Vector3 topBackRight = center + Vector3(1,1,-1) * halfExtents;
    Vector3 bottomBackLeft = center + Vector3(-1,-1,-1) * halfExtents;
    Vector3 bottomBackRight = center + Vector3(1,-1,-1) * halfExtents;
    Vector3 topFrontLeft = center + Vector3(-1,1,1) * halfExtents;
    Vector3 topFrontRight = center + Vector3(1,1,1) * halfExtents;
    Vector3 bottomFrontLeft = center + Vector3(-1,-1,1) * halfExtents;
    Vector3 bottomFrontRight = center + Vector3(1,-1,1) * halfExtents;

    debugLines.emplace_back(topBackLeft, topBackRight, color);
    debugLines.emplace_back(topBackRight, topFrontRight, color);
    debugLines.emplace_back(topFrontRight, topFrontLeft, color);
    debugLines.emplace_back(topFrontLeft, topBackLeft, color);
    
    debugLines.emplace_back(bottomBackLeft, bottomBackRight, color);
    debugLines.emplace_back(bottomBackRight, bottomFrontRight, color);
    debugLines.emplace_back(bottomFrontRight, bottomFrontLeft, color);
    debugLines.emplace_back(bottomFrontLeft, bottomBackLeft, color);
    
    debugLines.emplace_back(topBackLeft, bottomBackLeft, color);
    debugLines.emplace_back(topBackRight, bottomBackRight, color);
    debugLines.emplace_back(topFrontRight, bottomFrontRight, color);
    debugLines.emplace_back(topFrontLeft, bottomFrontLeft, color);
}

void AddDebugCubeHalfExtents(const Matrix& transformationMatrix, const Vector3& halfExtents, const Color& color)
{
    Vector3 topBackLeft = Vector3::Transform(Vector3(-1,1,-1) * halfExtents, transformationMatrix);
    Vector3 topBackRight = Vector3::Transform(Vector3(1,1,-1) * halfExtents, transformationMatrix);
    Vector3 bottomBackLeft = Vector3::Transform(Vector3(-1,-1,-1) * halfExtents, transformationMatrix);
    Vector3 bottomBackRight = Vector3::Transform(Vector3(1,-1,-1) * halfExtents, transformationMatrix);
    Vector3 topFrontLeft = Vector3::Transform(Vector3(-1,1,1) * halfExtents, transformationMatrix);
    Vector3 topFrontRight = Vector3::Transform(Vector3(1,1,1) * halfExtents, transformationMatrix);
    Vector3 bottomFrontLeft = Vector3::Transform(Vector3(-1,-1,1) * halfExtents, transformationMatrix);
    Vector3 bottomFrontRight = Vector3::Transform(Vector3(1,-1,1) * halfExtents, transformationMatrix);

    debugLines.emplace_back(topBackLeft, topBackRight, color);
    debugLines.emplace_back(topBackRight, topFrontRight, color);
    debugLines.emplace_back(topFrontRight, topFrontLeft, color);
    debugLines.emplace_back(topFrontLeft, topBackLeft, color);
    
    debugLines.emplace_back(bottomBackLeft, bottomBackRight, color);
    debugLines.emplace_back(bottomBackRight, bottomFrontRight, color);
    debugLines.emplace_back(bottomFrontRight, bottomFrontLeft, color);
    debugLines.emplace_back(bottomFrontLeft, bottomBackLeft, color);
    
    debugLines.emplace_back(topBackLeft, bottomBackLeft, color);
    debugLines.emplace_back(topBackRight, bottomBackRight, color);
    debugLines.emplace_back(topFrontRight, bottomFrontRight, color);
    debugLines.emplace_back(topFrontLeft, bottomFrontLeft, color);
}

void AddDebugCubeFullExtents(const Vector3& center, const Vector3& extents, const Color& color)
{
    AddDebugCubeHalfExtents(center, extents / 2.0f, color);
}

void AddDebugCubeFullExtents(const Matrix& transformationMatrix, const Vector3& extents, const Color& color)
{
    AddDebugCubeHalfExtents(transformationMatrix, extents / 2, color);
}

void AddDebugSphereRadius(const Vector3 position, float radius, const Color& color)
{
    const int segments = 32;
    const float stepAngle = 2.0f * std::numbers::pi_v<float> / static_cast<float>(segments);

    // Draw circles in three principal planes
    for (int i = 0; i < segments; ++i)
    {
        float angle = stepAngle * static_cast<float>(i);
        float nextAngle = stepAngle * static_cast<float>((i + 1) % segments);

        // XY plane circle
        Vector3 currentXY(radius * cos(angle), radius * sin(angle), 0.0f);
        Vector3 nextXY(radius * cos(nextAngle), radius * sin(nextAngle), 0.0f);
        debugLines.emplace_back(position + currentXY, position + nextXY, color);

        // XZ plane circle
        Vector3 currentXZ(radius * cos(angle), 0.0f, radius * sin(angle));
        Vector3 nextXZ(radius * cos(nextAngle), 0.0f, radius * sin(nextAngle));
        debugLines.emplace_back(position + currentXZ, position + nextXZ, color);

        // YZ plane circle
        Vector3 currentYZ(0.0f, radius * cos(angle), radius * sin(angle));
        Vector3 nextYZ(0.0f, radius * cos(nextAngle), radius * sin(nextAngle));
        debugLines.emplace_back(position + currentYZ, position + nextYZ, color);
    }
}

void AddDebugArrow(const Vector3& start, const Vector3& end, const Color& color)
{
    // Arrow body
    debugLines.push_back(DebugLine{ start, end, color });

    // Calculate arrow head
    Vector3 direction = end - start;
    float length = direction.Length();
    direction.Normalize();

    // Arrow head size
    float headLength = length * 0.2f;
    float headRadius = headLength * 0.35f;

    // Create a coordinate system for the arrow head
    Vector3 right = Vector3(1, 0, 0);
    if (abs(direction.Dot(right)) > 0.99f) right = Vector3(0, 1, 0);

    Vector3 up = direction.Cross(right);
    up.Normalize();
    right = up.Cross(direction);
    right.Normalize();

    // Create cone-shaped arrow head
    const int segments = 12; // Number of segments for the cone base
    const float stepAngle = 2.0f * std::numbers::pi_v<float> / static_cast<float>(segments);
    Vector3 headBase = end - direction * headLength;

    // Draw cone base circle
    for (int i = 0; i < segments; ++i)
    {
        float angle = stepAngle * static_cast<float>(i);
        float nextAngle = stepAngle * static_cast<float>((i + 1) % segments);

        Vector3 current = headBase + (right * cos(angle) + up * sin(angle)) * headRadius;
        Vector3 next = headBase + (right * cos(nextAngle) + up * sin(nextAngle)) * headRadius;

        // Draw base circle
        debugLines.push_back(DebugLine{ current, next, color });

        // Draw lines from base to tip
        debugLines.push_back(DebugLine{ current, end, color });
    }
}

void AddDebugCone(const Vector3& position, const Quaternion& rotation, float range, float innerAngle, float outerAngle, const Color& color)
{
    const int segments = 32;
    const int rings = 4;
    const float stepAngle = 2.0f * std::numbers::pi_v<float> / static_cast<float>(segments);
    float outerRadius = range * tan(outerAngle * 0.5f);
    float innerRadius = range * tan(innerAngle * 0.5f);
    Vector3 baseDirection = -Vector3::Forward;

    for (int ring = 0; ring <= rings; ++ring)
    {
        float t = static_cast<float>(ring) / static_cast<float>(rings);
        float height = range * t;
        float outerR = outerRadius * t;
        float innerR = innerRadius * t;

        // Draw outer ring
        for (int i = 0; i < segments; ++i)
        {
            float angle = stepAngle * static_cast<float>(i);
            float nextAngle = stepAngle * static_cast<float>((i + 1) % segments);
            Vector3 right = Vector3::Right;
            Vector3 up = Vector3::Up;

            // Create points in local space
            Vector3 current = baseDirection * height + (right * cos(angle) + up * sin(angle)) * outerR;
            Vector3 next = baseDirection * height + (right * cos(nextAngle) + up * sin(nextAngle)) * outerR;

            // Transform to world space
            current = Vector3::Transform(current, rotation) + position;
            next = Vector3::Transform(next, rotation) + position;

            debugLines.emplace_back(current, next, color);

            if (ring > 0)
            {
                float prevHeight = range * static_cast<float>(ring - 1) / static_cast<float>(rings);
                float prevOuterR = outerRadius * static_cast<float>(ring - 1) / static_cast<float>(rings);
                Vector3 prev = baseDirection * prevHeight + (right * cos(angle) + up * sin(angle)) * prevOuterR;
                prev = Vector3::Transform(prev, rotation) + position;
                debugLines.emplace_back(prev, current, color);
            }
        }

        // Draw inner ring (if inner angle > 0)
        if (innerAngle > 0)
        {
            for (int i = 0; i < segments; ++i)
            {
                float angle = stepAngle * static_cast<float>(i);
                float nextAngle = stepAngle * static_cast<float>((i + 1) % segments);
                Vector3 right = Vector3::Right;
                Vector3 up = Vector3::Up;

                // Create points in local space
                Vector3 current = baseDirection * height + (right * cos(angle) + up * sin(angle)) * innerR;
                Vector3 next = baseDirection * height + (right * cos(nextAngle) + up * sin(nextAngle)) * innerR;

                // Transform to world space
                current = Vector3::Transform(current, rotation) + position;
                next = Vector3::Transform(next, rotation) + position;

                debugLines.emplace_back(current, next, color);

                if (ring > 0)
                {
                    float prevHeight = range * static_cast<float>(ring - 1) / static_cast<float>(rings);
                    float prevInnerR = innerRadius * static_cast<float>(ring - 1) / static_cast<float>(rings);
                    Vector3 prev = baseDirection * prevHeight + (right * cos(angle) + up * sin(angle)) * prevInnerR;
                    prev = Vector3::Transform(prev, rotation) + position;
                    debugLines.emplace_back(prev, current, color);
                }
            }
        }
    }
}

void AddDebugFrustumPerspective(const Matrix& inverseViewProjection, const Color& color)
{
    Vector3 frustumPoints[]
    {
        // Near plane points
        Vector3::Transform(Vector3(-1, -1, -1), inverseViewProjection),
        Vector3::Transform(Vector3(1, -1, -1), inverseViewProjection),
        Vector3::Transform(Vector3(1, 1, -1), inverseViewProjection),
        Vector3::Transform(Vector3(-1, 1, -1), inverseViewProjection),
        // Far plane points
        Vector3::Transform(Vector3(-1, -1, 1), inverseViewProjection),
        Vector3::Transform(Vector3(1, -1, 1), inverseViewProjection),
        Vector3::Transform(Vector3(1, 1, 1), inverseViewProjection),
        Vector3::Transform(Vector3(-1, 1, 1), inverseViewProjection),
    };

    // Near plane
    debugLines.emplace_back(frustumPoints[0], frustumPoints[1], color);
    debugLines.emplace_back(frustumPoints[1], frustumPoints[2], color);
    debugLines.emplace_back(frustumPoints[2], frustumPoints[3], color);
    debugLines.emplace_back(frustumPoints[3], frustumPoints[0], color);

    // Far plane
    debugLines.emplace_back(frustumPoints[4], frustumPoints[5], color);
    debugLines.emplace_back(frustumPoints[5], frustumPoints[6], color);
    debugLines.emplace_back(frustumPoints[6], frustumPoints[7], color);
    debugLines.emplace_back(frustumPoints[7], frustumPoints[4], color);

    // Connect the two planes
    debugLines.emplace_back(frustumPoints[0], frustumPoints[4], color);
    debugLines.emplace_back(frustumPoints[1], frustumPoints[5], color);
    debugLines.emplace_back(frustumPoints[2], frustumPoints[6], color);
    debugLines.emplace_back(frustumPoints[3], frustumPoints[7], color);
}

void AddDebugFrustumOrthographic(const Matrix& inverseViewProjection, const Color& color)
{
    Vector3 frustumPoints[]
    {
        // Near plane points
        Vector3::Transform(Vector3(-1, -1, 0), inverseViewProjection),
        Vector3::Transform(Vector3(1, -1, 0), inverseViewProjection),
        Vector3::Transform(Vector3(1, 1, 0), inverseViewProjection),
        Vector3::Transform(Vector3(-1, 1, 0), inverseViewProjection),
        // Far plane points
        Vector3::Transform(Vector3(-1, -1, 1), inverseViewProjection),
        Vector3::Transform(Vector3(1, -1, 1), inverseViewProjection),
        Vector3::Transform(Vector3(1, 1, 1), inverseViewProjection),
        Vector3::Transform(Vector3(-1, 1, 1), inverseViewProjection),
    };

    // Near plane
    debugLines.emplace_back(frustumPoints[0], frustumPoints[1], color);
    debugLines.emplace_back(frustumPoints[1], frustumPoints[2], color);
    debugLines.emplace_back(frustumPoints[2], frustumPoints[3], color);
    debugLines.emplace_back(frustumPoints[3], frustumPoints[0], color);

    // Far plane
    debugLines.emplace_back(frustumPoints[4], frustumPoints[5], color);
    debugLines.emplace_back(frustumPoints[5], frustumPoints[6], color);
    debugLines.emplace_back(frustumPoints[6], frustumPoints[7], color);
    debugLines.emplace_back(frustumPoints[7], frustumPoints[4], color);

    // Connect the two planes
    debugLines.emplace_back(frustumPoints[0], frustumPoints[4], color);
    debugLines.emplace_back(frustumPoints[1], frustumPoints[5], color);
    debugLines.emplace_back(frustumPoints[2], frustumPoints[6], color);
    debugLines.emplace_back(frustumPoints[3], frustumPoints[7], color);
}

void AddDebugLine(const Vector3& start, const Vector3& end, const Color& color)
{
    debugLines.push_back(DebugLine{ start, end, color });
}

}