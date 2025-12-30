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
    float3 color : COLOR;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
    float3 lightVec : TEXCOORD1;
    float3 viewVec : TEXCOORD2;
};


Texture2D colorTex : BINDING(t0, 1);
SamplerState linearSampler : BINDING(s0, 2);


float4 main(PSInput input) : SV_TARGET
{
    float4 color = colorTex.Sample(linearSampler, input.texcoord);

    float3 N = normalize(input.normal);
    float3 L = normalize(input.lightVec);
    float3 V = normalize(input.viewVec);
    float3 R = reflect(-L, N);
    float3 diffuse = max(dot(N, L), 0.0) * input.color;
    float3 specular = pow(max(dot(R, V), 0.0), 16.0) * 0.75.xxx;

    return float4(diffuse * color.rgb + specular, 1.0);
}