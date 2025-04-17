#ifndef __REINHARD__
#define __REINHARD__

float ComputeLuminance(float3 color)
{
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));    
}

float3 change_luminance(float3 c_in, float l_out)
{
    float l_in = ComputeLuminance(c_in);
    return c_in * (l_out / l_in);
}

float3 Reinhard_ToneMapping(float3 v)
{
    return v / (1.0f + v);
}

float3 Reinhard_Extended_ToneMapping(float3 v, float max_white)
{
    float3 numerator = v * (1.0f + (v / (max_white * max_white)));
    return numerator / (1.0f + v);
}

float3 Reinhard_ExtendedLuminance_ToneMapping(float3 v, float max_white_l)
{
    float l_old = ComputeLuminance(v);
    float numerator = l_old * (1.0f + (l_old / (max_white_l * max_white_l)));
    float l_new = numerator / (1.0f + l_old);
    return change_luminance(v, l_new);
}

float3 Reinhard_Jodie_ToneMapping(float3 v)
{
    float l = ComputeLuminance(v);
    float3 tv = v / (1.0f + v);
    return lerp(v / (1.0f + l), tv, tv);
}

#endif