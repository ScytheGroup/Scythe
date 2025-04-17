module;

#include "Serialization/SerializationMacros.h"
#include "ThirdParty/DirectXMath/XDSP/XDSP.h"

module Common:Transform;

import Core;
import Serialization;
import :Transform;
import <numbers>;

namespace
{
    bool HasNegativeScale(const Vector3& scale, const Vector3& otherScale)
    {
        return XMVector3LessOrEqual(scale, DirectX::XMVectorZero()) || XMVector3LessOrEqual(otherScale, DirectX::XMVectorZero());
    }

    template <class T>
    requires std::is_floating_point_v<T>
    Vector3 GetReciprocalSafe(const Vector3& inVec, T tolerance)
    {
        Vector3 result = XMVectorReciprocal(inVec);
        
        if (std::abs(inVec.x) <= tolerance)
            result.x = 0;
        if (std::abs(inVec.y) <= tolerance)
            result.y = 0;
        if (std::abs(inVec.z) <= tolerance)
            result.z = 0;

        return result;
    }
}

Transform Transform::Multiply(const Transform& a, const Transform& b)
{
#ifdef SC_DEV_VERSION
    Assert(a.IsRotationNormalized());
    Assert(b.IsRotationNormalized());
#endif
    
    // check if negative scale
    if (HasNegativeScale(a.scale, b.scale))
    {
        return MultiplyUsingMatrixWithScale(a, b);
    }

    // else
    const Quaternion quatA = a.rotation;
    const Quaternion quatB = b.rotation;
    const Vector3 translateA = a.position;
    const Vector3 translateB = b.position;
    const Vector3 scaleA = a.scale;
    const Vector3 scaleB = b.scale;

    Transform out;
    out.rotation =  XMQuaternionMultiply(quatA, quatB);
    
    const Vector3 scaledTransA = translateA * scaleB;
    const Vector3 rotatedTranslate = XMVector3Rotate(scaledTransA, quatB);
    out.position = XMVectorAdd(rotatedTranslate, translateB);
    out.scale = XMVectorMultiply(scaleA, scaleB);

    return out;
}

Transform Transform::MultiplyUsingMatrixWithScale(const Transform& a, const Transform& b)
{
    return ConstructFromMatricesAndScale(a.ToMatrixWithScale(), b.ToMatrixWithScale(), a.scale * b.scale);
}

Matrix Transform::ToMatrixWithScale() const noexcept
{
    return Matrix::CreateScale(scale) * Matrix::CreateFromQuaternion(rotation) * Matrix::CreateTranslation(position);
}

Matrix Transform::ToMatrixNoScale() const noexcept
{
    return Matrix::CreateFromQuaternion(rotation) * Matrix::CreateTranslation(position);
}

Vector3 Transform::GetForwardVector() const noexcept { return Vector3::Transform(Vector3::Forward, rotation); }

Vector3 Transform::GetUpVector() const noexcept
{
    Assert(IsRotationNormalized());
    return Vector3::Transform(Vector3::Up, rotation);
}

Vector3 Transform::GetRightVector() const noexcept { return Vector3::Transform(Vector3::Right, rotation); }

Transform Transform::Inverse() const noexcept
{
    Quaternion invRot;
    rotation.Inverse(invRot);

    const Vector3 invScale = XMVectorReciprocal(scale);

    // invert the translation
    const Vector3 scaledTranslation = invScale * position;
    const Vector3 t2 = XMVector3Rotate(scaledTranslation, invRot);
    const Vector3 invTranslation = -t2;

    return { invTranslation, invRot, invScale };
}

void Transform::LookAt(const Vector3& eye, const Vector3& target, const Vector3& up)
{
    position = eye;
    LookAt(target, up);
}

void Transform::LookAt(const Vector3& target, const Vector3& up)
{
    const Matrix mat = Matrix::CreateLookAt(position, target, up);
    Quaternion quat = Quaternion::CreateFromRotationMatrix(mat);
    quat.Inverse(quat);
    rotation = quat;
}

