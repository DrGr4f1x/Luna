//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#include "Common.hlsli"


Texture2D colorTex : register(t0 VK_DESCRIPTOR_SET(1));
Texture1D gradientTex : register(t1 VK_DESCRIPTOR_SET(1));
SamplerState linearSampler : register(s0 VK_DESCRIPTOR_SET(2));


struct PSInput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
    float gradientU : TEXCOORD1;
};


float4 main(PSInput input) : SV_TARGET
{
    float3 color = colorTex.Sample(linearSampler, input.uv).rgb;
    color *= gradientTex.Sample(linearSampler, input.gradientU).rgb;

    return float4(color, 1.0);
}