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
    float3 normal : NORMAL;
    float3 viewVec : TEXCOORD1;
    float3 lightVec : TEXCOORD2;
};


Texture2D colorTex : register(t0 VK_DESCRIPTOR_SET(1));
SamplerState linearSampler : register(s0 VK_DESCRIPTOR_SET(2));


float4 main(PSInput input) : SV_Target
{
    float3 color = colorTex.Sample(linearSampler, input.uv).rgb;
    float3 N = normalize(input.normal);
    float3 L = normalize(input.lightVec);
    float3 V = normalize(input.viewVec);
    float3 R = reflect(-L, N);
    float3 diffuse = max(dot(N, L), 0.15);
    float3 specular = pow(max(dot(R, V), 0.0), 8.0) * 0.2.xxx;
    return float4(diffuse * color.rgb + specular, 1.0);
}