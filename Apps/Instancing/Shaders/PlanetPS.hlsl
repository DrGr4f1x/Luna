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
SamplerState linearSampler : register(s0 VK_DESCRIPTOR_SET(2));


struct PSInput
{
    float4 pos : SV_Position;
    float3 normal : NORMAL;
    float3 color : COLOR;
    float2 uv : TEXCOORD0;
    float3 viewVec : TEXCOORD1;
    float3 lightVec : TEXCOORD2;
};


float4 main(PSInput input) : SV_TARGET
{
    float4 color = colorTex.Sample(linearSampler, input.uv) * float4(input.color, 1.0) * 1.5;
	
    float3 N = normalize(input.normal);
    float3 L = normalize(input.lightVec);
    float3 V = normalize(input.viewVec);
    float3 R = reflect(-L, N);

    float3 diffuse = max(dot(N, L), 0.0) * input.color;
    float3 specular = pow(max(dot(R, V), 0.0), 4.0) * 0.5 * color.r;

    return float4(diffuse * color.rgb + specular, 1.0);
}