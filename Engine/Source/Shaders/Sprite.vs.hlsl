#include "Common.hlsli"
#include "PixelShaderHelpers.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
};

VSOutput main(uint vId : SV_VERTEXID)
{
    float4 position = float4(ScreenSpaceVertices[vId], 1);
    float2 uv = position.xy * 0.5f + 0.5f;

    VSOutput output;
    output.position = mul(frameData.viewProjection, mul(objectData.model, position));
    output.uv = uv;
    return output;
}