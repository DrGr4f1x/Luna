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
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};


struct VSOutput
{
    float4 position : SV_Position;
    float3 color : COLOR;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
    float3 lightVec : TEXCOORD1;
    float3 viewVec : TEXCOORD2;
};


cbuffer VSConstants : BINDING(b0, 0)
{
    float4x4 projectionMatrix;
    float4x4 modelMatrix;
    float4 lightPos;
};


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    output.normal = mul((float3x3) modelMatrix, input.normal);
    output.texcoord = input.texcoord;
    output.color = input.color.rgb;

    output.position = mul(projectionMatrix, mul(modelMatrix, float4(input.position, 1.0)));

    float4 pos = mul(modelMatrix, float4(input.position, 1.0));
    float3 lPos = mul((float3x3) modelMatrix, lightPos.xyz);
    output.lightVec = lPos - pos.xyz;
    output.viewVec = -pos.xyz;

    return output;
}