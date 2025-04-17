struct PSInput
{
    float4 outPos : SV_Position;
#if STORE_LINEAR_DEPTH == 1    
    float3 worldPos : POSITION;
#endif
};

#if STORE_LINEAR_DEPTH == 1
cbuffer PointLightData : register(b8)
{
    float3 lightPos;
    float farPlane;
}

float main(PSInput input) : SV_Depth
{
    return length(input.worldPos.xyz - lightPos) / farPlane;
}
#else
void main(float4 position : SV_Position)
{
}
#endif

