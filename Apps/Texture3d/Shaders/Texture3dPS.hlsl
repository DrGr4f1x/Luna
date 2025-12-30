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
    float3 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 viewVec : TEXCOORD1;
    float3 lightVec : TEXCOORD2;
};


Texture3D texColor : register(t0 VK_DESCRIPTOR_SET(1));
SamplerState linearSampler : register(s0 VK_DESCRIPTOR_SET(2));


float4 main(PSInput input) : SV_Target
{
    float3 color = texColor.Sample(linearSampler, input.uv).rgb;

    return float4(color, 1.0f);
}