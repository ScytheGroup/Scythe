module;

#include "PhysicsSystem-Headers.hpp"

export module Systems.Physics:Conversion;

import Common;

export inline Vector4 Convert(const JPH::Vec4& inVec)
{
    return { inVec.GetX(), inVec.GetY(), inVec.GetZ(), inVec.GetW() };
}

export inline JPH::Vec4 Convert(const Vector4& inVec)
{
    return { inVec.x, inVec.y, inVec.z, inVec.w };
}

export inline Vector3 Convert(const JPH::Vec3& inVec)
{
    return { inVec.GetX(), inVec.GetY(), inVec.GetZ() };
}

export inline JPH::Vec3 Convert(const Vector3& inVec)
{
    return { inVec.x, inVec.y, inVec.z };
}

export inline JPH::Quat Convert(const Quaternion& inVec)
{
    return { inVec.x, inVec.y, inVec.z, inVec.w };
}

export inline Quaternion Convert(const JPH::Quat& inVec)
{
    return { inVec.GetX(), inVec.GetY(), inVec.GetZ(), inVec.GetW() };
}

export inline Color Convert(const JPH::Color& inVec)
{
    return Color(Convert(inVec.ToVec4()));
}

export inline JPH::Color Convert(const Color& inVec)
{
    return {
        static_cast<uint8_t>(inVec.R() * 255),
        static_cast<uint8_t>(inVec.G() * 255),
        static_cast<uint8_t>(inVec.B() * 255),
        static_cast<uint8_t>(inVec.A() * 255)
    };
}

export inline JPH::Mat44 Convert(const Transform& inMat)
{
    return JPH::Mat44::sTranslation(Convert(inMat.position))
        * JPH::Mat44::sRotation(Convert(inMat.rotation))
        * JPH::Mat44::sScale(Convert(inMat.scale));
}

export inline JPH::Mat44 Convert(const Matrix& inMat)
{
    JPH::Float4 tab[4]
    {
        JPH::Float4(inMat.m[0][0], inMat.m[0][1], inMat.m[0][2], inMat.m[0][3]),
        JPH::Float4(inMat.m[1][0], inMat.m[1][1], inMat.m[1][2], inMat.m[1][3]),
        JPH::Float4(inMat.m[2][0], inMat.m[2][1], inMat.m[2][2], inMat.m[2][3]),
        JPH::Float4(inMat.m[3][0], inMat.m[3][1], inMat.m[3][2], inMat.m[3][3]),
    };
    
    return JPH::Mat44::sLoadFloat4x4(tab);
}

export inline Matrix Convert(const JPH::Mat44& inMat)
{
    JPH::Float4 data[4]; 
    inMat.StoreFloat4x4(data);
    return Matrix(
        data[0].x, data[0].y, data[0].z, data[0].w,
        data[1].x, data[1].y, data[1].z, data[1].w,
        data[2].x, data[2].y, data[2].z, data[2].w,
        data[3].x, data[3].y, data[3].z, data[3].w
    );
}
