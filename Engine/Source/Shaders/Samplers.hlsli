#ifndef _SAMPLERS_
#define _SAMPLERS_

SamplerState PointSampler : register(s0);
SamplerState LinearSampler : register(s1);
SamplerState AnisotropicSampler : register(s2);
SamplerComparisonState ShadowMapSampler : register(s3);
SamplerState WrapLinearSampler : register(s4);

#endif