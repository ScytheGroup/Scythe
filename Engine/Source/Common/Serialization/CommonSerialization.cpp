#include "Serialization/SerializationMacros.h"

import Common;
import Core;

template<>
ArchiveStream& operator<<(ArchiveStream& os, const std::filesystem::path& path)
{
    os << path.string();
    return os;
}

template<>
ArchiveStream& operator<<(ArchiveStream& ar, const Vector2& v)
{
    ar << v.x << v.y;
    return ar;
}

template<>
ArchiveStream& operator>>(ArchiveStream& ar, Vector2& v)
{
    ar >> v.x >> v.y;
    return ar;
}

template<>
ArchiveStream& operator<<(ArchiveStream& ar, const Vector3& v)
{
    ar << v.x << v.y << v.z;
    return ar;
}

template<>
ArchiveStream& operator>>(ArchiveStream& ar, Vector3& v)
{
    ar >> v.x >> v.y >> v.z;
    return ar;
}


template<>
ArchiveStream& operator<<(ArchiveStream& ar, const Quaternion& v)
{
    ar << v.x << v.y << v.z << v.w;
    return ar;
}

template<>
ArchiveStream& operator>>(ArchiveStream& ar, Quaternion& v)
{
    ar >> v.x >> v.y >> v.z >> v.w;
    return ar;
}

template<>
ArchiveStream& operator<<(ArchiveStream& ar, const Color& v)
{
    ar << v.x << v.y << v.z << v.w;
    return ar;
}

template<>
ArchiveStream& operator>>(ArchiveStream& ar, Color& v)
{
    ar >> v.x >> v.y >> v.z >> v.w;
    return ar;
}

BEGIN_DESERIALIZATION(Vector4)
    std::vector<float> colorVec;
    PARSE_VARIABLE(colorVec);
    PARSED_VAL = Vector4{ colorVec.data() };
END_DESERIALIZATION()

BEGIN_SERIALIZATION(Vector4)
    json[0] = SER_VAL.x;
    json[1] = SER_VAL.y;
    json[2] = SER_VAL.z;
    json[3] = SER_VAL.w;
END_SERIALIZATION()

BEGIN_DESERIALIZATION(Vector3)
    std::vector<float> colorVec;
    PARSE_VARIABLE(colorVec);
    PARSED_VAL = Vector3{ colorVec.data() };
END_DESERIALIZATION()

BEGIN_SERIALIZATION(Vector3)
    json[0] = SER_VAL.x;
    json[1] = SER_VAL.y;
    json[2] = SER_VAL.z;
END_SERIALIZATION()

BEGIN_DESERIALIZATION(Vector2)
    std::vector<float> colorVec;
    PARSE_VARIABLE(colorVec);
    PARSED_VAL = Vector2{ colorVec.data() };
END_DESERIALIZATION()

BEGIN_SERIALIZATION(Vector2)
    json[0] = SER_VAL.x;
    json[1] = SER_VAL.y;
END_SERIALIZATION()

BEGIN_DESERIALIZATION(Color)
    std::vector<float> colorVec;
    PARSE_VARIABLE(colorVec);
    PARSED_VAL = Color{ colorVec.data() };
END_DESERIALIZATION()

BEGIN_SERIALIZATION(Color)
    json[0] = SER_VAL.x;
    json[1] = SER_VAL.y;
    json[2] = SER_VAL.z;
    json[3] = SER_VAL.w;
END_SERIALIZATION()

BEGIN_DESERIALIZATION(Quaternion)
    std::vector<float> colorVec;
    PARSE_VARIABLE(colorVec);
    PARSED_VAL = Quaternion{ colorVec.data() };
END_DESERIALIZATION()

BEGIN_SERIALIZATION(Quaternion)
    json[0] = SER_VAL.x;
    json[1] = SER_VAL.y;
    json[2] = SER_VAL.z;
    json[3] = SER_VAL.w;
END_SERIALIZATION()
