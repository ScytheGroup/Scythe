#ifndef _PIXEL_SHADER_HELPERS_
#define _PIXEL_SHADER_HELPERS_

void ComputeTriangleVertex(in uint vertexID, out float4 position, out float2 uv)
{
    uv = float2((vertexID << 1) & 2, vertexID & 2);
    position = float4(uv.x * 2 - 1, -uv.y * 2 + 1, 0, 1);
}

static const float3 ScreenSpaceVertices[6] =
{
    float3(1, 1, 0),
    float3(-1, -1, 0),
    float3(-1, 1, 0),
    float3(1, 1, 0),
    float3(1, -1, 0),
    float3(-1, -1, 0)
};

/*
// Here is a template for a shader that uses Graphics::DrawFullScreenTriangle(...)
// This can be easily copy-pasted when starting a new file.
struct PSInput
{
    float4 position: SV_POSITION;
    float2 uv : TEXCOORD;
};

struct PSOutput
{
    float4 color: SV_Target;
    // TODO: add more stuff if needed depending on your output :-)
};

PSOutput main(PSInput input)
{
    PSOutput output;
    // TODO: fill in output color
    return output;
}

*/

#endif // _PIXEL_SHADER_HELPERS_