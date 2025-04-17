#ifndef _IMPORTANCE_SAMPLING_
#define _IMPORTANCE_SAMPLING_

#include "Constants.hlsli"

// Helper function for Hammersley sequences
float RadicalInverse(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);

    return float(bits) * 2.3283064365386963e-10;
}

// Hammersly sequence
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float2 Hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse(i));
}

// Reference: Real Shading in Unreal Engine 4
// by Brian Karis
// http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
// Importance sample a GGX specular function
float3 ImportanceSampleGGX(float2 xi, float roughness, float3 N)
{
    float alpha2 = roughness * roughness * roughness * roughness;
     
    float phi = 2.0f * PI * xi.x;
    float cosTheta = sqrt( (1.0f - xi.y) / (1.0f + (alpha2 - 1.0f) * xi.y ));
    float sinTheta = sqrt( 1.0f - cosTheta*cosTheta );
     
    float3 h;
    h.x = sinTheta * cos( phi );
    h.y = sinTheta * sin( phi );
    h.z = cosTheta;
     
    float3 up = abs(N.z) < 0.999 ? float3(0,0,1) : float3(1,0,0);
    float3 tangentX = normalize( cross( up, N ) );
    float3 tangentY = cross( N, tangentX );
     
    return (tangentX * h.x + tangentY * h.y + N * h.z);
}

#endif