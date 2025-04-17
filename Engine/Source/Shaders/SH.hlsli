#ifndef _SH_
#define _SH_

////////////////////////////////////////////////////////////////////////
//                        Spherical Harmonics
// References:
// [1] https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/gdc2018-precomputedgiobalilluminationinfrostbite.pdf
////////////////////////////////////////////////////////////////////////

#include "Constants.hlsli"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SH Constants
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Basis for first three bands of a SH
static const float Base0 = 1.0f / sqrt(PI) * 0.5f;
static const float Base1 = sqrt(3.0f / PI) * 0.5f;
static const float Base2_0 = sqrt(15.0f / PI) * 0.5f;
static const float Base2_1 = sqrt(5.0f / PI) * 0.25f;
static const float Base2_2 = sqrt(15.0f / PI) * 0.25f;

// SH coefficients for Lambertian BRDF AKA clamped cosine lobe
static const float A0 = sqrt(PI) / 2.0f;
static const float A1 = sqrt(PI / 3.0f);
static const float A2 = sqrt(5.0f * PI / 4.0f) / 4.0f;

static const float HatA0 = PI;
static const float HatA1 = 2 * PI / 3;
static const float HatA2 = PI / 4;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Second order SH (4 coefficients)
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct ScalarSH
{
    float4 v;
};

ScalarSH ZeroSH()
{
    return (ScalarSH) 0;
}

ScalarSH EvaluateSH(float3 direction)
{
    ScalarSH sh;
    sh.v.x = Base0;
    // I've inverted y and w here to make more sense (xyz for second order makes more sense than yzx imo)
    // This can cause some differences in other functions compared to what is found online
    sh.v.zwy = Base1 * direction;
    return sh;
}

ScalarSH SumSH(ScalarSH a, ScalarSH b)
{
    ScalarSH sh;
    sh.v = a.v + b.v;
    return sh;
}

ScalarSH ScaleSH(ScalarSH sh, float scaleFactor)
{
    sh.v *= scaleFactor;
    return sh;
}

float ConvolveSH(ScalarSH a, ScalarSH b)
{
    return dot(a.v, b.v);
}

ScalarSH ProjectSH(float value, float3 direction)
{
    ScalarSH sh = EvaluateSH(direction);
    sh.v *= value;
    return sh;
}

float UnprojectSH(ScalarSH sh, float3 direction)
{
    return ConvolveSH(sh, EvaluateSH(direction));
}

ScalarSH CosineLobeSH(float3 normal)
{
    // From [1]
    // SH coefficients for Lambertian BRDF AKA clamped cosine lobe
    static const float A0 = PI / sqrt(4 * PI);
    static const float A1 = sqrt(PI / 3);
    ScalarSH sh;
    sh.v.x = A0;
    sh.v.zwy = A1 * normal;
    return sh;
}

float ReconstructIrradiance(ScalarSH sh, float3 normal)
{
    return ConvolveSH(sh, CosineLobeSH(normal));
}

ScalarSH EvaluateDiffuseSH(float3 color)
{
    // From [1]
    // Combination of EvaluateSH and Convolve(sh, CosineLobeConvolveSH(float3(1, 1, 1)))
    static const float A0 = 0.25f;
    static const float A1 = 0.50f;
    ScalarSH sh;
    sh.v.x = A0;
    sh.v.zwy = A1 * color;
    return sh;
}

// Simply three ScalarSH to represent irradiance
struct ColorSH
{
    ScalarSH r;
    ScalarSH g;
    ScalarSH b;
};

ColorSH ZeroColorSH()
{
    return (ColorSH) 0;
}

