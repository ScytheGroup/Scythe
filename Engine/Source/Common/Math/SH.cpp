module Common:SH;
import :SH;

import <numbers>;

// Basis for first three bands of a SH
static const float Base0 = 1.0f / sqrt(std::numbers::pi_v<float>) * 0.5f;
static const float Base1 = sqrt(3.0f / std::numbers::pi_v<float>) * 0.5f;
static const float Base2_0 = sqrt(15.0f / std::numbers::pi_v<float>) * 0.5f;
static const float Base2_1 = sqrt(5.0f / std::numbers::pi_v<float>) * 0.25f;
static const float Base2_2 = sqrt(15.0f / std::numbers::pi_v<float>) * 0.25f;

// SH coefficients for Lambertian BRDF AKA clamped cosine lobe
static const float A0 = sqrt(std::numbers::pi_v<float>) / 2.0f;
static const float A1 = sqrt(std::numbers::pi_v<float> / 3.0f);
static const float A2 = sqrt(5.0f * std::numbers::pi_v<float> / 4.0f) / 4.0f;

static constexpr float HatA0 = std::numbers::pi_v<float>;
static constexpr float HatA1 = 2 * std::numbers::pi_v<float> / 3.0f;
static constexpr float HatA2 = std::numbers::pi_v<float> / 4.0f;

ScalarSH9 MakeSH9(Vector3& normal)
{
    ScalarSH9 result;
    
    // Band 0
    result[0] = Base0;
 
    // Band 1
    result[1] = Base1 * normal.y;
    result[2] = Base1 * normal.z;
    result[3] = Base1 * normal.x;
 
    // Band 2
    result[4] = Base2_0 * normal.x * normal.y;
    result[5] = Base2_0 * normal.y * normal.z;
    result[6] = Base2_1 * (3.0f * normal.z * normal.z - 1.0f);
    result[7] = Base2_0 * normal.x * normal.z;
    result[8] = Base2_2 * (normal.x * normal.x - normal.y * normal.y);
 
    return result;
}

void ColorSH9::Reduce()
{
    for (int i = 1; i < 9; ++i)
    {
        r[i] /= r[0];
        g[i] /= g[0];
        b[i] /= b[0];
    }
}

void ColorSH9::ApplyClampedCosineLobe()
{
    r[0] *= A0;
    g[0] *= A0;
    b[0] *= A0;

    r[1] *= A1;
    r[2] *= A1;
    r[3] *= A1;
    g[1] *= A1;
    g[2] *= A1;
    g[3] *= A1;
    b[1] *= A1;
    b[2] *= A1;
    b[3] *= A1;

    r[4] *= A2;
    r[5] *= A2;
    r[6] *= A2;
    r[7] *= A2;
    r[8] *= A2;
    g[4] *= A2;
    g[5] *= A2;
    g[6] *= A2;
    g[7] *= A2;
    g[8] *= A2;
    b[4] *= A2;
    b[5] *= A2;
    b[6] *= A2;
    b[7] *= A2;
    b[8] *= A2;
}

ColorSH9 ProjectSH9(Vector3 normal, const Color& color)
{
    normal.Normalize();
    ScalarSH9 shBasis = MakeSH9(normal);
    ColorSH9 result;
    for (int i = 0; i < 9; ++i)
    {
        result.r[i] = color.R() * shBasis[i];
        result.g[i] = color.G() * shBasis[i];
        result.b[i] = color.B() * shBasis[i];
    }
    return result;
}