void Transform::LookTowards(const Vector3& direction, const Vector3& up)
{
    LookAt(position + direction, up);
}

void Transform::Rotate(const int dx, const int dy)
{
    const Quaternion rotx = Quaternion::CreateFromAxisAngle(Vector3::UnitY, -dx / 1000.0f);
    const Quaternion roty = Quaternion::CreateFromAxisAngle(GetRightVector(), -dy / 1000.0f);
    rotation *= roty * rotx;
    rotation.Normalize();
}

void Transform::Rotate(const Quaternion& rotation) noexcept
{
    this->rotation *= XMQuaternionNormalize(rotation);
}

void Transform::RotateTo(const Vector3& eulerAngles) {
    // Convert Euler angles (in radians)
    auto targetRotation = Quaternion::CreateFromYawPitchRoll(eulerAngles.y, eulerAngles.x, eulerAngles.z);

    // Normalize current and target rotations
    rotation.Normalize();
    targetRotation.Normalize();
    
    // Use spherical linear interpolation to prevent gimbal lock
    rotation = Quaternion::Slerp(rotation, targetRotation, 1.0f);
}

bool Transform::IsRotationNormalized() const
{
    const auto TestValue = DirectX::XMVectorAbs(DirectX::XMVectorSubtract(Vector3::One, DirectX::XMVector4Dot(rotation, rotation)));
    return !DirectX::XMVector4Greater(TestValue, Quaternion{QuaternionNormalizedTreshhold, QuaternionNormalizedTreshhold, QuaternionNormalizedTreshhold, QuaternionNormalizedTreshhold});
}

std::tuple<Vector3, Vector3, Vector3> Transform::GetAxis() const noexcept
{
    return { GetUpVector(), GetRightVector(), GetForwardVector() };
}

Transform Transform::CreateLookAt(const Vector3& eye, const Vector3& target, const Vector3& up)
{
    Transform t;
    t.LookAt(eye, target, up);
    return t;
}

Transform::Transform(const Matrix& transformationMat)
{
    Matrix copy = transformationMat;
    Assert(copy.Decompose(scale, rotation, position));
}

Transform::Transform(const Vector3& position, const Quaternion& rotation, const Vector3& scale)
    : position(position)
    , rotation(rotation)
    , scale(scale)
{}

Transform Transform::ConstructFromMatricesAndScale(const Matrix& mat1, const Matrix& mat2, Vector3 desiredScale)
{
    using namespace Math::Matrices;
    Transform result;
    Matrix mat = mat1 * mat2;
    RemoveScaling(mat);

    Vector3 signedScale = desiredScale;
    
    SetAxis(mat, 0, signedScale.x * Math::Matrices::GetAxis(mat, Math::Matrices::Axis::X));
    SetAxis(mat, 1, signedScale.y * Math::Matrices::GetAxis(mat, Math::Matrices::Axis::Y));
    SetAxis(mat, 2, signedScale.z * Math::Matrices::GetAxis(mat, Math::Matrices::Axis::Z));

    Quaternion rot = Quaternion::CreateFromRotationMatrix(mat);
    rot.Normalize();

    result.scale = desiredScale;
    result.rotation = rot;

    result.position = GetOrigin(mat);
    return result;
}

