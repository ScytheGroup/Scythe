#include "InputLayouts/DebugLinesInputLayout.hlsli"
#include "Common.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float4 color : COLOR;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.position = mul(frameData.viewProjection, float4(input.position, 1));
    output.color = input.color;
    return output;
}