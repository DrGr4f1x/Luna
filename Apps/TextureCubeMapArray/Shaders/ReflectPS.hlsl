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


TextureCubeArray texCubeArray : register(t0 VK_DESCRIPTOR_SET(1));
SamplerState samplerLinear : register(s0 VK_DESCRIPTOR_SET(2));


cbuffer PSConstants : register(b0 VK_DESCRIPTOR_SET(1))
{
    float lodBias;
    int arraySlice;
};


float4 main(PSInput input) : SV_Target
{
    float3 cR = input.reflected;
    //cR.yz *= -1.0f;

    float4 color = texCubeArray.SampleBias(samplerLinear, float4(cR, arraySlice), lodBias);

    return color;
}