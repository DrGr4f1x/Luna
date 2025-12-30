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

struct PSInput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
    float gradientPos : TEXCOORD1;
};


Texture2D colorTex : BINDING(t0, 2);
Texture1D gradientTex : BINDING(t1, 2);
SamplerState linearSampler : BINDING(s0, 3);


float4 main(PSInput input) : SV_Target
{
    float3 color = gradientTex.Sample(linearSampler, input.gradientPos).rgb;
    color = color * colorTex.Sample(linearSampler, input.uv).rgb;

    return float4(color, 1.0);
}