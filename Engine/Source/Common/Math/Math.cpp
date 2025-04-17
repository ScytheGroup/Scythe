module;

#include "Serialization/SerializationMacros.h"

module Common:Math;

import Common;

float DegToRad(float degrees)
{
    return degrees / 180.0f * DirectX::XM_PI;
}

float RadToDeg(float radians)
{
    return radians / DirectX::XM_PI * 180.0f;
}

Vector3 DegToRad(const Vector3& degrees)
{
    Vector3 result;
    result.x = DegToRad(degrees.x);
    result.y = DegToRad(degrees.y);
    result.z = DegToRad(degrees.z);
    return result;
}

Vector3 RadToDeg(const Vector3& degrees)
{
    Vector3 result;
    result.x = RadToDeg(degrees.x);
    result.y = RadToDeg(degrees.y);
    result.z = RadToDeg(degrees.z);
    return result;
}

uint32_t RoundUpToNextPow2(uint32_t x)
{
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
}

Matrix ConvertDirectionVectorToRotationMatrix(Vector3 forward)
{
    Vector3 WorldUp = Vector3::UnitY;
    forward.Normalize();
    
    // Make sure side vector is valid (direction could be close to WorldUp)
    Vector3 right = forward.Cross(WorldUp);
    if (right.LengthSquared() < 0.001f)
    {
        Vector3 WorldRight = Vector3::Right;
        right = forward.Cross(WorldRight);
    }
    right.Normalize();

    Vector3 up = forward.Cross(right);
    return Matrix(forward, right, up);
}

constexpr uint32_t RaiseToNextMultipleOf(uint32_t val, uint32_t multiple)
{
    if (val == 0)
        return 0;

    return val + (multiple - val % multiple);
}
