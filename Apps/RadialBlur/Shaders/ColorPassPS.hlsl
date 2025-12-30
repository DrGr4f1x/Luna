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
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD;
    float3 color : COLOR;
};


Texture1D gradientTex : BINDING(t0, 1);
SamplerState linearSampler : BINDING(s0, 2);


float4 main(PSInput input) : SV_TARGET
{
	// Use max. color channel value to detect bright glow emitters
    if ((input.color.r >= 0.9f) || (input.color.g >= 0.9f) || (input.color.b >= 0.9f))
    {
        return float4(gradientTex.Sample(linearSampler, input.texcoord.x).rgb, 1.0f);
    }
    else
    {
        return float4(input.color, 1.0f);
    }
}