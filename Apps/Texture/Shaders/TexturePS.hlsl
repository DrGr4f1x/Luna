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
    float4 pos          : SV_Position;
    float2 uv           : TEXCOORD0;
    float lodBias       : TEXCOORD1;
    float3 normal       : NORMAL;
    float3 viewVec      : TEXCOORD2;
    float3 lightVec     : TEXCOORD3;
};


Texture2D colorTex : register(t0 VK_DESCRIPTOR_SET(1));
SamplerState linearSampler : register(s0 VK_DESCRIPTOR_SET(2));


float4 main(PSInput input) : SV_TARGET
{
    float4 color = colorTex.SampleLevel(linearSampler, input.uv, input.lodBias);

    float3 normal = normalize(input.normal);
    float3 lightVec = normalize(input.lightVec);
    float3 viewVec = normalize(input.viewVec);

    float3 reflectedLightVec = reflect(-lightVec, normal);

    float3 diffuse = max(dot(normal, lightVec), 0.0f);
    float3 specular = pow(max(dot(reflectedLightVec, viewVec), 0.0f), 16.0f) * color.a;

    return float4(diffuse * color.rgb + specular, 1.0f);
}