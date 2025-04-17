struct PSInput
{
    float4 position : SV_Position;
    float3 color : COLOR;
};

float3 main(PSInput input) : SV_Target
{
    return input.color;
}