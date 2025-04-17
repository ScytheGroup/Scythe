#ifndef _MATH_
#define _MATH_

// Tangent code sourced from Unreal Engine (@EpicGames)
// [Duff et al. 2017, "Building an Orthonormal Basis, Revisited"]
// Discontinuity at tangentZ.z == 0
float3x3 GetTangentBasis(float3 tangentZ)
{
    const float sign = tangentZ.z >= 0 ? 1 : -1;
    const float a = -rcp(sign + tangentZ.z);
    const float b = tangentZ.x * tangentZ.y * a;

    float3 tangentX = 
    { 
        1 + sign * a * tangentZ.x * tangentZ.x,
        sign * b,
        -sign * tangentZ.x
    };
    float3 tangentY = 
    {
        b,
        sign + a + tangentZ.y * tangentZ.y,
        -tangentZ.y
    };

    return float3x3(tangentX, tangentY, tangentZ);
}

float3 TangentToWorld(float3 vec, float3x3 tangentBasis)
{
    return mul(vec, tangentBasis);
}

float3 WorldToTangent(float3 vec, float3x3 tangentBasis)
{
    return mul(tangentBasis, vec);
}

float3 WorldPosFromDepth(float depth, float2 uv, matrix invViewProjection)
{
    float4 clipSpacePosition = float4(uv * 2.0f - 1.0f, depth, 1.0);
    clipSpacePosition.y *= -1;
    float4 worldSpacePosition = mul(invViewProjection, clipSpacePosition);
    return worldSpacePosition.xyz / worldSpacePosition.w;
}

// Returns depth in [nearPlane, farPlane] using world position.
float LinearDepthFromWorldPos(float3 worldPos, matrix view)
{
    float3 viewPos = mul(view, float4(worldPos, 1)).xyz;
    // Forward is -z
    return -viewPos.z;
}

// Normal vector encoding
// Source: https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/

float3 NormalizedXYZEncode(float3 direction)
{
    return direction * 0.5f + 0.5f;
}

float3 NormalizedXYZDecode(float3 direction)
{
    return direction * 2.0f - 1.0f;
}

float2 OctWrap(float2 v)
{
    return (1.0 - abs(v.yx)) * (v.xy >= 0.0 ? 1.0 : -1.0);
}

float2 NormalizedOctahedronEncode(float3 direction)
{
    direction /= (abs(direction.x) + abs(direction.y) + abs(direction.z));
    direction.xy = direction.z >= 0.0 ? direction.xy : OctWrap(direction.xy);
    direction.xy = direction.xy * 0.5 + 0.5;
    return direction.xy;
}

float3 NormalizedOctahedronDecode(float2 direction)
{
    direction = direction * 2.0 - 1.0;
 
    float3 n = float3(direction.x, direction.y, 1.0 - abs(direction.x) - abs(direction.y));
    float t = saturate(-n.z);
    n.xy += n.xy >= 0.0 ? -t : t;
    return normalize(n);
}

float GetViewDepth(matrix viewMatrix, float3 worldPos)
{
    return -mul(viewMatrix, float4(worldPos, 1)).z;
}

#endif // _MATH_