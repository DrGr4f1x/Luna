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
    float3 normal : NORMAL;
    float3 viewVec : TEXCOORD0;
    float3 lightVec : TEXCOORD1;
    float3 reflected : TEXCOORD2;
};


VK_BINDING(1, 1)
TextureCube texCube : register(t0);

VK_BINDING(0, 2)
SamplerState samplerLinear : register(s0);


VK_BINDING(0, 1)
cbuffer PSConstants : register(b0)
{
    float lodBias;
};


float4 main(PSInput input) : SV_Target
{
    float3 cR = input.reflected;
    cR.xy *= -1.0f;

    float4 color = texCube.SampleBias(samplerLinear, cR, lodBias);

    return color;
}