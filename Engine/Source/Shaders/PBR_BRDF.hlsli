#ifndef _PBR_BRDF_
#define _PBR_BRDF_

#include "Constants.hlsli"

////////////////////////////////////////////////
//             PBR BRDF Functions
//
// References:
// [1] Google Filament
// [2] https://seblagarde.wordpress.com/2011/08/17/hello-world/
//
// Following section contains functions that compute each term for the Specular BRDF:
// 
//             D(h, a) * G(v, l, a) * F(v, h, f0)
// f_r(v, l) = -----------------------------------
//                       4(n.v)(n.l)
//
////////////////////////////////////////////////

////////////////////////////////////////////////
// Normal Distribution Function (D)
////////////////////////////////////////////////

// GGX NDF
// From Walter et al. 2007, "Microfacet Models for Refraction through Rough Surfaces"
float D_GGX(float NoH, float roughness)
{
    float alpha = roughness * roughness;
    float a = NoH * alpha;
    float k = alpha / (1.0 - NoH * NoH + a * a);
    return k * k * PI_RCP;
}

// Alternative to GGX that produces a smoother highlight at the extreme edges of objects
// Also from Walter et al. 2007, "Microfacet Models for Refraction through Rough Surfaces"
float D_TrowbridgeReitz(float NoH, float roughness)
{
    float oneMinusNoHSquared = 1 - dot(NoH, NoH);
    float a = NoH * roughness;
    float k = roughness / max(oneMinusNoHSquared + a * a, SMALL_FLOAT_EPSILON);
    float d = k * k * PI_RCP;
    return d;
    // float roughnessSqr = roughness * roughness;
    // float distribution = NoH * NoH * (roughnessSqr - 1.0f) + 1.0f;
    // return roughnessSqr / max(PI * distribution * distribution, SMALL_FLOAT_EPSILON);
}

////////////////////////////////////////////////
// Geometric Shadowing (G)
////////////////////////////////////////////////

// Schlick-GGX function
float G_SchlickGGX(float NoV, float NoL, float roughness)
{
    float a = roughness;
    float GGXV = NoL * (NoV * (1.0 - a) + a);
    float GGXL = NoV * (NoL * (1.0 - a) + a);
    return 0.5 / max(GGXV + GGXL, SMALL_FLOAT_EPSILON);
}

// Schlick's approximation for the Beckman function. Works by multiplying roughness by sqrt(2/PI) (which is ~0.797884560802865)
float G_SchlickBeckman(float NoV, float NoL, float roughness)
{
    float roughnessSqr = roughness * roughness;

    // Equivalent to: roughnessSqr * (sqrt(2/PI))
    float k = roughnessSqr * 0.797884560802865;

    // Same calculation, we just swap the Light vector for the View vector
    float SmithL = NoL / max(NoL * (1 - k) + k, SMALL_FLOAT_EPSILON);
    float SmithV = NoV / max(NoV * (1 - k) + k, SMALL_FLOAT_EPSILON);

    float Gs = SmithL * SmithV;
    return Gs;
}

////////////////////////////////////////////////
// Fresnel term (F)
////////////////////////////////////////////////

// Schlick approximation for Fresnel term (F)
float3 F_Schlick(float VoH, float3 f0)
{
    return f0 + (1.0 - f0) * pow(1.0 - max(VoH, 0), 5.0);
}

// Version of Schlick's approximation for Fresnel term with added roughness term as described in [2] 
float3 F_SchlickRoughness(float VoH, float3 f0, float roughness)
{
    return f0 + (max((float3) (1.0 - roughness), f0) - f0) * pow(clamp(1.0 - VoH, 0.0, 1.0), 5.0);
}   

////////////////////////////////////////////////
// Specular Term
////////////////////////////////////////////////

// Cook-Torrance equation for specular term
float3 fr_SpecularBRDF(float NoH, float NoV, float NoL, float VoH, float3 kS, float roughness)
{
    // Avoid computing fresnel twice
    float3 F = kS;
    float3 numerator = D_GGX(NoH, roughness) * G_SchlickBeckman(NoV, NoL, roughness) * F;
    float denominator = 4 * NoV * NoL;

    return numerator / max(denominator, SMALL_FLOAT_EPSILON);
}

//----------------------------------------------

// Diffuse BRDF
// The following section describes the Diffuse BRDF:
// fd(v,l) = (σ / π) * (1 / (|n.v| * |n.l|)) * ∫D(m,α)*G(v,l,m)*(v⋅m)*(l⋅m)dm

// But in practice the lambertian diffuse gives good enough results
float fd_Lambert()
{
    return PI_RCP;
}

// Some use the Disney diffuse BRDF, also called the Burley BRDF but it is more expensive.

// Fresnel used for Burley
float F_SchlickDisney(float u, float f0, float f90)
{
    return f0 + (f90 - f0) * pow(1.0 - u, 5.0);
}

// Burley diffuse term
float fd_Burley(float NoV, float NoL, float LoH, float roughness)
{
    float f90 = 0.5 + 2.0 * roughness * LoH * LoH;
    float lightScatter = F_SchlickDisney(NoL, 1.0, f90);
    float viewScatter = F_SchlickDisney(NoV, 1.0, f90);
    return lightScatter * viewScatter * (1.0 / PI);
}

// For our causes, we'll just use the cheaper and more efficient lambertian diffuse
float3 fd_DiffuseBRDF(float3 diffuseColor, float NoV, float NoL, float LoH, float roughness)
{
    // We keep the other parameters to easily switch between Burley and Lambert.
    return diffuseColor * fd_Lambert();
    // return diffuseColor * fd_Burley(NoV, NoL, LoH, roughness);
}

struct Lighting
{
    float3 diffuse;
    float3 specular;
};

//----------------------------------------------------------------------------------------------
// Final BRDF calculation using the above
// 
// We suppose that diffuseColor and specularColor already have been modulated by the metalness 
// of our material which explains its non-existance as a parameter.
//----------------------------------------------------------------------------------------------
Lighting BRDF(float3 diffuseColor, float3 specularColor, float3 lightColor, float NoH, float NoV, float NoL, float VoH, float LoH, float roughness)
{
    // Conservation of energy implies that: Ks + Kd == 1 
    float3 Ks = F_SchlickRoughness(VoH, specularColor, roughness);
    float3 Kd = (1 - Ks);

    float3 diffuseTerm = fd_DiffuseBRDF(diffuseColor, NoV, NoL, LoH, roughness);
    float3 specularTerm = fr_SpecularBRDF(NoH, NoV, NoL, VoH, Ks, roughness);

    float3 color = lightColor * max(NoL, 0);

    Lighting colors;
    colors.diffuse = Kd * diffuseTerm * color;
    colors.specular = specularTerm * color;
    return colors;
}

#endif // _PBR_BRDF_