Transform MakeTransformScreenSpaceSizedBillboard(Transform objectTransform, Vector3 cameraPosition, float fovDeg, Vector2 minScreenSpaceSize, Vector2 screenSize)
{
    Vector3 pos = objectTransform.position;
    objectTransform.rotation = Quaternion::CreateFromRotationMatrix(Matrix::CreateLookAt(pos, cameraPosition, objectTransform.GetUpVector()).Invert());
    float distanceToObject = Vector3::Distance(cameraPosition, pos);
    const float maxConstantSizeDistance = 200.0f; // After this distance, object will scale with perspective

    float effectiveDistance = distanceToObject;
    if (distanceToObject > maxConstantSizeDistance)
    {
        // Scale quadratically beyond max distance to match perspective scaling
        float ratio = maxConstantSizeDistance / distanceToObject;
        effectiveDistance = maxConstantSizeDistance * ratio;
    }

    float screenHeightInWorldUnits = 2.0f * tanf(fovDeg * 0.5f * (std::numbers::pi_v<float> / 180.0f)) * effectiveDistance;
    Vector2 desiredWorldSize = (minScreenSpaceSize / screenSize) * screenHeightInWorldUnits;

    Vector2 scaleFactors;
    scaleFactors.x = desiredWorldSize.x / objectTransform.scale.x;
    scaleFactors.y = desiredWorldSize.y / objectTransform.scale.y;

    float scaleFactor = std::max(scaleFactors.x, scaleFactors.y);
    objectTransform.scale.x *= scaleFactor;
    objectTransform.scale.y *= scaleFactor;

    return objectTransform;
}

Transform MakeTransformBillboard(Transform objectTransform, Vector3 cameraPosition)
{
    Vector3 pos = objectTransform.position;
    objectTransform.rotation = Quaternion::CreateFromRotationMatrix(Matrix::CreateLookAt(pos, cameraPosition, objectTransform.GetUpVector()).Invert());
    return objectTransform;
}

bool Transform::operator==(const Transform& other) const
{
    return std::tie(position, scale, rotation) == std::tie(other.position, other.scale, other.rotation);
}

Transform Transform::GetRelativeTransform(const Transform& other) const
{
    Transform result;

    if (!other.IsRotationNormalized())
        return Transform{};
    
    if (HasNegativeScale(scale, other.scale))
    {
        return MultiplyUsingMatrixWithScale(*this, other);
    }
    
    Vector3 safeScale = GetReciprocalSafe(other.scale, FloatSmallNumber);
    
    Vector3 desiredScale = scale * safeScale;

    Vector3 translation = position - other.position;

    Quaternion invRot = XMQuaternionInverse(other.rotation);

    Vector3 vr = XMVector3Rotate(translation, invRot);
    Vector3 vtranslation = vr * safeScale;

    Quaternion vrotation = rotation * invRot;

    result.position = vtranslation;
    result.rotation = vrotation;
    result.scale = desiredScale;

    return result;
}

Transform Transform::GetRelativeTransformReverse(const Transform& other) const
{
    return other.GetRelativeTransform({*this});
}

void Transform::SetToRelativeTransform(const Transform& parent)
{
    *this = GetRelativeTransform(parent);
}

Transform Transform::GetRelativeTransformUsingMatrixWithScale(const Transform& base, const Transform& relative)
{
    Matrix A = base.ToMatrixWithScale();
    Matrix B = relative.ToMatrixWithScale();

    Vector3 scale = DirectX::XMVectorReciprocal(relative.scale);
    Vector3 desiredScale = base.scale * scale;

    return ConstructFromMatricesAndScale(A, B.Invert(), desiredScale);
}


DEFINE_SERIALIZATION_AND_DESERIALIZATION(Transform, position, rotation, scale)

#ifdef IMGUI_ENABLED

import ImGui;
import <format>;

bool Transform::ImGuiDraw()
{
    bool result = false;
    if (ImGui::BeginTable("table", 2))
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        ScytheGui::Header("Transform", ImGui::headerFont);

        ImGui::TreePush("indent");

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Position");
        ImGui::TableNextColumn();
        result |= ImGui::DragFloat3("##Position", &position.x, 0.5, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max());

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Scale");
        ImGui::TableNextColumn();
        result |= ImGui::DragFloat3("##Scale", &scale.x, 0.05, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max());

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Rotation");
        ImGui::TableNextColumn();
        Vector3 euler = rotation.ToEuler();
        ImGui::Text(std::format("Yaw: {}, Pitch: {}, Roll: {}", euler.y, euler.x, euler.z).c_str());

        ImGui::TreePop();
        ImGui::EndTable();
    }
    return result;
}

#endif