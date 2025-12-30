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

struct VSInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};


struct VSOutput
{
    float4 pos : SV_Position;
    float3 normal : NORMAL;
    float3 color : COLOR;
    float2 uv : TEXCOORD0;
    float3 viewVec : TEXCOORD1;
    float3 lightVec : TEXCOORD2;
};


cbuffer VSConstants : register(b0 VK_DESCRIPTOR_SET(0))
{
    float4x4 projectionMatrix;
    float4x4 modelMatrix;
    float4 lightPos;
}


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    output.pos = mul(projectionMatrix, mul(modelMatrix, float4(input.pos, 1.0)));
    output.uv = input.uv;
    output.color = input.color.rgb;

    float4 pos = mul(modelMatrix, float4(input.pos, 1.0));
    output.normal = mul((float3x3) modelMatrix, input.normal);
    float3 lPos = mul((float3x3) modelMatrix, lightPos.xyz);
    output.lightVec = lPos - pos.xyz;
    output.viewVec = -pos.xyz;

    return output;
}