ColorSH ScaleSH(ColorSH a, float scaleFactor)
{
    ColorSH sh;
    sh.r = ScaleSH(a.r, scaleFactor);
    sh.g = ScaleSH(a.g, scaleFactor);
    sh.b = ScaleSH(a.b, scaleFactor);
    return sh;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Third order SH (9 coefficients)
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct ScalarSH9 
{
    float v[9];
};

ScalarSH9 ZeroSH9()
{
    return (ScalarSH9) 0;
}

float ConvolveSH9(ScalarSH9 a, ScalarSH9 b)
{
    return 
        a.v[0] * b.v[0] +
        a.v[1] * b.v[1] +
        a.v[2] * b.v[2] +
        a.v[3] * b.v[3] +
        a.v[4] * b.v[4] +
        a.v[5] * b.v[5] +
        a.v[6] * b.v[6] +
        a.v[7] * b.v[7] +
        a.v[8] * b.v[8];
}

ScalarSH9 CosineLobeSH9(float3 direction)
{
    // From [1]
    ScalarSH9 sh = ZeroSH9();
    
    // Band 0
    sh.v[0] = Base0 * HatA0;

    // Band 1
    sh.v[1] = Base1 * HatA1 * direction.y;
    sh.v[2] = Base1 * HatA1 * direction.z;
    sh.v[3] = Base1 * HatA1 * direction.x;

    // Band 2
    sh.v[4] = Base2_0 * HatA2 * direction.x * direction.y;
    sh.v[5] = Base2_0 * HatA2 * direction.y * direction.z;
    sh.v[6] = Base2_1 * HatA2 * (3.0f * direction.z * direction.z - 1.0f);
    sh.v[7] = Base2_0 * HatA2 * direction.x * direction.z;
    sh.v[8] = Base2_2 * HatA2 * (direction.x * direction.x - direction.y * direction.y);

    return sh;
}

float ReconstructIrradiance(ScalarSH9 sh, float3 normal)
{
    return max(0, ConvolveSH9(sh, CosineLobeSH9(normal)));
}

ScalarSH9 ExpandSH9(ScalarSH9 sh)
{
    sh.v[1] *= sh.v[0];
    sh.v[2] *= sh.v[0];
    sh.v[3] *= sh.v[0];
    sh.v[4] *= sh.v[0];
    sh.v[5] *= sh.v[0];
    sh.v[6] *= sh.v[0];
    sh.v[7] *= sh.v[0];
    sh.v[8] *= sh.v[0];
    return sh;
}

ScalarSH9 ScaleSH9(ScalarSH9 a, float scaleFactor)
{
    ScalarSH9 sh = a;
    sh.v[0] *= scaleFactor;
    sh.v[1] *= scaleFactor;
    sh.v[2] *= scaleFactor;
    sh.v[3] *= scaleFactor;
    sh.v[4] *= scaleFactor;
    sh.v[5] *= scaleFactor;
    sh.v[6] *= scaleFactor;
    sh.v[7] *= scaleFactor;
    sh.v[8] *= scaleFactor;
    return sh;
}

ScalarSH9 SumSH9(ScalarSH9 a, ScalarSH9 b)
{
    ScalarSH9 sh;
    sh.v[0] = a.v[0] + b.v[0];
    sh.v[1] = a.v[1] + b.v[1];
    sh.v[2] = a.v[2] + b.v[2];
    sh.v[3] = a.v[3] + b.v[3];
    sh.v[4] = a.v[4] + b.v[4];
    sh.v[5] = a.v[5] + b.v[5];
    sh.v[6] = a.v[6] + b.v[6];
    sh.v[7] = a.v[7] + b.v[7];
    sh.v[8] = a.v[8] + b.v[8];
    return sh;
}

struct ColorSH9
{
    ScalarSH9 r;
    ScalarSH9 g;
    ScalarSH9 b;
};

ColorSH9 ZeroColorSH9()
{
    return (ColorSH9) 0;
}

float3 ReconstructIrradiance(ColorSH9 sh, float3 normal)
{
    return float3(
        ReconstructIrradiance(sh.r, normal),
        ReconstructIrradiance(sh.g, normal),
        ReconstructIrradiance(sh.b, normal)
    );
}

ColorSH9 ExpandSH9(ColorSH9 sh)
{
    sh.r = ExpandSH9(sh.r);
    sh.g = ExpandSH9(sh.g);
    sh.b = ExpandSH9(sh.b);
    return sh;
}

ScalarSH9 MakeSH9(float3 normal)
{
    ScalarSH9 result;
    
    // Band 0
    result.v[0] = Base0;
 
    // Band 1
    result.v[1] = Base1 * normal.y;
    result.v[2] = Base1 * normal.z;
    result.v[3] = Base1 * normal.x;
 
    // Band 2
    result.v[4] = Base2_0 * normal.x * normal.y;
    result.v[5] = Base2_0 * normal.y * normal.z;
    result.v[6] = Base2_1 * (3.0f * normal.z * normal.z - 1.0f);
    result.v[7] = Base2_0 * normal.x * normal.z;
    result.v[8] = Base2_2 * (normal.x * normal.x - normal.y * normal.y);
 
    return result;
}

ColorSH9 ProjectSH9(float3 normal, float3 color)
{
    normal = normalize(normal);
    ScalarSH9 shBasis = MakeSH9(normal);
    ColorSH9 result;
    for (int i = 0; i < 9; ++i)
    {
        result.r.v[i] = color.r * shBasis.v[i];
        result.g.v[i] = color.g * shBasis.v[i];
        result.b.v[i] = color.b * shBasis.v[i];
    }
    return result;
}

ColorSH9 MultiplySH9(ColorSH9 sh, float scalar)
{
    ColorSH9 ret;
    ret.r = ScaleSH9(sh.r, scalar);
    ret.g = ScaleSH9(sh.g, scalar);
    ret.b = ScaleSH9(sh.b, scalar);
    return ret;
}

ColorSH9 SumSH9(ColorSH9 a, ColorSH9 b)
{
    ColorSH9 ret;
    ret.r = SumSH9(a.r, b.r);
    ret.g = SumSH9(a.g, b.g);
    ret.b = SumSH9(a.b, b.b);
    return ret;
}

#endif // _SH_