#ifndef __COLOR__
#define __COLOR__

float3 ToSRGB(float3 linearColor)
{
    return max(1.055 * pow(abs(linearColor), 0.416666667) - 0.055, 0);
}

float4 ToSRGB(float4 linearColor)
{
    return float4(ToSRGB(linearColor.rgb), linearColor.a);
}

float3 ToLinear(float3 srgbColor)
{
    return pow(srgbColor, 2.2);
}

float4 ToLinear(float4 srgbColor)
{
    return float4(ToLinear(srgbColor.rgb), srgbColor.a);
}

float ComputeLuminance(float3 color)
{
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));    
}

// Convert rgb to grayscale luma value
float RGBToLuma(float3 rgb)
{
    return sqrt(dot(rgb, float3(0.299, 0.587, 0.114)));
}

#endif // __COLOR__