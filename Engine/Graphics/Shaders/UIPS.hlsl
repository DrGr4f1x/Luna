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
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};


VK_BINDING(0, 1)
Texture2D fontTex : register(t0);

VK_BINDING(0, 2)
SamplerState linearSampler : register(s0);


float4 main(PSInput input) : SV_Target
{
    return input.color * fontTex.Sample(linearSampler, input.uv);
}