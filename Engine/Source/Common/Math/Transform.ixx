module;

export module Common:Transform;

import :Math;

export class Editor;

export struct Transform
{
    // ScytheEngine uses right-handed chirality
    // up = Y
    // right = X
    // forward = -Z
    Vector3 position = Vector3::Zero; // X, Y, Z
    Quaternion rotation = Quaternion::Identity; // Quaternion
    Vector3 scale = Vector3::One; // X, Y, Z

    Transform() = default;
    Transform(const Matrix& transformationMat);
    Transform(const Vector3& position, const Quaternion& rotation, const Vector3& scale);
    static Transform ConstructFromMatricesAndScale(const Matrix& mat1, const Matrix& mat2, Vector3 desiredScale);
    
    static Transform Multiply(const Transform& a, const Transform& b);

    Transform GetRelativeTransform(const Transform& other) const;
    Transform GetRelativeTransformReverse(const Transform& other) const;
    void SetToRelativeTransform(const Transform& parent);
    
    void Translate(const Vector3& translation) noexcept { position += translation; }
    void Scale(const Vector3& scaling) noexcept { scale *= scaling; }
    
    Vector3 GetForwardVector() const noexcept;
    Vector3 GetUpVector() const noexcept;
    Vector3 GetRightVector() const noexcept;
    
    Transform Inverse() const noexcept;
    
    void LookAt(const Vector3& eye, const Vector3& target, const Vector3& up);
    void LookAt(const Vector3& target, const Vector3& up);

    void LookTowards(const Vector3& direction, const Vector3& up);

    void Rotate(const int dx, const int dy);
    void Rotate(const Quaternion& rotation) noexcept;
    void RotateTo(const Vector3& eulerAngles);

    bool IsRotationNormalized() const;
    
    // Better when the three are needed { up, right, forward }
    std::tuple<Vector3, Vector3, Vector3> GetAxis() const noexcept;

    static Transform CreateLookAt(const Vector3& eye, const Vector3& target, const Vector3& up);

    bool ImGuiDraw();

    bool operator==(const Transform&) const;
    Transform operator*(const Transform& other) const
    {
        return Multiply(*this, other);
    }
    
    Transform& operator*=(const Transform& other)
    {
        *this = Multiply(*this, other);
        return *this;
    }

    // Matrices
    Matrix ToMatrixWithScale() const noexcept;
    Matrix ToMatrixNoScale() const noexcept;
    static Transform MultiplyUsingMatrixWithScale(const Transform& a, const Transform& b);
    static Transform GetRelativeTransformUsingMatrixWithScale(const Transform& base, const Transform& relative);
};

export Transform MakeTransformScreenSpaceSizedBillboard(Transform objectTransform, Vector3 cameraPosition, float fovDeg, Vector2 minScreenSpaceSize, Vector2 screenSize);
export Transform MakeTransformBillboard(Transform objectTransform, Vector3 cameraPosition);

